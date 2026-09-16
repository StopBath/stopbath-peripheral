/*
 * What the peer's probe says while it searches for the application's serial
 * node (peripheral evaluation log, SE4, the two peer findings). The shell
 * feeds it one event per candidate node per pass and prints whatever it is
 * told to; the decisions are here, off the operating system, so they can be
 * proven: a denied open is reported once per node per search and then the
 * retry is silent; a node that opens but sends no HELLO is named after a few
 * passes, once, with the two likely causes; a find says nothing (the shell
 * has its own line for that) and resets everything for the next search.
 */
#include "test_support.h"

#include "../peer/development_peer_probe_report.h"

#include <string.h>

static size_t observe(DevelopmentPeerProbeReport* report_state, DevelopmentPeerProbeEvent event, const char* node, char* message, size_t capacity) {
    memset(message, 'x', capacity);
    return development_peer_probe_report_observe(report_state, event, node, message, capacity);
}

static void a_denied_open_is_reported_once_per_node_per_search_and_the_retry_is_silent(RemoteTestReport* report) {
    DevelopmentPeerProbeReport report_state;
    development_peer_probe_report_begin(&report_state);
    char message[DEVELOPMENT_PEER_PROBE_MESSAGE_CAPACITY];

    size_t length = observe(&report_state, DevelopmentPeerProbeEventOpenDenied, "/dev/ttyACM0", message, sizeof(message));
    REMOTE_TEST_ASSERT(report, length > 0, "the first denial is reported");
    REMOTE_TEST_ASSERT(report, strstr(message, "/dev/ttyACM0") != NULL, "the node is named");
    REMOTE_TEST_ASSERT(report, strstr(message, "retrying") != NULL, "the retry is announced");
    REMOTE_TEST_ASSERT(report, strstr(message, "udev") != NULL, "the likely cause is named");

    for(int pass = 0; pass < 5; pass++) {
        length = observe(&report_state, DevelopmentPeerProbeEventOpenDenied, "/dev/ttyACM0", message, sizeof(message));
        REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)length, "later denials of the same node say nothing");
    }
}

static void nodes_are_reported_independently(RemoteTestReport* report) {
    DevelopmentPeerProbeReport report_state;
    development_peer_probe_report_begin(&report_state);
    char message[DEVELOPMENT_PEER_PROBE_MESSAGE_CAPACITY];

    REMOTE_TEST_ASSERT(report, observe(&report_state, DevelopmentPeerProbeEventOpenDenied, "/dev/ttyACM0", message, sizeof(message)) > 0, "ACM0 reported");
    size_t length = observe(&report_state, DevelopmentPeerProbeEventOpenDenied, "/dev/ttyACM1", message, sizeof(message));
    REMOTE_TEST_ASSERT(report, length > 0, "ACM1 reported on its own first denial");
    REMOTE_TEST_ASSERT(report, strstr(message, "/dev/ttyACM1") != NULL, "and named");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)observe(&report_state, DevelopmentPeerProbeEventOpenDenied, "/dev/ttyACM1", message, sizeof(message)), "then silent");
}

static void a_silent_node_is_named_after_three_passes_once(RemoteTestReport* report) {
    DevelopmentPeerProbeReport report_state;
    development_peer_probe_report_begin(&report_state);
    char message[DEVELOPMENT_PEER_PROBE_MESSAGE_CAPACITY];

    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, "/dev/ttyACM1", message, sizeof(message)), "first silent pass: nothing yet");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, "/dev/ttyACM1", message, sizeof(message)), "second: nothing yet");
    size_t length = observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, "/dev/ttyACM1", message, sizeof(message));
    REMOTE_TEST_ASSERT(report, length > 0, "third silent pass is reported");
    REMOTE_TEST_ASSERT(report, strstr(message, "/dev/ttyACM1") != NULL, "the node is named");
    REMOTE_TEST_ASSERT(report, strstr(message, "no HELLO") != NULL, "the symptom is named");
    REMOTE_TEST_ASSERT(report, strstr(message, "another peer") != NULL, "the first likely cause is named");
    REMOTE_TEST_ASSERT(report, strstr(message, "not running") != NULL, "the second likely cause is named");
    for(int pass = 0; pass < 5; pass++) {
        REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, "/dev/ttyACM1", message, sizeof(message)), "then silent");
    }
}

