#include "remote_session_device.h"

#include <stdio.h>

static bool flipper_screen_locked(void* device_context) {
    const FlipperSessionDevice* device = device_context;
    return device->screen_locked;
}

static bool flipper_foregrounded(void* device_context) {
    const FlipperSessionDevice* device = device_context;
    return device->foregrounded;
}

/* The five button events this device reports, each to its own wire name;
 * the page is not consulted, because every page move here is its own
 * button (specification 2.3). What any of them means is the appliance's. */
static bool flipper_event_for_input(void* device_context, int input, int current_page, RemoteProtocolEvent* wire_event) {
    (void)device_context;
    (void)current_page;
    switch((RemoteReportableEvent)input) {
    case RemoteReportableEventCenterShort:
        *wire_event = RemoteProtocolEventCenterShort;
        return true;
    case RemoteReportableEventCenterLong:
        *wire_event = RemoteProtocolEventCenterLong;
        return true;
    case RemoteReportableEventLeftShort:
        *wire_event = RemoteProtocolEventLeftShort;
        return true;
    case RemoteReportableEventRightShort:
        *wire_event = RemoteProtocolEventRightShort;
        return true;
    case RemoteReportableEventCount:
    default:
        return false;
    }
}

RemoteSessionDevice flipper_session_device_initialise(FlipperSessionDevice* device) {
    device->screen_locked = false;
    device->foregrounded = true;
    RemoteSessionDevice surface = {
        .peripheral_token = FLIPPER_SESSION_PERIPHERAL_TOKEN,
        .device_context = device,
        .screen_locked = flipper_screen_locked,
        .foregrounded = flipper_foregrounded,
        .event_for_input = flipper_event_for_input,
        /* This device composes its screen by polling on every loop, so it
         * takes no callbacks. */
        .record_received = NULL,
        .link_changed = NULL,
    };
    return surface;
}

bool flipper_session_device_report(RemoteSession* session, RemoteReportableEvent event) {
    return remote_session_report_input(session, (int)event);
}

void flipper_session_device_lock_changed(RemoteSession* session, FlipperSessionDevice* device, bool locked) {
    device->screen_locked = locked;
    remote_session_lock_changed(session);
}

void remote_session_display(const RemoteSession* session, const FlipperSessionDevice* device, RemoteDisplayState* display_state) {
    remote_display_state_initialise(display_state);
    display_state->screen_locked = device->screen_locked;

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
        snprintf(display_state->payload, sizeof(display_state->payload), "%s", record->fields[RemoteProtocolDisplayFieldPayload].text);
        display_state->delivered_count = record->fields[RemoteProtocolDisplayFieldDelivered].integer;
        /* The error field maps to the display's error code text through the
         * protocol enumeration's wire names, so the code shown is the code
         * sent. NONE leaves the band off. */
        uint32_t error = record->fields[RemoteProtocolDisplayFieldError].integer;
        if(error != RemoteProtocolErrorNone) {
            int error_count = 0;
            const char* const* error_names = remote_protocol_enumeration_values(RemoteProtocolEnumerationError, &error_count);
            if((int)error < error_count) {
                snprintf(display_state->error_code, sizeof(display_state->error_code), "%s", error_names[error]);
            }
        }
        break;
    }
    }
}
