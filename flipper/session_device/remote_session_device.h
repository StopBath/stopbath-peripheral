/*
 * This device's side of the shared client session (peripheral spec SE5,
 * SD4): what the Flipper injects into ../../shared/session/remote_session.h
 * and how it turns the session's state into its own display state.
 *
 * What lives here and nowhere under shared/: the token "flipper-zero"; the
 * screen lock, the one fact this device owns (specification 2.4), which the
 * session reads for HELLO, STATE and the guard; the foreground value (always
 * true on this firmware, the FD19 finding recorded in stopbath_remote.c);
 * the mapping from the input model's reportable events to wire events; and
 * the composition of RemoteDisplayState from the held record, the link state
 * and the lock. Pure logic, no SDK, tested in tests/test_remote_session_device.c.
 */
#pragma once

#include <stdbool.h>

#include "../../shared/session/remote_session.h"
#include "../remote_display/remote_display_layout.h"
#include "../remote_input/remote_input_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLIPPER_SESSION_PERIPHERAL_TOKEN "flipper-zero"

/* The device state the session's surface reads. The glue keeps it current:
 * the lock from the input model, the foreground from the firmware. */
typedef struct {
    bool screen_locked;
    bool foregrounded;
} FlipperSessionDevice;

/* Starts the device state unlocked and foregrounded, and returns the surface
 * that reads it, for remote_session_initialise. The device struct must
 * outlive the session. */
RemoteSessionDevice flipper_session_device_initialise(FlipperSessionDevice* device);

/* Reports a reportable event from the input model through the session. The
 * lock has already been applied by the input model, so a locked screen never
 * produces one; the session checks the guard again regardless (2.4). */
bool flipper_session_device_report(RemoteSession* session, RemoteReportableEvent event);

/* The screen was locked or unlocked locally: records it for HELLO and the
 * guard, and has the session send STATE while connected. */
void flipper_session_device_lock_changed(RemoteSession* session, FlipperSessionDevice* device, bool locked);

/* Fills the display state to render: the appliance's record when connected,
 * a link screen otherwise, with the local lock overlaid. nfc_presenting is
 * left false for the glue to set. */
void remote_session_display(const RemoteSession* session, const FlipperSessionDevice* device, RemoteDisplayState* display_state);

#ifdef __cplusplus
}
#endif
