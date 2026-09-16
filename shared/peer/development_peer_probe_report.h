/*
 * What the development peer says while it probes serial nodes for the
 * application's channel. The shell owns the opening and reading; this module
 * owns the decision of when a fact is worth a line, so that a correct retry
 * does not read as a failure (a denied open on a node udev has not yet
 * handed to the user's group, reported once and then retried in silence) and
 * a node that opens but never speaks is not passed over in silence (another
 * peer holding it, or the application not running). Found on the Flipper's
 * SE4 gate run, 2026-09-16, peripheral evaluation log.
 *
 * No I/O, no allocation, no dependency beyond the C standard library, so it
 * is proven on the host by tests/test_development_peer_probe_report.c.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* open() failed with a permission error; the node exists. */
    DevelopmentPeerProbeEventOpenDenied,
    /* The node opened but sent no HELLO within the probe's window. */
    DevelopmentPeerProbeEventOpenedSilent,
    /* The node does not exist on this pass. */
    DevelopmentPeerProbeEventAbsent,
    /* The node sent a HELLO: the search is over. */
    DevelopmentPeerProbeEventFound,
} DevelopmentPeerProbeEvent;

/* The most nodes the shell probes, /dev/ttyACM0 to /dev/ttyACM15, and the
 * longest such name with its terminator. Both bound the state below. */
#define DEVELOPMENT_PEER_PROBE_NODE_COUNT 16
#define DEVELOPMENT_PEER_PROBE_NODE_NAME_CAPACITY 32

/* Every message this module produces fits a buffer of this size; a smaller
 * buffer gets nothing rather than a cut message. */
#define DEVELOPMENT_PEER_PROBE_MESSAGE_CAPACITY 192

/* A silent node is reported on this many consecutive silent opens. Fewer
 * would name a node the application is still enumerating on; the probe's
 * window is a second per pass. */
#define DEVELOPMENT_PEER_PROBE_SILENT_PASSES_BEFORE_REPORT 3

typedef struct {
    char node_name[DEVELOPMENT_PEER_PROBE_NODE_NAME_CAPACITY];
    bool denial_reported;
    bool silence_reported;
    int consecutive_silent_opens;
} DevelopmentPeerProbeNodeState;

typedef struct {
    DevelopmentPeerProbeNodeState nodes[DEVELOPMENT_PEER_PROBE_NODE_COUNT];
    int node_count;
} DevelopmentPeerProbeReport;

/* Starts a search: nothing has been said about any node. */
void development_peer_probe_report_begin(DevelopmentPeerProbeReport* report_state);

/* Records one event for one node and writes what, if anything, the shell
 * should print into message (terminated). Returns the message length, zero
 * when there is nothing to say or the buffer cannot hold it. A find resets
 * the search so the next one reports afresh. */
size_t development_peer_probe_report_observe(
    DevelopmentPeerProbeReport* report_state,
    DevelopmentPeerProbeEvent event,
    const char* node_name,
    char* message,
    size_t message_capacity);

#ifdef __cplusplus
}
#endif
