#include "remote_session.h"

#include <string.h>

static const char* const SAFE_DEFAULT_TOKEN = "peripheral";

static bool token_is_valid(const char* token) {
    size_t length = strlen(token);
    if(length == 0 || length > REMOTE_PROTOCOL_MAXIMUM_PERIPHERAL_TOKEN_LENGTH) return false;
    for(size_t index = 0; index < length; index++) {
        char character = token[index];
        bool allowed = (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '-';
        if(!allowed) return false;
    }
    return true;
}

/* Copies without ever writing past the destination; a source longer than
 * the destination is cut, which cannot happen to a validated token. */
static void copy_bounded(char* destination, size_t capacity, const char* source) {
    size_t length = 0;
    while(length + 1 < capacity && source[length] != '\0') {
        destination[length] = source[length];
        length++;
    }
    destination[length] = '\0';
}

static void reset_display_record(RemoteProtocolMessage* record) {
    remote_protocol_message_initialise(record, RemoteProtocolVerbDisplay);
    remote_protocol_message_set_integer(record, RemoteProtocolDisplayFieldStatus, RemoteProtocolStatusReady);
    remote_protocol_message_set_integer(record, RemoteProtocolDisplayFieldPage, RemoteProtocolPageNone);
    remote_protocol_message_set_text(record, RemoteProtocolDisplayFieldPayload, "");
    remote_protocol_message_set_integer(record, RemoteProtocolDisplayFieldDelivered, 0);
    remote_protocol_message_set_integer(record, RemoteProtocolDisplayFieldError, RemoteProtocolErrorNone);
}

static void clear_output(RemoteSession* session) {
    session->output_length = 0;
    session->output_message_count = 0;
}

static void enter_link_state(RemoteSession* session, RemoteSessionLinkState link_state) {
    session->link_state = link_state;
    if(session->device.link_changed != NULL) {
        session->device.link_changed(session->device.device_context, link_state);
    }
}

void remote_session_initialise(RemoteSession* session, RemoteSessionDevice device) {
    memset(session, 0, sizeof(*session));
    session->device = device;
    const char* token = (device.peripheral_token != NULL && token_is_valid(device.peripheral_token)) ? device.peripheral_token : SAFE_DEFAULT_TOKEN;
    copy_bounded(session->peripheral_token, sizeof(session->peripheral_token), token);
    session->link_state = RemoteSessionLinkDown;
    reset_display_record(&session->current_display);
    remote_protocol_line_assembler_initialise(&session->inbound);
}

/* Appends an encoded message to the output, or drops it and counts why. A
 * message that does not fit is dropped whole; nothing is ever written past
 * the buffer. Returns true if it was queued. */
static bool queue_message(RemoteSession* session, const RemoteProtocolMessage* message, uint32_t* drop_counter) {
    uint8_t line[REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH];
    size_t line_length = 0;
    if(!remote_protocol_encode(message, (char*)line, sizeof(line), &line_length)) {
        return false;
    }
    /* Bounded by the message count first (the queue depth), then by the byte
     * buffer as a hard backstop. Either full drops the message whole. */
    if(session->output_message_count >= REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH ||
       session->output_length + line_length > sizeof(session->output)) {
        if(drop_counter != NULL) {
            (*drop_counter)++;
        }
        return false;
    }
    memcpy(session->output + session->output_length, line, line_length);
    session->output_length += line_length;
    session->output_message_count++;
    return true;
}

static bool device_is_locked(const RemoteSession* session) {
    return session->device.screen_locked(session->device.device_context);
}

static void send_hello(RemoteSession* session) {
    RemoteProtocolMessage hello;
    remote_protocol_message_initialise(&hello, RemoteProtocolVerbHello);
    remote_protocol_message_set_integer(&hello, RemoteProtocolHelloFieldVersion, REMOTE_PROTOCOL_VERSION);
    remote_protocol_message_set_text(&hello, RemoteProtocolHelloFieldPeripheral, session->peripheral_token);
    remote_protocol_message_set_integer(&hello, RemoteProtocolHelloFieldLocked, device_is_locked(session) ? 1 : 0);
    queue_message(session, &hello, NULL);
}

void remote_session_port_opened(RemoteSession* session) {
    /* A fresh handshake every time, discarding any prior link state and any
     * half received line (Flipper 2.10). */
    remote_protocol_line_assembler_initialise(&session->inbound);
    clear_output(session);
    reset_display_record(&session->current_display);
    session->handshake_elapsed_milliseconds = 0;
    enter_link_state(session, RemoteSessionHandshaking);
    send_hello(session);
}

void remote_session_tick(RemoteSession* session, uint32_t elapsed_milliseconds) {
    if(session->link_state != RemoteSessionHandshaking) {
        session->handshake_elapsed_milliseconds = 0;
        return;
    }
    session->handshake_elapsed_milliseconds += elapsed_milliseconds;
    if(session->handshake_elapsed_milliseconds < REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS) {
        return;
    }
    session->handshake_elapsed_milliseconds = 0;
    session->counters.handshake_retries++;
    /* Only a stale HELLO can be queued while handshaking (a button is sent
     * only while connected), so clearing the output leaves exactly one fresh
     * HELLO rather than copies piling up if the previous was slow to drain. */
    clear_output(session);
    send_hello(session);
}

void remote_session_port_closed(RemoteSession* session) {
    if(session->link_state != RemoteSessionLinkDown) {
        session->counters.reconnections++;
    }
    /* Discard link state and clear the sensitive payload (Flipper Part 5).
     * Anything not yet drained is dropped, which is what stops a stale
     * press crossing the reconnection (Flipper 2.10, prohibition 8). */
    remote_protocol_line_assembler_initialise(&session->inbound);
    clear_output(session);
    reset_display_record(&session->current_display);
    enter_link_state(session, RemoteSessionLinkDown);
}

static void handle_display(RemoteSession* session, const RemoteProtocolMessage* record) {
    uint32_t error = record->fields[RemoteProtocolDisplayFieldError].integer;
    if(session->link_state != RemoteSessionConnected && error == RemoteProtocolErrorBadVersion) {
        /* The appliance will not talk to this version (Flipper 2.10 step 2).
         * A rejection is not a record to render, so the device is told the
         * state and not handed the record. */
        session->counters.version_mismatches++;
        enter_link_state(session, RemoteSessionIncompatible);
        return;
    }
    session->current_display = *record;
    if(session->link_state != RemoteSessionConnected) {
        enter_link_state(session, RemoteSessionConnected);
    }
    if(session->device.record_received != NULL) {
        session->device.record_received(session->device.device_context, &session->current_display);
    }
}

void remote_session_receive(RemoteSession* session, const uint8_t* bytes, size_t byte_count) {
    RemoteProtocolMessage message;
    for(size_t index = 0; index < byte_count; index++) {
        RemoteProtocolFeedOutcome outcome = remote_protocol_feed_byte(&session->inbound, bytes[index], &message);
        if(outcome.kind == RemoteProtocolFeedOutcomeMessage) {
            if(message.verb == RemoteProtocolVerbDisplay) {
                handle_display(session, &message);
            } else {
                /* Only DISPLAY flows from the appliance; anything else is a
                 * peer speaking out of turn and is treated as malformed. */
                session->counters.malformed_received++;
            }
        } else if(outcome.kind == RemoteProtocolFeedOutcomeError) {
            session->counters.malformed_received++;
        }
    }
}

bool remote_session_report_input(RemoteSession* session, int input) {
    RemoteProtocolEvent wire_event;
    int current_page = (int)session->current_display.fields[RemoteProtocolDisplayFieldPage].integer;
    if(!session->device.event_for_input(session->device.device_context, input, current_page, &wire_event)) {
        /* Not a press the device reports (a reserved key): not a fault, so
         * not counted. */
        return false;
    }
    if(session->link_state != RemoteSessionConnected) {
        session->counters.events_dropped_no_link++;
        return false;
    }
    /* The guard (Flipper 2.4): asked of the device at the moment of the
     * press, because a peripheral that self certifies its guard is trusting
     * the untrusted side, and the same reasoning applies within it. A device
     * with no lock and no background answers honestly and is never dropped
     * here. */
    bool foregrounded = session->device.foregrounded(session->device.device_context);
    bool unlocked = !device_is_locked(session);
    if(!foregrounded || !unlocked) {
        session->counters.events_dropped_by_guard++;
        return false;
    }

    RemoteProtocolMessage button;
    remote_protocol_message_initialise(&button, RemoteProtocolVerbButton);
    remote_protocol_message_set_integer(&button, RemoteProtocolButtonFieldEvent, wire_event);
    /* Both true by construction at this point; sent so the appliance can
     * make the final decision on what it receives (Flipper 2.4). */
    remote_protocol_message_set_integer(&button, RemoteProtocolButtonFieldForegrounded, 1);
    remote_protocol_message_set_integer(&button, RemoteProtocolButtonFieldUnlocked, 1);
    return queue_message(session, &button, &session->counters.events_dropped_by_output_full);
}

void remote_session_lock_changed(RemoteSession* session) {
    if(session->link_state != RemoteSessionConnected) {
        return;
    }
    RemoteProtocolMessage state;
    remote_protocol_message_initialise(&state, RemoteProtocolVerbState);
    remote_protocol_message_set_integer(&state, RemoteProtocolStateFieldLocked, device_is_locked(session) ? 1 : 0);
    queue_message(session, &state, NULL);
}

size_t remote_session_take_output(RemoteSession* session, uint8_t* destination, size_t destination_capacity) {
    size_t taken = session->output_length < destination_capacity ? session->output_length : destination_capacity;
    memcpy(destination, session->output, taken);
    memmove(session->output, session->output + taken, session->output_length - taken);
    session->output_length -= taken;
    /* Recount whole messages remaining by their terminators, so a partial
     * drain leaves the count honest and a fresh message can still be queued
     * up to the depth. */
    session->output_message_count = 0;
    for(size_t index = 0; index < session->output_length; index++) {
        if(session->output[index] == REMOTE_PROTOCOL_TERMINATOR) {
            session->output_message_count++;
        }
    }
    return taken;
}

RemoteSessionLinkState remote_session_link_state(const RemoteSession* session) {
    return session->link_state;
}

const RemoteProtocolMessage* remote_session_current_record(const RemoteSession* session) {
    return &session->current_display;
}

const RemoteSessionCounters* remote_session_counters(const RemoteSession* session) {
    return &session->counters;
}
