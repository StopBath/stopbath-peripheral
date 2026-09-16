#include "remote_session_device.h"

/* The device's inputs as the session sees them: one integer per physical
 * press the input model can classify. */
typedef enum {
    PicoSessionInputKey0Short,
    PicoSessionInputKey0Long,
    PicoSessionInputKey1Short,
    PicoSessionInputKey1Long,
    PicoSessionInputUnknown,
} PicoSessionInput;

static PicoSessionInput input_for_press(RemoteInputKey key, RemoteInputPressKind press_kind) {
    if(key == RemoteInputKey0 && press_kind == RemoteInputPressShort) return PicoSessionInputKey0Short;
    if(key == RemoteInputKey0 && press_kind == RemoteInputPressLong) return PicoSessionInputKey0Long;
    if(key == RemoteInputKey1 && press_kind == RemoteInputPressShort) return PicoSessionInputKey1Short;
    if(key == RemoteInputKey1 && press_kind == RemoteInputPressLong) return PicoSessionInputKey1Long;
    return PicoSessionInputUnknown;
}

/* No lock on this device (KD5); reported honestly. */
static bool pico_screen_locked(void* device_context) {
    (void)device_context;
    return false;
}

/* Single purpose firmware: always foregrounded, honestly (spec 2.4). */
static bool pico_foregrounded(void* device_context) {
    (void)device_context;
    return true;
}

RemoteProtocolEvent remote_session_page_event_for_key1(int page) {
    /* Spec 2.3, KD4: the appliance's page events are absolute (LEFT_SHORT is
     * the WIFI page, RIGHT_SHORT the GUEST page), and this device has one
     * key to move between them. From the GUEST page the other page is WIFI;
     * from anywhere else, including no page and a value the enumeration
     * does not name, it is GUEST, which the appliance answers with an
     * unchanged record outside a session. Recorded as a deviation from
     * Flipper 2.1 in IMPLEMENTATION_DEVIATIONS.md; provisional until KD9. */
    return page == (int)RemoteProtocolPageGuest ? RemoteProtocolEventLeftShort : RemoteProtocolEventRightShort;
}

static bool pico_event_for_input(void* device_context, int input, int current_page, RemoteProtocolEvent* wire_event) {
    (void)device_context;
    switch((PicoSessionInput)input) {
    case PicoSessionInputKey0Short:
        *wire_event = RemoteProtocolEventCenterShort;
        return true;
    case PicoSessionInputKey0Long:
        *wire_event = RemoteProtocolEventCenterLong;
        return true;
    case PicoSessionInputKey1Short:
        *wire_event = remote_session_page_event_for_key1(current_page);
        return true;
    case PicoSessionInputKey1Long:
        /* Reserved (spec 2.2): not a press this device reports. */
        return false;
    case PicoSessionInputUnknown:
    default:
        return false;
    }
}

RemoteSessionDevice pico_session_device(void) {
    RemoteSessionDevice surface = {
        .peripheral_token = PICO_SESSION_PERIPHERAL_TOKEN,
        .device_context = NULL,
        .screen_locked = pico_screen_locked,
        .foregrounded = pico_foregrounded,
        .event_for_input = pico_event_for_input,
        /* The panel is redrawn from the polled state by the refresher, so
         * no callbacks. */
        .record_received = NULL,
        .link_changed = NULL,
    };
    return surface;
}

bool remote_session_report_press(RemoteSession* session, RemoteInputKey key, RemoteInputPressKind press_kind) {
    return remote_session_report_input(session, (int)input_for_press(key, press_kind));
}

static void copy_bounded(char* destination, size_t capacity, const char* source) {
    size_t length = 0;
    while(length + 1 < capacity && source[length] != '\0') {
        destination[length] = source[length];
        length++;
    }
    destination[length] = '\0';
}

void remote_session_display(const RemoteSession* session, RemoteDisplayState* display_state) {
    remote_display_state_initialise(display_state);
    const RemoteSessionCounters* counters = remote_session_counters(session);
    display_state->diagnostics.reconnections = counters->reconnections;
    display_state->diagnostics.malformed_received = counters->malformed_received;
    display_state->diagnostics.version_mismatches = counters->version_mismatches;
    display_state->diagnostics.events_dropped_no_link = counters->events_dropped_no_link;
    display_state->diagnostics.events_dropped_by_output_full = counters->events_dropped_by_output_full;
    display_state->diagnostics.handshake_retries = counters->handshake_retries;

    switch(remote_session_link_state(session)) {
    case RemoteSessionLinkDown:
        display_state->link_connected = false;
        break;
    case RemoteSessionHandshaking:
        display_state->link_connected = false;
        display_state->link_connecting = true;
        break;
    case RemoteSessionIncompatible:
        display_state->link_connected = false;
        display_state->link_incompatible = true;
        break;
    case RemoteSessionConnected: {
        display_state->link_connected = true;
        const RemoteProtocolMessage* record = remote_session_current_record(session);
        display_state->status = (int)record->fields[RemoteProtocolDisplayFieldStatus].integer;
        display_state->page = (int)record->fields[RemoteProtocolDisplayFieldPage].integer;
        copy_bounded(display_state->payload, sizeof(display_state->payload), record->fields[RemoteProtocolDisplayFieldPayload].text);
        display_state->delivered_count = record->fields[RemoteProtocolDisplayFieldDelivered].integer;
        /* The error field becomes the display's error code text through the
         * enumeration's wire names, so the code shown is the code sent. NONE
         * leaves the band off. */
        uint32_t error = record->fields[RemoteProtocolDisplayFieldError].integer;
        if(error != RemoteProtocolErrorNone) {
            int error_count = 0;
            const char* const* error_names = remote_protocol_enumeration_values(RemoteProtocolEnumerationError, &error_count);
            if((int)error < error_count) {
                copy_bounded(display_state->error_code, sizeof(display_state->error_code), error_names[error]);
            }
        }
        break;
    }
    }
}
