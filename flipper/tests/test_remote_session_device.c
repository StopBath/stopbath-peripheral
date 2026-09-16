/*
 * This device's side of the shared session (peripheral spec SE5). The link
 * behaviour itself (handshake, retry, replacement, drop on disconnect, the
 * bounded queue) is proven once under ../../shared/tests/test_remote_session.c
 * against fake devices; what is proven here is what the Flipper injects and
 * what it draws: HELLO carries this device's token and its real lock; a
 * locked or backgrounded device is dropped by the guard the session asks it
 * about; each reportable event maps to its wire name; the display shows the
 * record while connected, a link screen otherwise, and the lock overlaid.
 * These are the FE4 cases that named the display or the lock, kept.
 */
#include "../../shared/tests/test_support.h"

#include "../session_device/remote_session_device.h"

#include <stdio.h>
#include <string.h>

static void receive_display(RemoteSession* session, RemoteProtocolStatus status, RemoteProtocolPage page, const char* payload, uint32_t delivered, RemoteProtocolError error) {
    RemoteProtocolMessage record;
    remote_protocol_message_initialise(&record, RemoteProtocolVerbDisplay);
    remote_protocol_message_set_integer(&record, RemoteProtocolDisplayFieldStatus, status);
    remote_protocol_message_set_integer(&record, RemoteProtocolDisplayFieldPage, page);
    remote_protocol_message_set_text(&record, RemoteProtocolDisplayFieldPayload, payload);
    remote_protocol_message_set_integer(&record, RemoteProtocolDisplayFieldDelivered, delivered);
    remote_protocol_message_set_integer(&record, RemoteProtocolDisplayFieldError, error);
    uint8_t line[REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH];
    size_t length = 0;
    remote_protocol_encode(&record, (char*)line, sizeof(line), &length);
    remote_session_receive(session, line, length);
}

static void take_output(RemoteSession* session, char* destination, size_t capacity) {
    size_t length = remote_session_take_output(session, (uint8_t*)destination, capacity - 1);
    destination[length] = '\0';
}

static void a_fresh_session_shows_not_connected(RemoteTestReport* report) {
    FlipperSessionDevice device;
    RemoteSession session;
    remote_session_initialise(&session, flipper_session_device_initialise(&device));
    RemoteDisplayState display_state;
    remote_session_display(&session, &device, &display_state);
    REMOTE_TEST_ASSERT(report, !display_state.link_connected, "shows not connected");
    REMOTE_TEST_ASSERT(report, !display_state.screen_locked, "unlocked");
}

static void hello_carries_this_devices_token_and_its_lock_state(RemoteTestReport* report) {
    FlipperSessionDevice device;
    RemoteSession session;
    remote_session_initialise(&session, flipper_session_device_initialise(&device));
    remote_session_port_opened(&session);
    char output[256];
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "HELLO version=1 peripheral=flipper-zero locked=0\n") == 0, "hello with the token, unlocked");

    flipper_session_device_lock_changed(&session, &device, true);
    remote_session_port_opened(&session);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "HELLO version=1 peripheral=flipper-zero locked=1\n") == 0, "locked hello");
}

static void the_display_follows_the_link_state(RemoteTestReport* report) {
    FlipperSessionDevice device;
    RemoteSession session;
    remote_session_initialise(&session, flipper_session_device_initialise(&device));
    RemoteDisplayState display_state;
    char output[256];

    remote_session_port_opened(&session);
    take_output(&session, output, sizeof(output));
    remote_session_display(&session, &device, &display_state);
    REMOTE_TEST_ASSERT(report, !display_state.link_connected && display_state.link_connecting, "connecting");

    receive_display(&session, RemoteProtocolStatusPresenting, RemoteProtocolPageWifi, "WIFI:T:WPA;S:a;P:b;;", 2, RemoteProtocolErrorNone);
    remote_session_display(&session, &device, &display_state);
    REMOTE_TEST_ASSERT(report, display_state.link_connected, "connected display");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteDisplayStatusPresenting, display_state.status, "status rendered");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteDisplayPageWifi, display_state.page, "page rendered");
    REMOTE_TEST_ASSERT(report, strcmp(display_state.payload, "WIFI:T:WPA;S:a;P:b;;") == 0, "payload rendered");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 2, display_state.delivered_count, "count rendered");

    remote_session_port_closed(&session);
    remote_session_display(&session, &device, &display_state);
    REMOTE_TEST_ASSERT(report, !display_state.link_connected, "not connected after a drop");
    REMOTE_TEST_ASSERT(report, display_state.payload[0] == '\0', "the passphrase is gone from the screen");

    remote_session_port_opened(&session);
    take_output(&session, output, sizeof(output));
    receive_display(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "", 0, RemoteProtocolErrorBadVersion);
    remote_session_display(&session, &device, &display_state);
    REMOTE_TEST_ASSERT(report, display_state.link_incompatible, "incompatible display");
}

