/*
 * This device's side of the shared client session (peripheral spec SE5,
 * SD4): what the Pico injects into ../../shared/session/remote_session.h and
 * how it turns the session's state into its own display state.
 *
 * What lives here and nowhere under shared/: the token stopbath-pico (KD7);
 * the guard values, no lock and always foregrounded, honestly (KD5, spec
 * 2.4: single purpose firmware); the mapping from the two keys to wire
 * events, including the one choice this device makes, Key1 short by page
 * (spec 2.3, the recorded deviation, provisional until KD9), in
 * remote_session_page_event_for_key1 and nowhere else; and the composition
 * of RemoteDisplayState from the held record, the link state and the
 * session's counters. Pure logic, no SDK, tested in
 * tests/test_remote_session_device.c.
 */
#pragma once

#include <stdbool.h>

#include "../../shared/session/remote_session.h"
#include "../remote_display/remote_display_layout.h"
#include "../remote_input/remote_input_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PICO_SESSION_PERIPHERAL_TOKEN "stopbath-pico"

/* The surface this device supplies to remote_session_initialise. It reads no
 * device state (the guard values are fixed), so there is no struct to keep. */
RemoteSessionDevice pico_session_device(void);

/* A classified press from the input model, through the session. Key0 short
 * and long are CENTER_SHORT and CENTER_LONG; Key1 short is the page event
 * for the current page; Key1 long is reserved and sends nothing. Returns
 * true if a message was queued. */
bool remote_session_report_press(RemoteSession* session, RemoteInputKey key, RemoteInputPressKind press_kind);

/* The Key1 choice of spec 2.3, in one place: LEFT_SHORT when the page is
 * GUEST, RIGHT_SHORT for every other value including ones outside the
 * enumeration. Takes a plain integer so a test can pass any value. Can
 * return nothing but those two events. */
RemoteProtocolEvent remote_session_page_event_for_key1(int page);

/* Fills the display state to render: the appliance's record when connected,
 * a link screen otherwise, with the session's counters in the diagnostics.
 * show_diagnostics and the refresher's counters are left for the firmware. */
void remote_session_display(const RemoteSession* session, RemoteDisplayState* display_state);

#ifdef __cplusplus
}
#endif
