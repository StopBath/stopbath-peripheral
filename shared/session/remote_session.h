/*
 * The client session shared by every peripheral (peripheral spec SE5): the
 * state machine between a device's transport edge, its input and its
 * display. Pure logic, no SDK, no allocation, so the whole of it is proven on
 * a development machine against a table of fake devices
 * (tests/test_remote_session.c).
 *
 * What it owns, and the rules it enforces (Flipper 2.4, 2.5, 2.10, Part 5;
 * Pico 2.2 to 2.4):
 *
 * - Handshake with retry: when the host opens the port it sends HELLO with
 *   the protocol version, the device's token and the device's lock value,
 *   and waits. The first DISPLAY the appliance answers with is the
 *   acceptance; a DISPLAY carrying BAD_VERSION is a rejection and the link
 *   is incompatible. A lost handshake is retried on an interval, never
 *   piling up HELLOs.
 * - Wholesale replacement: it holds no authoritative state. Each DISPLAY
 *   replaces the last one wholly and is handed to the device. On a link
 *   drop it discards everything, including the payload that may carry a
 *   Wi-Fi passphrase, and nothing queued survives the drop.
 * - A press is sent only while connected, and only when the device's guard
 *   allows (foregrounded and unlocked, as the device reports them); a press
 *   that cannot be sent now is dropped and counted, never queued, so a
 *   stale press cannot cross a reconnection and end a later session.
 * - The outbound queue is bounded and overflow is counted; malformed input
 *   is counted and leaves the record alone.
 *
 * What it does not own (peripheral spec 0.17): what any button means, which
 * is the appliance's; and the device's own choices, which arrive through the
 * RemoteSessionDevice surface below and live under the device's directory:
 * its token, whether it has a lock and a foreground, and which wire event a
 * physical input maps to (the Pico's Key1 choice by page is one such).
 * Nothing here knows a display: the device reads the held record and the
 * link state, or takes them from the callbacks, and draws what it likes.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../protocol/remote_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* How many outbound messages the session holds before dropping the next and
 * counting it. A peripheral implementation constant, not a protocol bound:
 * it bounds the peripheral's own input path and the appliance never sees or
 * honours it (removed from the definition at FD20). A press that would
 * exceed it is dropped, never queued across a disconnection. */
#define REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH 4

/* How long to wait for the appliance's DISPLAY acceptance before re-sending
 * HELLO. The appliance treats a second HELLO as a restart and resends the
 * record (seam 4.2), so a periodic retry recovers a handshake that would
 * otherwise hang. Well under the inbound rate bound. */
#define REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS 2000

typedef enum {
    /* The host has not opened the port: no cable, or the appliance has not
     * opened it. Nothing is sent. */
    RemoteSessionLinkDown,
    /* The port is open and HELLO has been sent; awaiting the first DISPLAY. */
    RemoteSessionHandshaking,
    /* A DISPLAY has been received; the record is the appliance's. */
    RemoteSessionConnected,
    /* The appliance answered HELLO with BAD_VERSION. */
    RemoteSessionIncompatible,
} RemoteSessionLinkState;

/*
 * The device surface (SD4): supplied once at initialisation, never copied
 * beyond the struct itself. The three functions the session asks are
 * required; the two callbacks are optional, for a device that renders on
 * change rather than by polling remote_session_current_record.
 */
typedef struct {
    /* The token sent in HELLO. Copied; an invalid one (not lower case
     * letters, digits and hyphens, 1 to the protocol's bound) is replaced
     * with a safe default so HELLO can always be sent. */
    const char* peripheral_token;
    /* Passed back to every function below. */
    void* device_context;
    /* The lock value: HELLO's and STATE's locked field, and the guard. A
     * device with no lock returns false, honestly. */
    bool (*screen_locked)(void* device_context);
    /* The foreground value for the guard. A single purpose device returns
     * true, honestly. */
    bool (*foregrounded)(void* device_context);
    /* Maps a physical input, in the device's own numbering, to the wire
     * event it reports, given the page of the record currently held (so a
     * device with one page key may choose by it). Returns false for an
     * input the device does not report, which is not an error. */
    bool (*event_for_input)(void* device_context, int input, int current_page, RemoteProtocolEvent* wire_event);
    /* Optional: each accepted DISPLAY record, after it has replaced the
     * held one. */
    void (*record_received)(void* device_context, const RemoteProtocolMessage* record);
    /* Optional: every change of link state. */
    void (*link_changed)(void* device_context, RemoteSessionLinkState link_state);
} RemoteSessionDevice;

