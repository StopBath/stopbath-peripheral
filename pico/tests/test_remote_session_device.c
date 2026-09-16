/*
 * This device's side of the shared session (peripheral spec SE5). The link
 * behaviour itself (handshake, retry, replacement, drop on disconnect, the
 * bounded queue) is proven once under ../../shared/tests/test_remote_session.c
 * against fake devices; what is proven here is what the Pico injects and
 * what it draws: HELLO carries the fixed token and no lock; each press maps
 * to its wire event with both guard flags true, including the Key1 choice by
 * page; Key1 long is reserved; the Key1 mapping can produce nothing but a
 * page event for any page value (the property the peripheral spec asks to be
 * tested under pico/); the display shows the record while connected, a link
 * screen otherwise, and carries the session's counters; every error code is
 * shown by name inside the band. These are the KE3 cases that named the
 * display or this device's keys, kept.
 */
#include "../../shared/tests/test_support.h"

#include "../remote_display/remote_display_layout.h"
#include "../session_device/remote_session_device.h"

static const char* const READY_RECORD = "DISPLAY status=READY page=NONE payload= delivered=0 error=NONE\n";
static const char* const WIFI_RECORD =
    "DISPLAY status=PRESENTING page=WIFI payload=WIFI%3AT%3AWPA%3BS%3Afix%3BP%3Apass%3B%3B delivered=2 error=NONE\n";
static const char* const GUEST_RECORD =
    "DISPLAY status=GUEST_CONNECTED page=GUEST payload=HTTP%3A%2F%2F192.168.72.1%2F delivered=3 error=NONE\n";
static const char* const BAD_VERSION_RECORD = "DISPLAY status=READY page=NONE payload= delivered=0 error=BAD_VERSION\n";
static const char* const GUARD_ERROR_RECORD = "DISPLAY status=READY page=NONE payload= delivered=0 error=GUARD\n";

static void feed(RemoteSession* session, const char* text) {
    remote_session_receive(session, (const uint8_t*)text, strlen(text));
}

/* Drains the output into a terminated string the assertions can read. */
static size_t drain(RemoteSession* session, char* destination, size_t capacity) {
    size_t taken = remote_session_take_output(session, (uint8_t*)destination, capacity - 1);
    destination[taken] = '\0';
    return taken;
}

static RemoteSession connected_session(const char* record) {
    RemoteSession session;
    remote_session_initialise(&session, pico_session_device());
    remote_session_port_opened(&session);
    char discard[512];
    drain(&session, discard, sizeof(discard));
    feed(&session, record);
    return session;
}

static void opening_the_port_sends_hello_with_the_fixed_token_and_no_lock(RemoteTestReport* report) {
    RemoteSession session;
    remote_session_initialise(&session, pico_session_device());
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionLinkDown, remote_session_link_state(&session), "starts down");
    remote_session_port_opened(&session);
    char output[512];
    drain(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "HELLO version=1 peripheral=stopbath-pico locked=0\n") == 0, output);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, remote_session_link_state(&session), "handshaking");
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, !display_state.link_connected && display_state.link_connecting, "connecting screen");
}

static void the_first_display_record_is_the_acceptance_and_is_rendered(RemoteTestReport* report) {
    RemoteSession session = connected_session(WIFI_RECORD);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionConnected, remote_session_link_state(&session), "connected");
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, display_state.link_connected, "link up");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteDisplayStatusPresenting, display_state.status, "status");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteDisplayPageWifi, display_state.page, "page");
    REMOTE_TEST_ASSERT(report, strcmp(display_state.payload, "WIFI:T:WPA;S:fix;P:pass;;") == 0, "payload decoded");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 2, display_state.delivered_count, "delivered");
    REMOTE_TEST_ASSERT(report, display_state.error_code[0] == '\0', "no error");
}


static void an_error_code_is_shown_as_its_wire_name(RemoteTestReport* report) {
    RemoteSession session = connected_session(READY_RECORD);
    feed(&session, GUARD_ERROR_RECORD);
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, strcmp(display_state.error_code, "GUARD") == 0, display_state.error_code);
    feed(&session, READY_RECORD);
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, display_state.error_code[0] == '\0', "cleared by the next record");
}