static void an_error_code_is_shown_as_its_wire_name(RemoteTestReport* report) {
    FlipperSessionDevice device;
    RemoteSession session;
    remote_session_initialise(&session, flipper_session_device_initialise(&device));
    remote_session_port_opened(&session);
    char output[256];
    take_output(&session, output, sizeof(output));
    receive_display(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "", 0, RemoteProtocolErrorNone);
    RemoteDisplayState display_state;
    remote_session_display(&session, &device, &display_state);
    REMOTE_TEST_ASSERT(report, display_state.error_code[0] == '\0', "no band for NONE");
    receive_display(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "", 0, RemoteProtocolErrorTooLong);
    remote_session_display(&session, &device, &display_state);
    REMOTE_TEST_ASSERT(report, strcmp(display_state.error_code, "TOO_LONG") == 0, "the wire name");
}

static void a_button_is_transmitted_only_while_connected_and_foregrounded_and_unlocked(RemoteTestReport* report) {
    FlipperSessionDevice device;
    RemoteSession session;
    remote_session_initialise(&session, flipper_session_device_initialise(&device));
    char output[256];

    REMOTE_TEST_ASSERT(report, !flipper_session_device_report(&session, RemoteReportableEventCenterShort), "not while down");
    remote_session_port_opened(&session);
    take_output(&session, output, sizeof(output));
    receive_display(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "", 0, RemoteProtocolErrorNone);
    REMOTE_TEST_ASSERT(report, flipper_session_device_report(&session, RemoteReportableEventCenterLong), "sent");
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "BUTTON event=CENTER_LONG foregrounded=1 unlocked=1\n") == 0, "both flags true");

    device.foregrounded = false;
    REMOTE_TEST_ASSERT(report, !flipper_session_device_report(&session, RemoteReportableEventCenterShort), "not while backgrounded");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, (int)remote_session_counters(&session)->events_dropped_by_guard, "guard drop counted");
    device.foregrounded = true;

    flipper_session_device_lock_changed(&session, &device, true);
    take_output(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "STATE locked=1\n") == 0, "the lock was reported");
    REMOTE_TEST_ASSERT(report, !flipper_session_device_report(&session, RemoteReportableEventCenterShort), "not while locked");
    RemoteDisplayState display_state;
    remote_session_display(&session, &device, &display_state);
    REMOTE_TEST_ASSERT(report, display_state.screen_locked, "display shows locked");
}

static void every_reportable_event_maps_to_its_wire_name(RemoteTestReport* report) {
    typedef struct {
        RemoteReportableEvent event;
        const char* wire;
    } EventRow;
    static const EventRow rows[] = {
        {RemoteReportableEventCenterShort, "CENTER_SHORT"},
        {RemoteReportableEventCenterLong, "CENTER_LONG"},
        {RemoteReportableEventLeftShort, "LEFT_SHORT"},
        {RemoteReportableEventRightShort, "RIGHT_SHORT"},
    };
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(rows); row_index++) {
        FlipperSessionDevice device;
        RemoteSession session;
        remote_session_initialise(&session, flipper_session_device_initialise(&device));
        remote_session_port_opened(&session);
        char output[256];
        take_output(&session, output, sizeof(output));
        receive_display(&session, RemoteProtocolStatusReady, RemoteProtocolPageNone, "", 0, RemoteProtocolErrorNone);
        REMOTE_TEST_ASSERT(report, flipper_session_device_report(&session, rows[row_index].event), rows[row_index].wire);
        take_output(&session, output, sizeof(output));
        char expected[64];
        snprintf(expected, sizeof(expected), "BUTTON event=%s foregrounded=1 unlocked=1\n", rows[row_index].wire);
        REMOTE_TEST_ASSERT(report, strcmp(output, expected) == 0, rows[row_index].wire);
    }
    /* The count sentinel is not an event and is not reported. */
    FlipperSessionDevice device;
    RemoteSession session;
    remote_session_initialise(&session, flipper_session_device_initialise(&device));
    REMOTE_TEST_ASSERT(report, !flipper_session_device_report(&session, RemoteReportableEventCount), "the sentinel is nothing");
}

static void composing_the_display_never_allocates(RemoteTestReport* report) {
    FlipperSessionDevice device;
    RemoteSession session;
    remote_session_initialise(&session, flipper_session_device_initialise(&device));
    remote_session_port_opened(&session);
    char output[256];
    take_output(&session, output, sizeof(output));
    receive_display(&session, RemoteProtocolStatusGuestConnected, RemoteProtocolPageGuest, "HTTP://192.168.72.1/", 3, RemoteProtocolErrorNone);
    int allocations_before = remote_test_allocation_count;
    RemoteDisplayState display_state;
    remote_session_display(&session, &device, &display_state);
    flipper_session_device_report(&session, RemoteReportableEventLeftShort);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, allocations_before, remote_test_allocation_count, "no allocation");
}

int main(void) {
    static const RemoteTestCase test_cases[] = {
        {"a fresh session shows not connected", a_fresh_session_shows_not_connected},
        {"hello carries this device's token and its lock state", hello_carries_this_devices_token_and_its_lock_state},
        {"the display follows the link state", the_display_follows_the_link_state},
        {"an error code is shown as its wire name", an_error_code_is_shown_as_its_wire_name},
        {"a button is transmitted only while connected and foregrounded and unlocked",
         a_button_is_transmitted_only_while_connected_and_foregrounded_and_unlocked},
        {"every reportable event maps to its wire name", every_reportable_event_maps_to_its_wire_name},
        {"composing the display never allocates", composing_the_display_never_allocates},
    };
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
