/*
 * The shared client session (peripheral spec SE5): the union of the Flipper's
 * FE4 session tests and the Pico's KE3 session tests, rewritten against the
 * device surface, a struct of function pointers supplied at initialisation
 * (SD4). Two fake devices stand in for the real ones: one shaped like the
 * Flipper (a real lock and foreground, five buttons) and one shaped like the
 * Pico (no lock, always foregrounded, two keys with the second's event chosen
 * by the current page). Nothing here knows a display; what a record looks
 * like on a screen is each device's own test, under its directory.
 *
 * What the session owns, and this suite proves: the handshake with retry,
 * wholesale replacement of the record, BAD_VERSION marking the link
 * incompatible, drop on disconnect with nothing queued across it, the
 * bounded outbound queue, the malformed and dropped counters, and the
 * invariant that a press is sent only while connected. What it does not
 * own, and this suite never asserts: what any button means.
 */
#include "test_support.h"

#include "../session/remote_session.h"

#include <stdio.h>
#include <string.h>

/* A stand-in appliance built from the protocol library, so a test drives the
 * session with real bytes rather than hand written lines. */
static size_t encode_display(uint8_t* line, RemoteProtocolStatus status, RemoteProtocolPage page, const char* payload, uint32_t delivered, RemoteProtocolError error) {
    RemoteProtocolMessage record;
    remote_protocol_message_initialise(&record, RemoteProtocolVerbDisplay);
    remote_protocol_message_set_integer(&record, RemoteProtocolDisplayFieldStatus, status);
    remote_protocol_message_set_integer(&record, RemoteProtocolDisplayFieldPage, page);
    remote_protocol_message_set_text(&record, RemoteProtocolDisplayFieldPayload, payload);
    remote_protocol_message_set_integer(&record, RemoteProtocolDisplayFieldDelivered, delivered);
    remote_protocol_message_set_integer(&record, RemoteProtocolDisplayFieldError, error);
    size_t line_length = 0;
    remote_protocol_encode(&record, (char*)line, REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH, &line_length);
    return line_length;
}

static void receive_display(RemoteSession* session, RemoteProtocolStatus status, RemoteProtocolPage page, const char* payload, uint32_t delivered, RemoteProtocolError error) {
    uint8_t line[REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH];
    size_t length = encode_display(line, status, page, payload, delivered, error);
    remote_session_receive(session, line, length);
}

/* Takes the session's output as a terminated string. */
static void take_output(RemoteSession* session, char* destination, size_t capacity) {
    size_t length = remote_session_take_output(session, (uint8_t*)destination, capacity - 1);
    destination[length] = '\0';
}

/*
 * The fake devices. One struct serves both shapes: the Flipper shaped one
 * reads its lock and foreground from the struct, the Pico shaped one reports
 * fixed values. Both record what the session tells them, so the callbacks
 * are proven as well as the polled state.
 */
typedef enum {
    FakeInputCenterShort,
    FakeInputCenterLong,
    FakeInputLeftShort,
    FakeInputRightShort,
    /* The Pico shaped device's second key: its event depends on the page. */
    FakeInputPageKeyShort,
    /* An input the device does not report (the Pico's reserved Key1 long). */
    FakeInputReserved,
} FakeInput;

typedef struct {
    bool screen_locked;
    bool foregrounded;
    bool has_lock;
    int records_received;
    RemoteProtocolMessage last_record;
    int link_changes;
    RemoteSessionLinkState last_link_state;
} FakeDevice;

static bool fake_screen_locked(void* device_context) {
    FakeDevice* device = device_context;
    return device->has_lock && device->screen_locked;
}

static bool fake_foregrounded(void* device_context) {
    FakeDevice* device = device_context;
    return device->foregrounded;
}

