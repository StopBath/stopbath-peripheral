/*
 * The link edge decision table (Pico spec KE4, peripheral spec SE2):
 * total over every observed fact and prior state, held under shared/ so
 * both transports map the same facts to the same session events: the port
 * is open exactly when the cable is present and the host has asserted DTR,
 * the session hears only the edges, and bytes are delivered only while open.
 */
#include "test_support.h"

#include "../link/remote_link_edge.h"

typedef struct {
    bool was_open;
    bool cable_present;
    bool host_opened;
    RemoteLinkEdgeOutcome expected;
    const char* description;
} EdgeRow;

/* Every combination: two prior states by four observations. */
static const EdgeRow edge_rows[] = {
    {false, false, false, RemoteLinkEdgeNone, "closed, nothing: stays closed"},
    {false, false, true, RemoteLinkEdgeNone, "closed, DTR without a cable (stale): stays closed"},
    {false, true, false, RemoteLinkEdgeNone, "closed, cable but host has not opened: stays closed"},
    {false, true, true, RemoteLinkEdgeOpened, "closed, cable and DTR: opens"},
    {true, false, false, RemoteLinkEdgeClosed, "open, cable pulled and DTR gone: closes"},
    {true, false, true, RemoteLinkEdgeClosed, "open, cable pulled with DTR cached: closes"},
    {true, true, false, RemoteLinkEdgeClosed, "open, host closed the port: closes"},
    {true, true, true, RemoteLinkEdgeNone, "open, still both: stays open"},
};

static void the_decision_table_is_total_and_opens_only_on_cable_and_dtr_together(RemoteTestReport* report) {
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(edge_rows); row_index++) {
        const EdgeRow* row = &edge_rows[row_index];
        RemoteLinkEdge edge;
        remote_link_edge_initialise(&edge);
        edge.port_open = row->was_open;
        RemoteLinkEdgeOutcome outcome = remote_link_edge_observe(&edge, row->cable_present, row->host_opened);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, row->expected, outcome, row->description);
        REMOTE_TEST_ASSERT_EQUAL_INT(
            report, row->cable_present && row->host_opened, remote_link_edge_is_open(&edge), row->description);
    }
}

static void an_edge_is_reported_once_and_repeats_are_silent(RemoteTestReport* report) {
    RemoteLinkEdge edge;
    remote_link_edge_initialise(&edge);
    REMOTE_TEST_ASSERT(report, !remote_link_edge_is_open(&edge), "starts closed");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&edge, true, true), "opens");
    for(int repeat = 0; repeat < 5; repeat++) {
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeNone, remote_link_edge_observe(&edge, true, true), "silent");
    }
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeClosed, remote_link_edge_observe(&edge, true, false), "closes");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeNone, remote_link_edge_observe(&edge, false, false), "silent");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&edge, true, true), "opens again");
}

/* A cable pull and reinsertion with the host reopening is two edges, so
 * the session handshakes afresh rather than believing the old link. */
static void a_cable_pull_and_reinsertion_is_a_close_then_an_open(RemoteTestReport* report) {
    RemoteLinkEdge edge;
    remote_link_edge_initialise(&edge);
    remote_link_edge_observe(&edge, true, true);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeClosed, remote_link_edge_observe(&edge, false, false), "pull");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeNone, remote_link_edge_observe(&edge, true, false), "reinserted, host not yet open");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&edge, true, true), "host reopened");
}

/*
 * The three observations the Flipper's transport made on hardware
 * (flipper/docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md, "Hardware
 * findings, FE4" 3 and "Reconnect and button findings" 1), as sequences of
 * the two facts the table takes. Each case says what the table does with
 * the facts as the edge reports them, and therefore what the edge owes the
 * table: the truth about DTR, never a cached value. Where a device's edge
 * and these cases disagree, that is a finding for the device (peripheral
 * spec SE2), not for the table.
 */

/* A cable yank gives the host no chance to drop DTR. The table closes on the
 * loss of the cable alone, whatever the DTR fact says, so a pull is a close
 * even before the edge has cleared its cached DTR. */
static void a_physical_pull_with_no_dtr_drop_closes_on_the_cable_alone(RemoteTestReport* report) {
    RemoteLinkEdge edge;
    remote_link_edge_initialise(&edge);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&edge, true, true), "open");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeClosed, remote_link_edge_observe(&edge, false, true), "yanked, DTR still reported");
    REMOTE_TEST_ASSERT(report, !remote_link_edge_is_open(&edge), "closed after the yank");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeNone, remote_link_edge_observe(&edge, false, true), "still out, still stale: silent");
}

/* A resume that arrives with no preceding suspend: the transport never saw
 * the cable go, and the host has re-enumerated the device but not yet
 * opened the port. With DTR re-read from the line, as the Flipper's edge now
 * does, the table closes the link the moment the re-read says the port is
 * not open, and opens it only when the host really opens it. */
static void a_resume_with_no_preceding_suspend_follows_the_re_read_dtr(RemoteTestReport* report) {
    RemoteLinkEdge edge;
    remote_link_edge_initialise(&edge);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&edge, true, true), "open");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeClosed, remote_link_edge_observe(&edge, true, false), "resume, DTR re-read false: closes");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeNone, remote_link_edge_observe(&edge, true, false), "host still not open: silent");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&edge, true, true), "host opens: opens");
}

/* A re-enumeration with a stale cached DTR. The table trusts the facts it
 * is given: fed the stale value it opens on reinsertion with nobody there,
 * which is exactly the false handshake the Flipper hit. Fed the truth, the
 * link stays closed until the host opens. Both halves are asserted so the
 * hazard, and the edge's duty to clear the cached value, are on record. */
static void a_re_enumeration_with_stale_cached_dtr_opens_falsely_unless_the_edge_clears_it(RemoteTestReport* report) {
    RemoteLinkEdge stale;
    remote_link_edge_initialise(&stale);
    remote_link_edge_observe(&stale, true, true);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeClosed, remote_link_edge_observe(&stale, false, true), "pull");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&stale, true, true), "reinserted with the cached DTR: a false open");

    RemoteLinkEdge cleared;
    remote_link_edge_initialise(&cleared);
    remote_link_edge_observe(&cleared, true, true);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeClosed, remote_link_edge_observe(&cleared, false, false), "pull, cached DTR cleared by the edge");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeNone, remote_link_edge_observe(&cleared, true, false), "reinserted, host not yet open: stays closed");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&cleared, true, true), "host opens: a true open");
}

static const RemoteTestCase test_cases[] = {
    {"the decision table is total and opens only on cable and dtr together",
     the_decision_table_is_total_and_opens_only_on_cable_and_dtr_together},
    {"an edge is reported once and repeats are silent", an_edge_is_reported_once_and_repeats_are_silent},
    {"a cable pull and reinsertion is a close then an open", a_cable_pull_and_reinsertion_is_a_close_then_an_open},
    {"a physical pull with no dtr drop closes on the cable alone", a_physical_pull_with_no_dtr_drop_closes_on_the_cable_alone},
    {"a resume with no preceding suspend follows the re-read dtr", a_resume_with_no_preceding_suspend_follows_the_re_read_dtr},
    {"a re-enumeration with stale cached dtr opens falsely unless the edge clears it",
     a_re_enumeration_with_stale_cached_dtr_opens_falsely_unless_the_edge_clears_it},
};

int main(void) {
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