static void closing_the_port_discards_everything_and_shows_not_connected(RemoteTestReport* report) {
    RemoteSession session = connected_session(WIFI_RECORD);
    remote_session_report_press(&session, RemoteInputKey0, RemoteInputPressShort);
    remote_session_port_closed(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionLinkDown, remote_session_link_state(&session), "down");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, remote_session_counters(&session)->reconnections, "counted as a reconnection");
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, !display_state.link_connected, "not connected screen");
    REMOTE_TEST_ASSERT(report, display_state.payload[0] == '\0', "the passphrase is gone");
    char output[512];
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, drain(&session, output, sizeof(output)), "the unsent press was dropped, not kept");

    /* Reopening starts a fresh handshake and nothing from before survives. */
    remote_session_port_opened(&session);
    drain(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strncmp(output, "HELLO ", 6) == 0, "only a fresh HELLO");
    REMOTE_TEST_ASSERT(report, strstr(output, "BUTTON") == NULL, "no replayed press");
}



typedef struct {
    RemoteInputKey key;
    RemoteInputPressKind press_kind;
    const char* record_before;
    const char* expected_line;
    const char* description;
} PressRow;

static const PressRow press_rows[] = {
    {RemoteInputKey0, RemoteInputPressShort, READY_RECORD, "BUTTON event=CENTER_SHORT foregrounded=1 unlocked=1\n", "key0 short is CENTER_SHORT"},
    {RemoteInputKey0, RemoteInputPressLong, WIFI_RECORD, "BUTTON event=CENTER_LONG foregrounded=1 unlocked=1\n", "key0 long is CENTER_LONG"},
    {RemoteInputKey1, RemoteInputPressShort, WIFI_RECORD, "BUTTON event=RIGHT_SHORT foregrounded=1 unlocked=1\n", "key1 on the wifi page asks for the guest page"},
    {RemoteInputKey1, RemoteInputPressShort, GUEST_RECORD, "BUTTON event=LEFT_SHORT foregrounded=1 unlocked=1\n", "key1 on the guest page asks for the wifi page"},
    {RemoteInputKey1, RemoteInputPressShort, READY_RECORD, "BUTTON event=RIGHT_SHORT foregrounded=1 unlocked=1\n", "key1 with no page sends RIGHT_SHORT, which the appliance ignores outside a session"},
};

static void every_press_encodes_its_event_with_both_guard_flags_true(RemoteTestReport* report) {
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(press_rows); row_index++) {
        const PressRow* row = &press_rows[row_index];
        RemoteSession session = connected_session(row->record_before);
        REMOTE_TEST_ASSERT(report, remote_session_report_press(&session, row->key, row->press_kind), row->description);
        char output[512];
        drain(&session, output, sizeof(output));
        REMOTE_TEST_ASSERT(report, strcmp(output, row->expected_line) == 0, output);
    }
}

static void key1_long_sends_nothing_and_is_not_an_error(RemoteTestReport* report) {
    RemoteSession session = connected_session(WIFI_RECORD);
    REMOTE_TEST_ASSERT(report, !remote_session_report_press(&session, RemoteInputKey1, RemoteInputPressLong), "reserved, unsent");
    char output[512];
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, drain(&session, output, sizeof(output)), "nothing on the wire");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, remote_session_counters(&session)->events_dropped_no_link, "not counted as a drop");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, remote_session_counters(&session)->events_dropped_by_output_full, "nor as a full queue");
}

/* The one function of spec 2.3: whatever the page value, including values
 * outside the enumeration, Key1 can only ever produce one of the two page
 * events, never anything destructive. */
static void the_key1_mapping_can_only_produce_a_page_event(RemoteTestReport* report) {
    for(int page_value = -8; page_value < 16; page_value++) {
        RemoteProtocolEvent event = remote_session_page_event_for_key1(page_value);
        REMOTE_TEST_ASSERT(
            report, event == RemoteProtocolEventLeftShort || event == RemoteProtocolEventRightShort,
            "only LEFT_SHORT or RIGHT_SHORT");
        if(page_value == (int)RemoteProtocolPageGuest) {
            REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteProtocolEventLeftShort, event, "guest page goes left");
        } else {
            REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteProtocolEventRightShort, event, "everything else goes right");
        }
    }
}