static bool fake_event_for_input(void* device_context, int input, int current_page, RemoteProtocolEvent* wire_event) {
    (void)device_context;
    switch((FakeInput)input) {
    case FakeInputCenterShort:
        *wire_event = RemoteProtocolEventCenterShort;
        return true;
    case FakeInputCenterLong:
        *wire_event = RemoteProtocolEventCenterLong;
        return true;
    case FakeInputLeftShort:
        *wire_event = RemoteProtocolEventLeftShort;
        return true;
    case FakeInputRightShort:
        *wire_event = RemoteProtocolEventRightShort;
        return true;
    case FakeInputPageKeyShort:
        /* The Pico's choice, as its device would inject it; the session only
         * passes the page through and never looks at it. */
        *wire_event = current_page == (int)RemoteProtocolPageGuest ? RemoteProtocolEventLeftShort : RemoteProtocolEventRightShort;
        return true;
    case FakeInputReserved:
        return false;
    }
    return false;
}

static void fake_record_received(void* device_context, const RemoteProtocolMessage* record) {
    FakeDevice* device = device_context;
    device->records_received++;
    device->last_record = *record;
}

static void fake_link_changed(void* device_context, RemoteSessionLinkState link_state) {
    FakeDevice* device = device_context;
    device->link_changes++;
    device->last_link_state = link_state;
}

static RemoteSessionDevice surface_for(FakeDevice* device, const char* token) {
    RemoteSessionDevice surface = {
        .peripheral_token = token,
        .device_context = device,
        .screen_locked = fake_screen_locked,
        .foregrounded = fake_foregrounded,
        .event_for_input = fake_event_for_input,
        .record_received = fake_record_received,
        .link_changed = fake_link_changed,
    };
    return surface;
}

static void flipper_shaped(FakeDevice* device) {
    memset(device, 0, sizeof(*device));
    device->has_lock = true;
    device->foregrounded = true;
}

static void pico_shaped(FakeDevice* device) {
    memset(device, 0, sizeof(*device));
    device->has_lock = false;
    device->foregrounded = true;
}

/* A session connected with the given record, output drained. */
static void connect(RemoteSession* session, RemoteProtocolStatus status, RemoteProtocolPage page, const char* payload) {
    char output[256];
    remote_session_port_opened(session);
    take_output(session, output, sizeof(output));
    receive_display(session, status, page, payload, 0, RemoteProtocolErrorNone);
}

static void a_fresh_session_is_down_and_silent(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionLinkDown, remote_session_link_state(&session), "down");
    char output[256];
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, output[0] == '\0', "nothing sent");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, device.link_changes, "nothing announced");
}

static void opening_the_port_sends_hello_and_begins_handshake(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    remote_session_port_opened(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, remote_session_link_state(&session), "handshaking");
    char output[256];
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "HELLO version=1 peripheral=flipper-zero locked=0\n") == 0, "hello sent with lock state");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, device.last_link_state, "the device was told");
}

static void hello_carries_the_injected_token_and_lock_value(RemoteTestReport* report) {
    /* The Flipper shaped device, locked. */
    FakeDevice locked_device;
    flipper_shaped(&locked_device);
    locked_device.screen_locked = true;
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&locked_device, "flipper-zero"));
    remote_session_port_opened(&session);
    char output[256];
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "HELLO version=1 peripheral=flipper-zero locked=1\n") == 0, "locked hello");

    /* The Pico shaped device: fixed token, no lock, honestly. */
    FakeDevice pico_device;
    pico_shaped(&pico_device);
    pico_device.screen_locked = true; /* ignored: the device has no lock */
    remote_session_initialise(&session, surface_for(&pico_device, "stopbath-pico"));
    remote_session_port_opened(&session);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "HELLO version=1 peripheral=stopbath-pico locked=0\n") == 0, "fixed token, no lock");
}

static void the_first_display_completes_the_handshake_and_reaches_the_device(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    connect(&session, RemoteProtocolStatusPresenting, RemoteProtocolPageWifi, "WIFI:T:WPA;S:a;P:b;;");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionConnected, remote_session_link_state(&session), "connected");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionConnected, device.last_link_state, "the device was told it is connected");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, device.records_received, "the record was handed over once");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteProtocolStatusPresenting, (int)device.last_record.fields[RemoteProtocolDisplayFieldStatus].integer, "status as sent");
    REMOTE_TEST_ASSERT(report, strcmp(device.last_record.fields[RemoteProtocolDisplayFieldPayload].text, "WIFI:T:WPA;S:a;P:b;;") == 0, "payload as sent");
    const RemoteProtocolMessage* held = remote_session_current_record(&session);
    REMOTE_TEST_ASSERT(report, strcmp(held->fields[RemoteProtocolDisplayFieldPayload].text, "WIFI:T:WPA;S:a;P:b;;") == 0, "and held for the device to read");
}