static void a_silent_node_that_becomes_absent_starts_its_count_again(RemoteTestReport* report) {
    /* A silent open followed by a pass where the node did not open at all is
     * not the same node stubbornly silent: the device was reinserted. */
    DevelopmentPeerProbeReport report_state;
    development_peer_probe_report_begin(&report_state);
    char message[DEVELOPMENT_PEER_PROBE_MESSAGE_CAPACITY];

    observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, "/dev/ttyACM1", message, sizeof(message));
    observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, "/dev/ttyACM1", message, sizeof(message));
    observe(&report_state, DevelopmentPeerProbeEventAbsent, "/dev/ttyACM1", message, sizeof(message));
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, "/dev/ttyACM1", message, sizeof(message)), "the count restarted: nothing at what would have been the third");
}

static void a_find_says_nothing_and_resets_the_search(RemoteTestReport* report) {
    DevelopmentPeerProbeReport report_state;
    development_peer_probe_report_begin(&report_state);
    char message[DEVELOPMENT_PEER_PROBE_MESSAGE_CAPACITY];

    observe(&report_state, DevelopmentPeerProbeEventOpenDenied, "/dev/ttyACM0", message, sizeof(message));
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)observe(&report_state, DevelopmentPeerProbeEventFound, "/dev/ttyACM1", message, sizeof(message)), "a find is the shell's line, not this module's");
    size_t length = observe(&report_state, DevelopmentPeerProbeEventOpenDenied, "/dev/ttyACM0", message, sizeof(message));
    REMOTE_TEST_ASSERT(report, length > 0, "after a find the next search reports a denial afresh");
}

static void every_message_fits_the_declared_capacity_and_a_smaller_buffer_is_refused(RemoteTestReport* report) {
    DevelopmentPeerProbeReport report_state;
    development_peer_probe_report_begin(&report_state);
    char message[DEVELOPMENT_PEER_PROBE_MESSAGE_CAPACITY];
    /* The longest node name the probe can produce, per the shell's bound. */
    const char* long_node = "/dev/ttyACM15";

    size_t denied_length = observe(&report_state, DevelopmentPeerProbeEventOpenDenied, long_node, message, sizeof(message));
    REMOTE_TEST_ASSERT(report, denied_length > 0 && denied_length < sizeof(message), "the denial fits");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, (int)denied_length, (int)strlen(message), "and is terminated");

    observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, long_node, message, sizeof(message));
    observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, long_node, message, sizeof(message));
    size_t silent_length = observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, long_node, message, sizeof(message));
    REMOTE_TEST_ASSERT(report, silent_length > 0 && silent_length < sizeof(message), "the silence report fits");

    development_peer_probe_report_begin(&report_state);
    char small[16];
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)observe(&report_state, DevelopmentPeerProbeEventOpenDenied, long_node, small, sizeof(small)), "a buffer too small gets nothing rather than a cut message");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, (int)strlen(small), "and is left empty");
}

static void the_report_never_allocates(RemoteTestReport* report) {
    DevelopmentPeerProbeReport report_state;
    char message[DEVELOPMENT_PEER_PROBE_MESSAGE_CAPACITY];
    int allocations_before = remote_test_allocation_count;
    development_peer_probe_report_begin(&report_state);
    for(int pass = 0; pass < 4; pass++) {
        observe(&report_state, DevelopmentPeerProbeEventOpenDenied, "/dev/ttyACM0", message, sizeof(message));
        observe(&report_state, DevelopmentPeerProbeEventOpenedSilent, "/dev/ttyACM1", message, sizeof(message));
    }
    observe(&report_state, DevelopmentPeerProbeEventFound, "/dev/ttyACM1", message, sizeof(message));
    REMOTE_TEST_ASSERT_EQUAL_INT(report, allocations_before, remote_test_allocation_count, "no allocation");
}

int main(void) {
    static const RemoteTestCase test_cases[] = {
        {"a denied open is reported once per node per search and the retry is silent",
         a_denied_open_is_reported_once_per_node_per_search_and_the_retry_is_silent},
        {"nodes are reported independently", nodes_are_reported_independently},
        {"a silent node is named after three passes once", a_silent_node_is_named_after_three_passes_once},
        {"a silent node that becomes absent starts its count again", a_silent_node_that_becomes_absent_starts_its_count_again},
        {"a find says nothing and resets the search", a_find_says_nothing_and_resets_the_search},
        {"every message fits the declared capacity and a smaller buffer is refused",
         every_message_fits_the_declared_capacity_and_a_smaller_buffer_is_refused},
        {"the report never allocates", the_report_never_allocates},
    };
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
