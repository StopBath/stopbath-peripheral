#include "development_peer_probe_report.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void development_peer_probe_report_begin(DevelopmentPeerProbeReport* report_state) {
    memset(report_state, 0, sizeof(*report_state));
}

/* Finds the node's state, creating it if there is room. A name beyond the
 * table's capacity gets no state and therefore no message, which is the
 * refuse rather than truncate rule applied to the table. */
static DevelopmentPeerProbeNodeState* node_state_for(DevelopmentPeerProbeReport* report_state, const char* node_name) {
    if(strlen(node_name) >= DEVELOPMENT_PEER_PROBE_NODE_NAME_CAPACITY) {
        return NULL;
    }
    for(int index = 0; index < report_state->node_count; index++) {
        if(strcmp(report_state->nodes[index].node_name, node_name) == 0) {
            return &report_state->nodes[index];
        }
    }
    if(report_state->node_count >= DEVELOPMENT_PEER_PROBE_NODE_COUNT) {
        return NULL;
    }
    DevelopmentPeerProbeNodeState* created = &report_state->nodes[report_state->node_count++];
    memset(created, 0, sizeof(*created));
    strcpy(created->node_name, node_name);
    return created;
}

/* Writes the message only if it fits whole; otherwise leaves an empty
 * string and reports nothing, so a caller with a short buffer never prints
 * half a sentence. */
static size_t write_whole(char* message, size_t message_capacity, const char* format, const char* node_name) {
    if(message_capacity == 0) {
        return 0;
    }
    int written = snprintf(message, message_capacity, format, node_name);
    if(written < 0 || (size_t)written >= message_capacity) {
        message[0] = '\0';
        return 0;
    }
    return (size_t)written;
}

size_t development_peer_probe_report_observe(
    DevelopmentPeerProbeReport* report_state,
    DevelopmentPeerProbeEvent event,
    const char* node_name,
    char* message,
    size_t message_capacity) {
    if(message_capacity > 0) {
        message[0] = '\0';
    }
    if(event == DevelopmentPeerProbeEventFound) {
        development_peer_probe_report_begin(report_state);
        return 0;
    }
    DevelopmentPeerProbeNodeState* node = node_state_for(report_state, node_name);
    if(node == NULL) {
        return 0;
    }
    switch(event) {
    case DevelopmentPeerProbeEventOpenDenied:
        node->consecutive_silent_opens = 0;
        if(node->denial_reported) {
            return 0;
        }
        node->denial_reported = true;
        return write_whole(
            message, message_capacity,
            "cannot open %s: permission denied; retrying, udev may not have applied the group to the new node yet",
            node_name);
    case DevelopmentPeerProbeEventOpenedSilent:
        node->consecutive_silent_opens++;
        if(node->silence_reported || node->consecutive_silent_opens < DEVELOPMENT_PEER_PROBE_SILENT_PASSES_BEFORE_REPORT) {
            return 0;
        }
        node->silence_reported = true;
        return write_whole(
            message, message_capacity,
            "%s opens but sends no HELLO: another peer may hold it (ps aux | grep development_peer), or the application is not running on the device",
            node_name);
    case DevelopmentPeerProbeEventAbsent:
        /* The node went away: whatever comes back on this name is a fresh
         * enumeration and earns a fresh count. */
        node->consecutive_silent_opens = 0;
        node->silence_reported = false;
        return 0;
    case DevelopmentPeerProbeEventFound:
        return 0;
    }
    return 0;
}