static void a_bad_version_answer_marks_the_link_incompatible(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    remote_session_port_opened(&session);
    char output[256];
    take_output(&session, output, sizeof(output));
    receive_display(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "", 0, RemoteProtocolErrorBadVersion);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionIncompatible, remote_session_link_state(&session), "incompatible");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, (int)remote_session_counters(&session)->version_mismatches, "counted");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionIncompatible, device.last_link_state, "the device was told");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, device.records_received, "the rejection is not a record to render");
    REMOTE_TEST_ASSERT(report, !remote_session_report_input(&session, FakeInputCenterShort), "no event while incompatible");
}

static void a_later_record_replaces_the_previous_one_wholly(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    connect(&session, RemoteProtocolStatusGuestConnected, RemoteProtocolPageGuest, "HTTP://192.168.72.1/");
    receive_display(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "", 0, RemoteProtocolErrorNone);
    const RemoteProtocolMessage* held = remote_session_current_record(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteProtocolStatusReady, (int)held->fields[RemoteProtocolDisplayFieldStatus].integer, "replaced");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)held->fields[RemoteProtocolDisplayFieldDelivered].integer, "count replaced, not merged");
    REMOTE_TEST_ASSERT(report, held->fields[RemoteProtocolDisplayFieldPayload].text[0] == '\0', "payload cleared by the new record");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 2, device.records_received, "each record handed over");
}

static void a_disconnect_mid_message_discards_the_partial_line(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    connect(&session, RemoteProtocolStatusPresenting, RemoteProtocolPageWifi, "WIFI:T:WPA;S:a;P:b;;");

    const char* half = "DISPLAY status=GUEST_CONNECTED page=GUEST payl";
    remote_session_receive(&session, (const uint8_t*)half, strlen(half));
    remote_session_port_closed(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionLinkDown, remote_session_link_state(&session), "down");

    char output[256];
    remote_session_port_opened(&session);
    take_output(&session, output, sizeof(output));
    const char* rest = "oad=x delivered=0 error=NONE\n";
    remote_session_receive(&session, (const uint8_t*)rest, strlen(rest));
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, remote_session_link_state(&session), "still handshaking, not connected on stale bytes");
}

static void closing_the_port_discards_everything_and_clears_the_payload(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    connect(&session, RemoteProtocolStatusPresenting, RemoteProtocolPageWifi, "WIFI:T:WPA;S:secret;P:passphrase;;");

    remote_session_port_closed(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionLinkDown, remote_session_link_state(&session), "down");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionLinkDown, device.last_link_state, "the device was told");
    REMOTE_TEST_ASSERT(report, remote_session_current_record(&session)->fields[RemoteProtocolDisplayFieldPayload].text[0] == '\0', "the passphrase is cleared on disconnect");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, (int)remote_session_counters(&session)->reconnections, "counted");

    remote_session_port_opened(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, remote_session_link_state(&session), "connecting, not the old record");
    REMOTE_TEST_ASSERT(report, remote_session_current_record(&session)->fields[RemoteProtocolDisplayFieldPayload].text[0] == '\0', "still no stale payload");
}

static void a_press_is_sent_only_while_connected_and_the_guard_allows(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    char output[256];

    REMOTE_TEST_ASSERT(report, !remote_session_report_input(&session, FakeInputCenterShort), "not while down");
    remote_session_port_opened(&session);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, !remote_session_report_input(&session, FakeInputCenterShort), "not while handshaking");
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, output[0] == '\0', "nothing sent while handshaking");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 2, (int)remote_session_counters(&session)->events_dropped_no_link, "both counted as no link");

    receive_display(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "", 0, RemoteProtocolErrorNone);
    REMOTE_TEST_ASSERT(report, remote_session_report_input(&session, FakeInputCenterLong), "sent");
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "BUTTON event=CENTER_LONG foregrounded=1 unlocked=1\n") == 0, "both flags true");

    device.foregrounded = false;
    REMOTE_TEST_ASSERT(report, !remote_session_report_input(&session, FakeInputCenterShort), "not while backgrounded");
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, output[0] == '\0', "nothing sent while backgrounded");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, (int)remote_session_counters(&session)->events_dropped_by_guard, "guard drop counted");

    device.foregrounded = true;
    device.screen_locked = true;
    REMOTE_TEST_ASSERT(report, !remote_session_report_input(&session, FakeInputCenterShort), "not while locked, even foregrounded");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 2, (int)remote_session_counters(&session)->events_dropped_by_guard, "counted again");
}