/* Diagnostics for the view Flipper 2.11 asks for. Every drop is counted
 * under the reason it was dropped. */
typedef struct {
    uint32_t reconnections;
    uint32_t malformed_received;
    uint32_t version_mismatches;
    uint32_t events_dropped_no_link;
    uint32_t events_dropped_by_guard;
    uint32_t events_dropped_by_output_full;
    uint32_t handshake_retries;
} RemoteSessionCounters;

typedef struct {
    RemoteSessionDevice device;
    RemoteSessionLinkState link_state;
    /* The inbound line assembler, reset whenever the link drops so no partial
     * line survives a reconnection. */
    RemoteProtocolLineAssembler inbound;
    /* The device's token as validated. */
    char peripheral_token[REMOTE_PROTOCOL_MAX_TEXT_LENGTH + 1];
    /* The last DISPLAY received, held only while connected; a neutral READY
     * record otherwise, so a device reading it never sees stale content. */
    RemoteProtocolMessage current_display;
    /* Bytes waiting to be sent by the transport. Bounded; a press that would
     * overflow it is dropped and counted, never queued unboundedly. */
    uint8_t output[REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH * REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH];
    size_t output_length;
    /* Whole messages queued: the depth is a message count, not a byte count. */
    int output_message_count;
    /* Milliseconds spent in the handshake since the last HELLO. */
    uint32_t handshake_elapsed_milliseconds;
    RemoteSessionCounters counters;
} RemoteSession;

void remote_session_initialise(RemoteSession* session, RemoteSessionDevice device);

/* The host opened the port (DTR asserted) or the peripheral restarted: send
 * HELLO and begin the handshake. Discards any prior link state first. */
void remote_session_port_opened(RemoteSession* session);

/* The host closed the port, the cable was pulled, or USB suspended: discard
 * link state, clear the sensitive payload, and drop anything unsent. */
void remote_session_port_closed(RemoteSession* session);

/* Advances the handshake retry clock by the elapsed time since the last
 * call. While handshaking, re-sends HELLO once the interval passes. A no-op
 * in every other link state. The session keeps no clock of its own. */
void remote_session_tick(RemoteSession* session, uint32_t elapsed_milliseconds);

/* Feeds bytes received from the appliance. Complete DISPLAY records replace
 * the held one and reach the device; a BAD_VERSION answer marks the link
 * incompatible; malformed input, and any verb that is not DISPLAY, is
 * counted and changes nothing. */
void remote_session_receive(RemoteSession* session, const uint8_t* bytes, size_t byte_count);

/* A physical input, in the device's own numbering. Asks the device which
 * wire event it is (none: nothing happens, nothing is counted), then sends
 * BUTTON only while connected and only when the device's guard allows,
 * counting every drop under its reason. Returns true if a message was
 * queued. */
bool remote_session_report_input(RemoteSession* session, int input);

/* The device's lock changed. Sends STATE with the device's current value
 * while connected; otherwise nothing, since there is no link to report on. */
void remote_session_lock_changed(RemoteSession* session);

/* Drains bytes the transport should send, clearing them. Returns the count. */
size_t remote_session_take_output(RemoteSession* session, uint8_t* destination, size_t destination_capacity);

RemoteSessionLinkState remote_session_link_state(const RemoteSession* session);

/* The record to render while connected; the neutral record otherwise. */
const RemoteProtocolMessage* remote_session_current_record(const RemoteSession* session);

const RemoteSessionCounters* remote_session_counters(const RemoteSession* session);

#ifdef __cplusplus
}
#endif