static void the_display_carries_the_session_counters_for_diagnostics(RemoteTestReport* report) {
    RemoteSession session;
    remote_session_initialise(&session, pico_session_device());
    remote_session_port_opened(&session);
    feed(&session, BAD_VERSION_RECORD);
    remote_session_port_closed(&session);
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, display_state.diagnostics.reconnections, "reconnections");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, display_state.diagnostics.version_mismatches, "version mismatches");
}

static void the_session_never_allocates(RemoteTestReport* report) {
    int allocations_before = remote_test_allocation_count;
    RemoteSession session = connected_session(WIFI_RECORD);
    remote_session_report_press(&session, RemoteInputKey1, RemoteInputPressShort);
    char output[512];
    drain(&session, output, sizeof(output));
    feed(&session, GUEST_RECORD);
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    remote_session_port_closed(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, allocations_before, remote_test_allocation_count, "no allocation");
}

/* Owed by KE2: the error band across every code the protocol defines. Each
 * arrives on the wire, is shown as its own name, and stays inside the band
 * (every code is at most eleven characters, the band's width). */
static void every_error_code_is_shown_by_name_inside_the_error_band(RemoteTestReport* report) {
    int error_count = 0;
    const char* const* error_names = remote_protocol_enumeration_values(RemoteProtocolEnumerationError, &error_count);
    REMOTE_TEST_ASSERT(report, error_count > 10, "the code set is present");
    RemoteSession plain = connected_session(READY_RECORD);
    RemoteDisplayState plain_state;
    remote_session_display(&plain, &plain_state);
    RemoteBitmap plain_bitmap;
    remote_display_layout_render(&plain_state, &plain_bitmap);
    RemoteLayoutRectangle band = remote_display_layout_region(RemoteLayoutRegionError);
    for(int error = 0; error < error_count; error++) {
        if(error == (int)RemoteProtocolErrorNone || error == (int)RemoteProtocolErrorBadVersion) {
            continue;
        }
        char line[160] = "DISPLAY status=READY page=NONE payload= delivered=0 error=";
        strncat(line, error_names[error], sizeof(line) - strlen(line) - 2);
        strncat(line, "\n", sizeof(line) - strlen(line) - 1);
        RemoteSession session = connected_session(READY_RECORD);
        feed(&session, line);
        RemoteDisplayState display_state;
        remote_session_display(&session, &display_state);
        REMOTE_TEST_ASSERT(report, strcmp(display_state.error_code, error_names[error]) == 0, error_names[error]);
        REMOTE_TEST_ASSERT(report, strlen(display_state.error_code) <= 11, "fits the band");
        RemoteBitmap bitmap;
        remote_display_layout_render(&display_state, &bitmap);
        for(int y = 0; y < REMOTE_BITMAP_HEIGHT; y++) {
            for(int x = 0; x < REMOTE_BITMAP_WIDTH; x++) {
                bool inside_band = x >= band.x && x < band.x + band.width && y >= band.y && y < band.y + band.height;
                if(!inside_band && remote_bitmap_get_pixel(&bitmap, x, y) != remote_bitmap_get_pixel(&plain_bitmap, x, y)) {
                    REMOTE_TEST_ASSERT(report, false, error_names[error]);
                    y = REMOTE_BITMAP_HEIGHT;
                    break;
                }
            }
        }
    }
}

static const RemoteTestCase test_cases[] = {
    {"opening the port sends hello with the fixed token and no lock", opening_the_port_sends_hello_with_the_fixed_token_and_no_lock},
    {"the first display record is the acceptance and is rendered", the_first_display_record_is_the_acceptance_and_is_rendered},
    {"an error code is shown as its wire name", an_error_code_is_shown_as_its_wire_name},
    {"closing the port discards everything and shows not connected", closing_the_port_discards_everything_and_shows_not_connected},
    {"every press encodes its event with both guard flags true", every_press_encodes_its_event_with_both_guard_flags_true},
    {"key1 long sends nothing and is not an error", key1_long_sends_nothing_and_is_not_an_error},
    {"the key1 mapping can only produce a page event", the_key1_mapping_can_only_produce_a_page_event},
    {"the display carries the session counters for diagnostics", the_display_carries_the_session_counters_for_diagnostics},
    {"the session never allocates", the_session_never_allocates},
    {"every error code is shown by name inside the error band", every_error_code_is_shown_by_name_inside_the_error_band},
};

int main(void) {
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