static void every_reportable_input_is_sent_as_the_event_the_device_maps_it_to(RemoteTestReport* report) {
    typedef struct {
        FakeInput input;
        RemoteProtocolPage page_before;
        const char* wire;
        const char* description;
    } InputRow;
    static const InputRow rows[] = {
        {FakeInputCenterShort, RemoteProtocolPageNone, "CENTER_SHORT", "centre short"},
        {FakeInputCenterLong, RemoteProtocolPageWifi, "CENTER_LONG", "centre long"},
        {FakeInputLeftShort, RemoteProtocolPageGuest, "LEFT_SHORT", "left short"},
        {FakeInputRightShort, RemoteProtocolPageWifi, "RIGHT_SHORT", "right short"},
        {FakeInputPageKeyShort, RemoteProtocolPageWifi, "RIGHT_SHORT", "the page key on the wifi page: the device chose right"},
        {FakeInputPageKeyShort, RemoteProtocolPageGuest, "LEFT_SHORT", "the page key on the guest page: the device chose left"},
        {FakeInputPageKeyShort, RemoteProtocolPageNone, "RIGHT_SHORT", "the page key with no page: the device chose right"},
    };
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(rows); row_index++) {
        FakeDevice device;
        pico_shaped(&device);
        RemoteSession session;
        remote_session_initialise(&session, surface_for(&device, "stopbath-pico"));
        connect(&session, RemoteProtocolStatusReady, rows[row_index].page_before, "");
        REMOTE_TEST_ASSERT(report, remote_session_report_input(&session, rows[row_index].input), rows[row_index].description);
        char output[256];
        take_output(&session, output, sizeof(output));
        char expected[96];
        snprintf(expected, sizeof(expected), "BUTTON event=%s foregrounded=1 unlocked=1\n", rows[row_index].wire);
        REMOTE_TEST_ASSERT(report, strcmp(output, expected) == 0, rows[row_index].description);
    }
}

static void an_input_the_device_does_not_report_sends_nothing_and_is_not_an_error(RemoteTestReport* report) {
    FakeDevice device;
    pico_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "stopbath-pico"));
    connect(&session, RemoteProtocolStatusPresenting, RemoteProtocolPageWifi, "");
    REMOTE_TEST_ASSERT(report, !remote_session_report_input(&session, FakeInputReserved), "reserved, unsent");
    char output[256];
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, output[0] == '\0', "nothing on the wire");
    const RemoteSessionCounters* counters = remote_session_counters(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)counters->events_dropped_no_link, "not counted as a drop");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)counters->events_dropped_by_guard, "nor by the guard");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)counters->events_dropped_by_output_full, "nor as a full queue");
}

static void a_press_is_not_queued_across_a_disconnection(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    connect(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "");
    char output[256];

    REMOTE_TEST_ASSERT(report, remote_session_report_input(&session, FakeInputCenterLong), "reported");
    remote_session_port_closed(&session);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, output[0] == '\0', "the unsent press is discarded on disconnect");

    remote_session_port_opened(&session);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strncmp(output, "HELLO", 5) == 0, "reconnect sends only a fresh hello");
    REMOTE_TEST_ASSERT(report, strstr(output, "CENTER_LONG") == NULL, "no replayed press");
}

static void lock_changes_send_state_only_while_connected(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    char output[256];

    device.screen_locked = true;
    remote_session_lock_changed(&session);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, output[0] == '\0', "no state while down");

    device.screen_locked = false;
    connect(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "");
    remote_session_lock_changed(&session);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "STATE locked=0\n") == 0, "state sent with the device's value");
    device.screen_locked = true;
    remote_session_lock_changed(&session);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "STATE locked=1\n") == 0, "state sent again");
}

static void malformed_and_unexpected_verbs_are_counted_and_leave_the_record_alone(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    remote_session_port_opened(&session);
    char output[256];
    take_output(&session, output, sizeof(output));

    const char* garbage = "%%% not a line %%%\n";
    remote_session_receive(&session, (const uint8_t*)garbage, strlen(garbage));
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, remote_session_link_state(&session), "still handshaking");
    REMOTE_TEST_ASSERT(report, remote_session_counters(&session)->malformed_received >= 1, "counted");

    const char* wrong_direction = "BUTTON event=CENTER_SHORT foregrounded=1 unlocked=1\n";
    remote_session_receive(&session, (const uint8_t*)wrong_direction, strlen(wrong_direction));
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, remote_session_link_state(&session), "a button does not connect");
    REMOTE_TEST_ASSERT(report, remote_session_counters(&session)->malformed_received >= 2, "wrong direction counted");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, device.records_received, "nothing handed to the device");

    /* Connected, then garbage: the held record is untouched. */
    receive_display(&session, RemoteProtocolStatusGuestConnected, RemoteProtocolPageGuest, "HTTP://192.168.72.1/", 3, RemoteProtocolErrorNone);
    remote_session_receive(&session, (const uint8_t*)garbage, strlen(garbage));
    const RemoteProtocolMessage* held = remote_session_current_record(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 3, (int)held->fields[RemoteProtocolDisplayFieldDelivered].integer, "the record is left alone");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, device.records_received, "and not re-handed");
}

static void the_output_queue_is_bounded_and_an_overflowing_press_is_dropped_and_counted(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    connect(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "");

    int transmitted = 0;
    for(int repeat = 0; repeat < 100; repeat++) {
        if(remote_session_report_input(&session, FakeInputCenterShort)) transmitted++;
    }
    REMOTE_TEST_ASSERT(report, transmitted <= (int)REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH, "no more than the queue depth held");
    REMOTE_TEST_ASSERT(report, remote_session_counters(&session)->events_dropped_by_output_full > 0, "the rest dropped and counted");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 100 - transmitted, (int)remote_session_counters(&session)->events_dropped_by_output_full, "every one of them");
    char output[REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH * REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH + 1];
    take_output(&session, output, sizeof(output));
    int lines = 0;
    for(const char* cursor = output; *cursor != '\0'; cursor++) {
        if(*cursor == '\n') lines++;
    }
    REMOTE_TEST_ASSERT_EQUAL_INT(report, transmitted, lines, "exactly the transmitted ones are on the wire, whole");
}

static void an_invalid_token_is_replaced_with_a_safe_default(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "Flipper Zero!"));
    remote_session_port_opened(&session);
    char output[256];
    take_output(&session, output, sizeof(output));
    RemoteProtocolMessage decoded;
    RemoteProtocolFeedOutcome outcome = remote_protocol_parse_line(output, strlen(output) - 1, &decoded);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteProtocolFeedOutcomeMessage, outcome.kind, "a valid hello is still sent");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteProtocolVerbHello, decoded.verb, "hello");
}

static void a_stalled_handshake_retries_hello_on_the_interval(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    remote_session_port_opened(&session);
    char output[256];
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strncmp(output, "HELLO", 5) == 0, "first hello sent");

    remote_session_tick(&session, REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS - 1);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, output[0] == '\0', "no retry before the interval");

    remote_session_tick(&session, 1);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "HELLO version=1 peripheral=flipper-zero locked=0\n") == 0, "hello resent");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, (int)remote_session_counters(&session)->handshake_retries, "retry counted");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, remote_session_link_state(&session), "still handshaking");
}

static void the_retry_stops_once_connected(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    connect(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "");
    char output[256];
    remote_session_tick(&session, REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS * 3);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, output[0] == '\0', "no retry once connected");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)remote_session_counters(&session)->handshake_retries, "no retries counted");
}

static void a_retry_does_not_pile_up_hellos(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    remote_session_port_opened(&session);
    remote_session_tick(&session, REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS);
    remote_session_tick(&session, REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS);
    char output[512];
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "HELLO version=1 peripheral=flipper-zero locked=0\n") == 0, "exactly one hello queued");
}

static void the_callbacks_are_optional_and_the_polled_state_is_always_there(RemoteTestReport* report) {
    /* A device that renders by polling supplies no callbacks; the session
     * must not require them. */
    FakeDevice device;
    pico_shaped(&device);
    RemoteSessionDevice surface = surface_for(&device, "stopbath-pico");
    surface.record_received = NULL;
    surface.link_changed = NULL;
    RemoteSession session;
    remote_session_initialise(&session, surface);
    connect(&session, RemoteProtocolStatusGuestConnected, RemoteProtocolPageGuest, "HTTP://192.168.72.1/");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionConnected, remote_session_link_state(&session), "connected without callbacks");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteProtocolPageGuest, (int)remote_session_current_record(&session)->fields[RemoteProtocolDisplayFieldPage].integer, "the record is readable");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, device.records_received, "and nothing was called");
}

static void the_session_never_allocates(RemoteTestReport* report) {
    FakeDevice device;
    flipper_shaped(&device);
    int allocations_before = remote_test_allocation_count;
    RemoteSession session;
    remote_session_initialise(&session, surface_for(&device, "flipper-zero"));
    connect(&session, RemoteProtocolStatusPresenting, RemoteProtocolPageWifi, "WIFI:T:WPA;S:a;P:b;;");
    remote_session_report_input(&session, FakeInputRightShort);
    char output[512];
    take_output(&session, output, sizeof(output));
    receive_display(&session, RemoteProtocolStatusGuestConnected, RemoteProtocolPageGuest, "HTTP://192.168.72.1/", 4, RemoteProtocolErrorNone);
    remote_session_lock_changed(&session);
    remote_session_tick(&session, REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS);
    remote_session_port_closed(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, allocations_before, remote_test_allocation_count, "no allocation");
}

int main(void) {
    static const RemoteTestCase test_cases[] = {
        {"a fresh session is down and silent", a_fresh_session_is_down_and_silent},
        {"opening the port sends hello and begins handshake", opening_the_port_sends_hello_and_begins_handshake},
        {"hello carries the injected token and lock value", hello_carries_the_injected_token_and_lock_value},
        {"the first display completes the handshake and reaches the device", the_first_display_completes_the_handshake_and_reaches_the_device},
        {"a bad version answer marks the link incompatible", a_bad_version_answer_marks_the_link_incompatible},
        {"a later record replaces the previous one wholly", a_later_record_replaces_the_previous_one_wholly},
        {"a disconnect mid message discards the partial line", a_disconnect_mid_message_discards_the_partial_line},
        {"closing the port discards everything and clears the payload", closing_the_port_discards_everything_and_clears_the_payload},
        {"a press is sent only while connected and the guard allows", a_press_is_sent_only_while_connected_and_the_guard_allows},
        {"every reportable input is sent as the event the device maps it to", every_reportable_input_is_sent_as_the_event_the_device_maps_it_to},
        {"an input the device does not report sends nothing and is not an error", an_input_the_device_does_not_report_sends_nothing_and_is_not_an_error},
        {"a press is not queued across a disconnection", a_press_is_not_queued_across_a_disconnection},
        {"lock changes send state only while connected", lock_changes_send_state_only_while_connected},
        {"malformed and unexpected verbs are counted and leave the record alone", malformed_and_unexpected_verbs_are_counted_and_leave_the_record_alone},
        {"the output queue is bounded and an overflowing press is dropped and counted", the_output_queue_is_bounded_and_an_overflowing_press_is_dropped_and_counted},
        {"an invalid token is replaced with a safe default", an_invalid_token_is_replaced_with_a_safe_default},
        {"a stalled handshake retries hello on the interval", a_stalled_handshake_retries_hello_on_the_interval},
        {"the retry stops once connected", the_retry_stops_once_connected},
        {"a retry does not pile up hellos", a_retry_does_not_pile_up_hellos},
        {"the callbacks are optional and the polled state is always there", the_callbacks_are_optional_and_the_polled_state_is_always_there},
        {"the session never allocates", the_session_never_allocates},
    };
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
