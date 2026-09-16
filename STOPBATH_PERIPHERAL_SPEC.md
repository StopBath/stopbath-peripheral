# StopBath Peripheral

A specification for handoff to a coding agent.

Status: DRAFT, 2026-09-15, second revision. Nothing in it is accepted until the
author says so. It proposes one repository, `stopbath-peripheral`, holding the
two existing peripherals and the code they share, and every decision that
proposal needs is listed in Part 9 for the author to take.

The peripheral side of the StopBath protocol has two implementations: the
Flipper Zero application in `stopbath-flipper` and the Pico remote in
`stopbath-pico`. Each carries the same parser, encoder, table generator, fuzz
harness, test harness, development peer, published QR vectors and typography
scan, the second copied from the first byte for byte with a hand maintained
digest table beside it. This document specifies one repository in which both
devices live beside the code they share, so there is one copy of the shared
code, one continuous integration run that builds both images from one commit,
and one commit hash that names the state of everything a hardware gate was
cleared against.

This is a companion to `docs/SPEC.md` and `docs/PERIPHERAL_EXTENSION.md` in the
StopBath repository, to `STOPBATH_FLIPPER_SPEC.md` and to
`STOPBATH_PICO_SPEC.md`, both of which move into this repository unchanged in
authority. It replaces no rule in any of them. Where this document and any of
those disagree, the other wins and the disagreement is a defect to raise.

## How to read this document

Linear. Read it top to bottom once, then work the stages in order.

Every normative line carries a tag. An untagged sentence is context, not a
requirement.

| Tag | Meaning |
|---|---|
| **MUST** | Mandatory. Not negotiable. Failing it fails the phase. |
| **MUST NOT** | Prohibited. Not negotiable. |
| **DECIDE** | A blocking decision belonging to the author. Numbered `SD1` and up. Stop and ask. Never guess. |
| **GUIDANCE** | A suggestion with reasoning. Depart from it only by saying so and why. |

Identifiers follow the scheme shared by the other documents. The second letter
says what kind of thing it is and the first says which document owns it. The
device prefixes are unchanged by the move: a Pico phase is still `KE`, a
Flipper decision is still `FD`.

| Prefix | Meaning |
|---|---|
| `E1` and up, `D1` and up | phases and decisions in `docs/SPEC.md` |
| `PE1` and up, `PD1` and up | phases and decisions in `docs/PERIPHERAL_EXTENSION.md` |
| `FE1` and up, `FD1` and up | phases and decisions in `flipper/STOPBATH_FLIPPER_SPEC.md` |
| `KE1` and up, `KD1` and up | phases and decisions in `pico/STOPBATH_PICO_SPEC.md` |
| `SE0` and up | an execute phase in this document |
| `SD1` and up | a decision in this document |

`S` was chosen under `SD1` because it is a prefix of no existing phase or
decision prefix and no existing prefix is a prefix of `SE` or `SD` (the rule in
extension "How to read this document").

Concrete values are illustrative unless they appear in **Appendix A**, which
lists values that are unverified and must be settled during the Plan stage.

### What this document is not

It was written on 2026-09-15 having read every markdown document in the three
existing repositories and none of their code, build files or continuous
integration definitions. Every claim below about a compiler flag set, a manifest
glob, a CMake target, a Makefile rule or a file's digest is a question for the
Plan stage, never a fact to build on. The one exception is what the three
repositories' own documents state about themselves, which is cited where used.

It was also written without `STOPBATH_FLIPPER_SPEC.md`, which is cited by every
other document and is present in none of the three repositories or their git
history (recorded as a defect on 2026-09-15). See `SD6`.

---

# Part 0: Absolute rules

**Part 0 of `docs/SPEC.md` applies in full and is not restated here.** That is:
the git prohibition (0.1), evidence before code (0.2), blocked means ask in the
five part form batched by stage (0.3), tests first (0.4), naming (0.5), comments
explain why (0.6), do not repeat yourself (0.7), no em dashes (0.8), contracts
sacred once accepted (0.10), dependencies (0.11), destructive operations (0.13)
and the consolidated prohibitions (0.14). Restating them would breach 0.7.

Where the appliance text says "Go", read "C"; where it names `gofmt`, `go vet`,
`staticcheck` and `-race`, read the C rules in 0.16 below. The Flipper
specification's additions to 0.8 (no en dash, no converter mangled hyphen run in
prose) apply to the whole tree, and the scan that enforces them is
`shared/scripts/check_typography.py`, run once from the root.

The device specifications' own Part 0 additions (Flipper 0.9 and 0.10, Pico
0.14 and 0.15) continue to apply inside their directories.

Four additions, because this repository is a library and two devices in one
tree.

## 0.15 Two languages, one rule set

The library and both devices are C. Python appears only in the generators and
scans, and shell only in scripts that set up or check a build, exactly as in
both devices today. **MUST NOT** add a third language.

## 0.16 The C rules

These are Flipper 0.10, the C rules both devices already work to, restated for
a tree in which the same C is compiled by two toolchains. Flipper 0.10 remains
the source (restored under `SD6`) and this section adds only what having two
flag sets and a shared session and link layer requires:

- **MUST** compile warning clean, with warnings as errors, under every device's
  flag set (Appendix A), not only the host's. The Flipper evaluation log
  records the reason: the device toolchain builds with `-fshort-enums`, a
  bounds check that is sound on the host is provably false there, and a warning
  clean host build therefore proves nothing about the device build.
- **MUST** run every host test under the address and undefined behaviour
  sanitisers in continuous integration.
- **MUST NOT** allocate on the heap in the parse, encode, session or link paths.
  Every test binary is linked with the heap functions wrapped so a test can
  prove the code under it did not allocate.
- **MUST** size every buffer once from a bound in the protocol table, never from
  input, and refuse rather than truncate when a bound is reached.
- **MUST** keep every module under `shared/` free of any device SDK, driver or
  operating system header, so the whole of `shared/` builds and tests on a
  development machine with `gcc` and `make` and nothing else.

## 0.17 The shared code defines nothing

**MUST NOT** define, extend, rename, alias, normalise or improve the protocol.
The appliance owns it (extension 4.1). This repository holds a copy of the
frozen definition held to the appliance's digest and implements it.

**MUST NOT** encode what any button means anywhere under `shared/`.
Interpretation is the appliance's (extension 2.1) and a device specific choice
such as the Pico's Key1 mapping is that device's recorded deviation, living
under that device's directory and injected into the shared code, never written
in it.

## 0.18 One tree, one copy

**MUST NOT** hold two copies of any file in the tree. A file that both devices
need lives under `shared/` and nowhere else. A device directory holds only what
is that device's. The check in `SE1` enforces it and stays in continuous
integration.

**MUST NOT** let a device directory include a file from the other device's
directory. `flipper/` and `pico/` depend on `shared/` and on nothing in each
other, proven by a check on include lines.

---

# Part 1: Intent and scope

## 1.1 What it is

One repository, three directories with code in them:

```text
shared/     the C both devices compile: protocol library, peer, link table, later the session
flipper/    the Flipper Zero application, today's stopbath-flipper minus its copies
pico/       the Pico 2 W firmware, today's stopbath-pico minus its copies
```

Each device directory is what its repository is today, with its own
specification, its own required documents, its own build entry point run from
inside that directory, and its own identifiers. The move changes where the
files are and removes the copies. It changes no rule either device works to.

`shared/` is also the reference implementation of the peripheral side of
protocol version 1, in the sense that a third peripheral would start there. It
is not the definition, and the appliance never depends on it (extension 1.4:
the peer is a test double and is never ported into the appliance).

## 1.2 Goals

The first useful version:

- holds both devices' full histories, imported so that every file's log is
  intact under its new path
- holds the protocol library, its generator, the frozen `protocol.json` copy
  and the digest check, the fuzz harness, the test harness, the published QR
  vectors, the vendored QR encoder and the typography scan once, under
  `shared/`, with both devices compiling them from there
- holds the development peer once, so either device can be developed with no
  appliance present (Flipper 2.9)
- holds the link edge decision table once, the pure mapping from observed
  transport facts to session events, which both transports must agree on and
  which today exists as pure logic only in the Pico
- builds both device images and runs every host test from one commit in one
  continuous integration run
- names one commit in each device's `HARDWARE_COMPATIBILITY.md` for every gate
- retires the two old repositories with a note that says where the history went
- later, holds a display agnostic client session under `shared/` (`SE5`, `SD4`)

## 1.3 Non-goals

**MUST NOT** hold anything under `shared/` that knows a display geometry, a
font, a key, a refresh policy, an NFC stack, a USB stack, a GPIO or an SDK
function. Those are the devices'.

**MUST NOT** hold a second copy of anything the appliance owns beyond
`protocol.json`. The appliance's `docs/peripheral/PROTOCOL.md` prose is
referenced, not copied.

**MUST NOT** bring the appliance into this repository. It owns the protocol and
must never depend on the peer (extension 1.4); keeping it in its own repository
keeps the direction of dependency visible: `stopbath.photo` publishes the
definition, `stopbath-peripheral` implements it.

**MUST NOT** version by directory. There is no `v1/` and there will be no
`v2/`. The protocol version lives in `protocol.json` and the state of the tree
lives in git tags. A version 2 of the protocol, if the appliance ever freezes
one, is a new frozen definition and a major tag, not a new folder, and whether
one tree ever speaks two versions at once is a decision then (Flipper Part 9).

**MUST NOT** grow the development peer into anything resembling the appliance.
It interprets no button, holds no session and emits what its shell is told to
emit (extension 1.4, Pico `peer/PROVENANCE.md`).

**MUST NOT** carry credentials, fixtures with real payloads, or an experiment
credentials mechanism under `shared/`. The Flipper's `experiment_credentials.h`
pattern stays under `flipper/`, untracked as it is today.

## 1.4 Why one tree rather than a library repository

An earlier revision of this document specified a separate library repository
consumed by each device as a pinned git submodule. It was withdrawn on
2026-09-15 for the reasons below, recorded so the question is not reopened
without new evidence:

- every coordination cost of the submodule design (pin bumps, tags kept in
  step across three repositories, a session refactor as three ordered commits)
  is a cost paid by one author, since 0.1 makes every pin commit theirs, and the
  argument for separate repositories (independent teams, independent release
  cadence) does not apply to one person, two devices and one protocol
- a change that crosses the seam, of which the session (`SE5`) is the first,
  is one commit in one tree with continuous integration proving both images
  still build, rather than a sequence whose intermediate states are broken
- one commit names the state of everything: a gate cleared on the Pico names
  the exact parser bytes the Flipper is also on
- drift becomes impossible by construction rather than caught by a check

Why now rather than earlier: Flipper Part 9 said not to generalise ahead of a
second implementation. The second implementation exists and cleared its `KE6`
gate on 2026-09-15. The drift the copy approach was meant to prevent has
already been observed: the two `protocol.json` copies differed for three days
after promotion (the Flipper's still carried `BACK_SHORT`, which produced the
`BAD_VALUE` refusals of 2026-09-12); the generated tables differ by one
`#define`; and the Pico's link edge table was written by hand "to match the
Flipper's" with no artefact holding both to it. Three copies of a security
boundary is the case 0.7 is written for.

---

# Part 2: The contract

## 2.1 Ownership

Two repositories, one direction of dependency:

```text
stopbath.photo         owns the protocol definition; depends on nothing here
stopbath-peripheral    implements it; holds the appliance's frozen files by digest
  shared/              compiled by both devices; depends on nothing in either
  flipper/  pico/      depend on shared/; depend on nothing in each other
```

**MUST** hold `shared/protocol.json` byte for byte equal to the appliance's
`docs/peripheral/protocol.json`, proven by
`shared/scripts/check_protocol_definition.py` against the recorded digest,
exactly as the Pico does today.

**MUST NOT** propose a protocol change from this repository. A change is raised
in the StopBath repository as a version decision (Pico 2.8, extension 4.1).

**MUST** treat `shared/`'s public headers, the generated table output, the
peer's shell command grammar and the test harness macros as an accepted
contract under 0.10 from the first tag. Inside this tree a change to them is
made together with both devices in one commit; outside it, a third party
peripheral that took `shared/` would meet a breaking change as a major tag with
migration notes in `CHANGELOG.md` (`SD3`).

## 2.2 What is shared

| Module | From | Why it belongs under `shared/` |
|---|---|---|
| `protocol/` parser, encoder, generated tables, `protocol.json`, generator, digest check | both, identical except one generated line | the wire contract's only C implementation; untrusted input into firmware |
| `fuzz/` | both, identical | the parser's proof against hostile bytes |
| `tests/test_support.h` and the allocation wrappers | both, identical | one harness, one definition of "a test with no assertions fails" |
| `tests/qr_published_vectors.h` and its test | both, identical | the QR proof against an independent encoder |
| `lib/qrcodegen/` | both, Nayuki `v1.8.0`, identical | vendored once, provenance once (`SD5`) |
| `peer/` core and shell | both, identical | the stand in for the appliance |
| `link/` decision table | Pico `transport/remote_link_edge.c` | both transports must map the same USB facts to the same session events; the table is the artefact that says what those are |
| `scripts/check_typography.py` | both, identical | one scan, one exemption list, run from the root over the whole tree |
| `session/` | neither as is, see 2.3 | later, `SE5` |

## 2.3 What stays with each device

| Stays under `flipper/` or `pico/` | Because |
|---|---|
| `remote_display/` layouts, fonts, regions, refresh policy | 128 by 64 against 400 by 300 |
| `remote_input/` | five buttons and a lock against two keys and a hold timer |
| the transport edge: `remote_transport.c`, `firmware/usb_link.c` | the one SDK edge of the link on each device |
| the NDEF builder and its vectors | the Pico has no NFC |
| the QR wrapper's geometry: version ceiling, module size, placement | display facts; only the encode to matrix step is shareable (`SD5`) |
| the Key1 mapping, the fixed guard flag values | device deviations, recorded in each device's `IMPLEMENTATION_DEVIATIONS.md`, injected into the shared session |
| `HARDWARE_COMPATIBILITY.md`, `FIELD_NOTES.md`, the device specification, the device evaluation log | the device's own; `shared/` has no hardware |
| `application.fam`, `.env`, `.ufbt/`; `firmware/CMakeLists.txt`, `scripts/setup_toolchain.sh`, `.toolchain/` | each device's build entry point and tool state, run from and ignored under its own directory |

The session is the case that decides the shape of `SE5`. Both device
specifications say `session/` has no SDK dependency, yet the Pico could not copy
the Flipper's because it includes the Flipper's display layout header. That
coupling is the only reason the session is not already shared. A shared session
hands each decoded `DISPLAY` record to a device supplied callback and takes its
guard flag values and its page event choice from device supplied functions.
What `shared/` then owns is the handshake with retry, wholesale replacement of
the record, `BAD_VERSION` marking the link incompatible, drop on disconnect with
nothing queued across it, the bounded outbound queue, the malformed and dropped
counters, and the invariant that a press is sent only while connected. That is
the union of the two devices' session test lists, which is the `SE5` test list.

## 2.4 What a device directory owes

Each device directory:

- **MUST** compile `shared/`'s sources under its own flag set in its own device
  build, reached by a relative path (`../shared/...`) and by no other means: no
  copy, no symlink into the device tree unless Appendix A finds the Flipper's
  build tool needs one, in which case the symlink is recorded as a deviation.
- **MUST** exclude `shared/tests/`, `shared/fuzz/` and `shared/peer/` from its
  device build. The Flipper manifest's source glob is recursive (its evaluation
  log 4.1) and the Pico's CMake takes explicit targets; how each does it is
  Appendix A, and `SE3` proves it by the image's link map.
- **MUST** record in its `HARDWARE_COMPATIBILITY.md` the commit of this
  repository that a gate was cleared at. One hash; there is no longer a
  separate library version to name.
- **MUST** keep its own required documents (its Appendix B) under its own
  directory, each saying what it is for, exactly as today.
- **MUST NOT** carry a copy of anything under `shared/` (0.18).

---

# Part 3: Repository layout and build entry points

```text
stopbath-peripheral/
  README.md                        what each directory is, how to build each, where the old repositories went
  STOPBATH_PERIPHERAL_SPEC.md      this document
  CHANGELOG.md                     per tag, with the appliance definition digest
  PROVENANCE.md                    the import record, written once at SE0
  LICENSE
  Makefile                         root convenience: test, check, everything under shared/ and both devices' host tests
  .github/workflows/ci.yml         jobs: typography, shared host tests and sanitiser and fuzz, flipper FAP, pico firmware; path filtered
  shared/
    protocol/                      remote_protocol.c and .h, remote_protocol_tables.c and .h
    protocol.json                  the appliance's frozen definition, byte for byte
    link/                          remote_link_edge.c and .h: the decision table, no I/O
    peer/                          development_peer_core.c and .h, development_peer_shell.c (POSIX, host only)
    session/                       SE5: the display agnostic client session, no SDK
    fuzz/                          fuzz_remote_protocol.c
    lib/qrcodegen/                 Nayuki QR Code generator, unmodified, LICENSE and PROVENANCE.md
    qr/                            SD5: payload to matrix with a caller supplied version ceiling, no pixels
    tests/                         test_support.h, every shared host test, qr_published_vectors.h
    scripts/                       generate_protocol_tables.py, check_protocol_definition.py, check_typography.py, check_one_copy.py, generate_qr_vectors.sh
    CMakeLists.txt                 the Pico's way in: one static library target per consumable module
    Makefile                       the host way in: test, test-sanitise, fuzz, fuzz-sanitise, check-protocol-tables, check-protocol-definition
  flipper/                         stopbath-flipper as it is, minus shared/'s files; run ufbt from here
    STOPBATH_FLIPPER_SPEC.md       restored (SD6)
    application.fam  stopbath_remote.c  remote_transport.c  remote_display/  remote_input/  tests/  scripts/  docs/
    README.md  PROTOCOL.md  TESTING.md  IMPLEMENTATION_DEVIATIONS.md  HARDWARE_COMPATIBILITY.md  FIELD_NOTES.md
    Makefile                       the Flipper's host tests, pointing at ../shared for the harness
  pico/                            stopbath-pico as it is, minus shared/'s files; run the build scripts from here
    STOPBATH_PICO_SPEC.md
    firmware/  remote_display/  remote_input/  transport/  tests/  scripts/  docs/  lib/waveshare/
    README.md  PROTOCOL.md  TESTING.md  IMPLEMENTATION_DEVIATIONS.md  HARDWARE_COMPATIBILITY.md  FIELD_NOTES.md
    Makefile                       the Pico's host tests, pointing at ../shared for the harness
```

**MUST** keep every directory under `shared/` other than `lib/` free of any
header that is not this repository's own or the C standard library's. The
peer's shell is the one POSIX dependency (termios), is host only, and is never
part of a device build.

**MUST** make the shared modules separable. A device that has no use for the
session or the QR step takes the protocol library alone. In CMake that is one
target per directory; for the Flipper it is one source directory per private
library entry.

**MUST** keep each device's build entry point where it is today, run from
inside its directory: `py -3 -m ufbt` from `flipper/`, `scripts/setup_toolchain.sh`
and `scripts/build_firmware.sh` from `pico/`. Each device's tool state
(`.ufbt/`, `.env`, `.toolchain/`, `build/`, `dist/`) is ignored under its own
directory, as it is today.

**MUST** run the typography scan once from the root over the whole tree, with
the exemption for `lib/` directories (the Flipper's recorded deviation)
applying to every `lib/` in the tree.

**MUST** record where every imported file came from once, in `PROVENANCE.md`
at the root, at `SE0`: source repository, the commit at which its history was
imported, and for the files that were then moved into `shared/`, the device
directory they were taken from and the one whose copy was deleted. After that
the repository's own history is the provenance. For `shared/lib/qrcodegen/`
the record carries the upstream repository and tag as well, since the Flipper
vendored it from there.

**GUIDANCE** Keep the devices' module names (`remote_protocol`,
`development_peer_core`, `remote_link_edge`) so `SE1` to `SE3` are path changes
and deletions, not renames.

---

# Part 4: Stage one, Plan

**MUST NOT** write implementation code or a behavioural test until the Plan
stage has settled the phase being implemented.

`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md` at the root records, for every
item below, source, commit, retrieval date, what was observed, uncertainties,
experiments and conclusions. The two devices' evaluation logs move with them
and are not merged into it. Nothing in this document was read from code, so
every section starts empty.

## 4.1 The devices' flag sets

Read from source, at the devices' current commits: the Flipper's compiler flags
as its SDK's `sdk.opts` and `site_scons/cc.scons` set them (the evaluation log
there records `-std=gnu2x -Wstrict-prototypes -Wall -Wextra -Werror
-Wno-error=deprecated-declarations -Wno-address-of-packed-member
-Wredundant-decls -Wdouble-promotion -Wundef`, `-Os`, `-fshort-enums`, section
garbage collection, and `-DFW_ORIGIN_Unleashed`; confirm at the pinned SDK and
record which are warning flags and which are code generation flags); the
Pico's base warning set from `firmware/CMakeLists.txt` and its host set from its
`Makefile`, including the sanitiser and wrapping arguments. Record the union and
the difference. Every warning flag either device uses is a flag the `shared/`
continuous integration job compiles under.

## 4.2 The devices' build entry points against a sibling directory

Verify how a Flipper `fap_private_libs` entry names its sources
(`scripts/fbt_tools/fbt_apps.py` and the manifest documentation at the pinned
Unleashed commit) and whether it accepts a path outside the application
directory such as `../shared/protocol`. This is the one build question that
could be awkward: the builder is documented as compiling "a library built from
sources under the application's `lib` folder". If it cannot reach a sibling,
the options are a `sources` list with relative paths, a symlink
`flipper/lib/shared` to `../../shared` (recorded as a deviation, and checked on
Windows where symlinks need a setting), or the builder's `fap_extbuild` hook.
Record which works from a real `ufbt` run.

Verify the Pico's `firmware/CMakeLists.txt` and how it adds `protocol/`,
`peer/` and `lib/` today, so `add_subdirectory(../shared ...)` and
`target_link_libraries` replace those lines and nothing else.

## 4.3 The copies as they stand

Diff the corresponding files across the two repositories at their current
commits and record every difference. The expected set: the generated tables
differ by the `maximum_wifi_payload_length_for_qr` line; the two
`protocol.json` files may differ by the status line the Pico's provenance
mentions; the Flipper's `tests/test_remote_protocol.c` may have moved on since
`3677e61`. Anything else is a finding. The Pico's `transport/remote_link_edge.c`
and its test are read against the Flipper's `remote_transport.c` to confirm the
Pico test's claim that the table "matches the Flipper's". The decision for each
differing file, which copy becomes `shared/`'s, is recorded here.

## 4.4 The import

Verify `git filter-repo` (a separate tool, not part of git) is available on the
author's machine and record its version; record that it is a one time migration
tool and not a dependency of anything built (0.11 applies to what ships, and
this ships nothing). Verify on throwaway clones that
`--to-subdirectory-filter` rewrites every path and keeps every commit, that the
merge with `--allow-unrelated-histories` into an empty repository succeeds for
both, and that `git log --follow` on a moved file shows its history from the
old repository. Verify that both old repositories are clean and pushed, so the
import takes everything.

## 4.5 Continuous integration mechanics

Verify what the host runners provide (`gcc`, `make`, Python 3, `libasan`,
`libubsan`), how each device's existing workflow installs its toolchain (the
Flipper's `ufbt` and SDK zip, the Pico's `setup_toolchain.sh`), and how a path
filter is expressed so that a change under `pico/` alone does not build the
FAP. **GUIDANCE** Host `gcc` with the warning flags plus `-fshort-enums` is the
cheap approximation to the Flipper device build for the `shared/` job and
catches the enum class of defect; the device builds proper are the device jobs,
which compile `shared/` again under the real toolchains.

## 4.6 Plan stage exit criteria

Both flag sets recorded from source. The Flipper's sibling path question
answered by a real build. The cross repository diff recorded with every
difference decided. The import rehearsed on throwaway clones. Every `SD` item
answered or explicitly open with the phases it blocks.

---

# Stage two, Execute

Each phase **MUST** begin by rereading `docs/SPEC.md` Part 0, in particular
0.14.

Each phase carries all five headings. Automated criteria **MUST** be
reproducible by the author on a clean machine with no device attached. The
hardware gates in `SE3`, `SE4` and `SE5` are the devices' gates, run on the
devices, recorded in the devices' documents with this repository's commit.
**MUST NOT** claim one has passed or infer one from a green run.

## SE0: Import with history

**Work** The empty repository. Both old repositories imported with their full
histories under `flipper/` and `pico/`. The root `README.md`, `LICENSE`,
`PROVENANCE.md` and this document. Nothing moved, nothing deleted, nothing
edited except the four root files: at the end of `SE0` each device directory
is byte for byte its old repository's `main`, and each still builds and tests
from inside its directory exactly as before. A tag.

Every step of this phase is a git write and is therefore the author's under
0.1. The agent's work is the rehearsed, recorded command sequence in the
evaluation log (4.4) and the four root files; the author runs the sequence.
**GUIDANCE** The shape, illustrative until 4.4 records the real commands:

```text
git clone stopbath-flipper flipper-import && cd flipper-import
git filter-repo --to-subdirectory-filter flipper
cd .. && git clone stopbath-pico pico-import && cd pico-import
git filter-repo --to-subdirectory-filter pico
cd ../stopbath-peripheral
git remote add flipper-import ../flipper-import && git fetch flipper-import
git merge --allow-unrelated-histories flipper-import/main
git remote add pico-import ../pico-import && git fetch pico-import
git merge --allow-unrelated-histories pico-import/main
```

**Precondition** `SD1` settled. 4.4 rehearsed.

**Excluded** Any change inside `flipper/` or `pico/`. Retiring the old
repositories, which waits until `SE3` has proven a device builds from the new
tree.

**Assumptions to verify** 4.4: that both device builds still run from inside
their directories with nothing changed, including the Flipper's `.env` and
`.ufbt/` state and the Pico's `.toolchain/`, each now ignored under its own
directory.

**Tests first** Both devices' existing suites, run from inside their
directories, unchanged. That they pass is the proof the import moved files and
nothing else. Added: a root check that `git log --follow` on one file from each
device reaches a commit older than the import.

**Done when, automated** Both suites green from inside their directories. The
root typography scan green over the whole tree. The history check passes.

## SE1: Extract the shared code

**Work** `shared/` created from the Pico's copies (they carry the appliance's
definition and the digest check; 4.3 records any file where the Flipper's copy
is preferred): `protocol/`, `protocol.json`, the generator, the digest check,
`fuzz/`, `tests/test_support.h`, the protocol tests, `lib/qrcodegen/`, the
published QR vectors and their test, and `scripts/check_typography.py`. The
Flipper's and the Pico's copies deleted, their `PROVENANCE.md` files deleted,
`PROVENANCE.md` at the root extended to say which copy became `shared/`'s. Both
devices' host `Makefile`s and device builds re-pointed at `../shared/` (4.2).
`shared/Makefile` and `shared/CMakeLists.txt`. Continuous integration: the
`shared/` job (typography, host tests, sanitiser, fuzz, both flag set compiles,
the table check, the definition digest check) and the two device jobs, path
filtered. `shared/scripts/check_one_copy.py`, which fails if any file under
`shared/` has a same named and same content file under a device directory, and
the include check of 0.18.

**Precondition** `SE0` tagged. 4.1, 4.2 and 4.3 recorded.

**Excluded** The peer and the link table (`SE2`). The session. Any change to
any moved file beyond a path in an include line.

**Assumptions to verify** 4.1 and 4.3: that the moved files compile warning
clean under both flag sets as they are, or that the demotion each device
already records for `lib/qrcodegen/` under `-Wconversion` is the only one
needed and is recorded here the same way. 4.2: that each device build reaches
`../shared/` by the mechanism the Plan stage found.

**Tests first** The moved suites are the tests; **MUST** see each fail when run
from `shared/` before the moved sources are added (no rule to make the target)
and pass after, so the harness is proven to run them rather than skip them.
Both devices' own suites, unchanged, run from inside their directories. Added:
the one copy check and the include check, each seen to fail on a deliberately
planted duplicate and a deliberately planted cross device include.

**Done when, automated** Every job green. No file under `shared/` has a copy
under a device directory. No device includes the other. Both device images
build in their jobs. A tag.

## SE2: Development peer and link edge table

**Work** `shared/peer/` from the Pico's copy with its test, the Flipper's copy
deleted. `shared/link/` from the Pico's `transport/` with its test, and the
test's claim of agreement with the Flipper's transport turned into a table
driven test against the observations the Flipper's evaluation log records under
"Hardware findings, FE4" and "Reconnect and button findings": a physical pull
with no DTR drop, a resume with no preceding suspend, a re-enumeration with
stale cached DTR. Where the Flipper's transport behaviour and the table disagree
the disagreement is a finding for the Flipper, raised in its
`IMPLEMENTATION_DEVIATIONS.md`, not settled here. The Flipper's
`remote_transport.c` is not changed in this phase; whether it adopts the table
is `FD` work in its own directory.

**Precondition** `SE1` tagged.

**Excluded** Any transport change. The session.

**Assumptions to verify** That the peer shell builds on the continuous
integration runner (POSIX) and is excluded from every device build (the link
map check from `SE3` is brought forward for the peer).

**Tests first** The peer's suite (drives every display state, records every
event, answers the handshake, produces each misbehaviour mode). The link table's
suite (total over every prior state and observation, opens only on cable and
DTR together, each edge reported once, a pull and reinsertion is a close then an
open) plus the three Flipper findings above as named cases.

**Done when, automated** Green. The peer builds from `make peer` in `shared/`
and drives the shared protocol layer through a byte pipe in a test, as the
Flipper's `tests/test_link_integration.c` does today. A tag.

## SE3: First device proven from the tree, and retirement

**Work** The device named by `SD7` has its documents amended per Appendix C
(specification layout table, `README.md` build instructions saying "from this
directory", `PROTOCOL.md` "what this directory holds", `HARDWARE_COMPATIBILITY.md`
recording this repository's commit). Its link gate is re-run on the image
built from this tree. Then the old repository for that device is retired: the
note in Appendix C written as the first section of its `README.md`, the
repository archived read only. Both are the author's git writes; the agent
supplies the text.

**Precondition** `SE2` tagged. `SD7` settled.

**Excluded** The second device. The session.

**Assumptions to verify** 4.2 for that device. That the device build's
exclusion of `shared/tests/`, `shared/fuzz/` and `shared/peer/` holds, proven by
the image size not having grown across `SE1` and `SE2` and by the link map,
since a recursive glob that silently compiled a host test into a firmware
image would be the failure to look for.

**Tests first** The device's full suite, unchanged. Added: a check in its
continuous integration job that the image's link map names no symbol from
`shared/tests/`, `shared/fuzz/` or `shared/peer/`.

**Done when, automated** The device's job green, the link map check green, the
device's documents amended, the old repository's retirement note drafted.

**Hardware gate** The device's own link gate, re-run with the image built from
this tree: for the Pico, twenty cable pulls and a restart of each side against
the peer built from `shared/` (its `KE4` gate), then a session against the real
appliance (its `KE6` gate); for the Flipper, its `FE4` gate against the peer.
Recorded in that device's `HARDWARE_COMPATIBILITY.md` with this repository's
commit. The old repository is archived only after this gate.

## SE4: Second device proven from the tree, and retirement

**Work** As `SE3` for the other device, including whatever `SE3` found the
manifest or CMake needed. Any difference between what the two devices needed
from `shared/` in order to build from it is recorded in the root
`IMPLEMENTATION_DEVIATIONS.md` if it forced a change under `shared/`, since
shared code that needs a device specific edit to be consumed has a defect.

**Precondition** `SE3` cleared, both its automated criteria and its gate.

**Excluded** The session.

**Assumptions to verify** 4.2 for that device.

**Tests first** As `SE3`.

**Done when, automated** As `SE3`. Both old repositories retired. The
appliance repository asked for the citation changes in Appendix C. A tag,
`v1.0.0` under `SD3`.

**Hardware gate** That device's link gate, as `SE3`.

## SE5: The client session

**Work** `shared/session/`: the display agnostic session described in 2.3,
written fresh against the union of the two devices' session tests, with the
Flipper's and the Pico's sessions as references. The device facing surface: a
callback for a decoded record, a callback for link state, and injected
functions for the guard flag values and for the event a physical input maps to
(so the Pico's Key1 choice and the Flipper's lock live under the device
directories). Both devices adopt it in the same commit, each deleting its own
`session/` and re-pointing its deviation entries at the injected function that
now carries the deviation. This is the change the one tree exists for: one
commit, both images built by the same continuous integration run.

**Precondition** `SE4` cleared. `SD4` settled.

**Excluded** Any interpretation of a button. Any knowledge of a page's meaning
beyond passing the record through. Support for a second protocol version.

**Assumptions to verify** That every test in both devices' session suites is
expressible against the callback surface without a display header. Any test
that is not is a finding about the surface, raised before the surface is fixed.

**Tests first** The union of `tests/test_remote_session.c` from both devices,
rewritten against the surface: `HELLO` carries version 1, the injected token and
the injected lock value; the first record is the acceptance; a later record
replaces the previous wholly; `BAD_VERSION` marks the link incompatible; closing
the port discards everything including an unsent press and clears the held
record; a line cut by a disconnection is not completed after reconnection; a
press is sent only while connected and otherwise dropped and counted; every
press carries the injected flag values; the outbound queue is bounded and
overflow is counted; a lost handshake is retried on the interval; malformed
input is counted and leaves the record alone; the session never allocates; and
the Pico's "no input to the mapping can produce anything but a page event"
holds for whatever the Pico injects, tested under `pico/`, not under `shared/`.

**Done when, automated** Green in the `shared/` job and both device jobs in
one run, both devices' session copies gone, both devices' deviation records
pointing at their injected functions.

**Hardware gate** Both devices' link gates once more, since the session is the
code under the transport edge on both devices.

---

# Stage three, Verify

Stage three of `docs/SPEC.md` applies: the per phase reproduction report (exact
commands, toolchain versions, focused and full test results, sanitiser result,
fuzz result, warning count under each flag set, both image sizes, skipped tests
and why, gates outstanding), the continuous integration list, and the release
checks. **MUST NOT** declare a phase complete before the author reproduces the
automated criteria on a clean machine.

Added to the release checks, for every tag:

- `CHANGELOG.md` names the appliance definition digest the tag was built
  against, every change under `shared/` since the previous tag, and which
  device gates were cleared at which commit since the previous tag.
- Each device's `HARDWARE_COMPATIBILITY.md` names a commit of this repository
  for every cleared gate, and the appliance's `HARDWARE_COMPATIBILITY.md` rows
  for the two remotes name the same commits.

---

# Part 5: Security

The parser is the one piece of code on both devices that untrusted bytes reach
before anything else, and after `SE1` it lives in exactly one place.

**MUST** keep the fuzz harness in continuous integration under the sanitisers
on every change under `shared/`, not on a schedule.

**MUST** keep every bound the parser enforces derived from `protocol.json`
through the generator, never written by hand in C.

**MUST NOT** log, print or retain a payload anywhere under `shared/`. The peer
shell prints what it is told to print by the person driving it and is a host
tool.

**MUST NOT** carry any credential, real Wi-Fi payload, or real gallery address
in a test or fixture under `shared/`. The published vectors use the Flipper's
existing synthetic payloads.

The devices' own Part 5 rules (the Pico's flash and panel clearing, the
Flipper's lock) are unchanged and stay in their specifications.

The physical key limit (extension 5.5) is unchanged by this repository, which
changes no transport and no device behaviour.

---

# Part 6: Testing

`docs/SPEC.md` Part 7 applies as the devices apply it: tests cover behaviour,
every unit test is a table driven C function run by `shared/tests/test_support.h`,
a case with no assertions fails, every test runs in under a second on the host,
nothing under `shared/` needs a device, and nothing spans appliance and device.

Three `TESTING.md` files, one per directory with code. `shared/`'s lists every
suite with its command and the device it was taken from, and has no testing
deviations, because there is nothing under `shared/` that needs hardware to
test. The devices' list their own suites, the shared suites their jobs also
run, and their hardware gates, as today.

**MUST** run the shared host tests in every device's continuous integration
job as well as in the `shared/` job, under that device's host flag set. A green
`shared/` job is evidence about `shared/` under its own flags and nothing else
(0.16).

---

# Part 7: Success criteria

The repository succeeds when both peripherals build from one commit with no
copy of any shared file in the tree, when a defect fixed in the parser reaches
both devices in the commit that fixes it, when a gate cleared on either device
names one hash that says what both devices and the shared code were, and when
the next protocol version is one change under `shared/` with both devices
adopting it in the same run.

---

# Part 8: Later

A third peripheral, if one ever exists, is a third device directory beside
`shared/`, adding a display, an input model and a transport edge. Nothing under
`shared/` is designed for it ahead of its existence (Flipper Part 9).

Protocol version 2, if the appliance freezes one (`KD9`, `PICO_REMOTE_HANDOFF`
Part 2), arrives as a new `shared/protocol.json`, regenerated tables and a
major tag, with both devices adopting it in the same commit. Whether the session
then gains a per device version choice is decided then.

The Pico's next hardware step (`KD9` direction: a session toggle switch and
dedicated page buttons) is a Pico phase under `pico/` with its own plan, as its
specification already says, and does not touch `shared/` unless the protocol
changes, which is the appliance's decision.

---

# Part 9: Blocking decisions

**MUST NOT** guess any of these. Ask in the format required by 0.3, batched by
the stage that needs them.

`SD1` **Repository name, identifier prefix, licence. Settled by the author on
2026-09-16** as proposed: `stopbath-peripheral`, `SE` and `SD`, MIT with the
same copyright holder as the three existing repositories, which is also the
licence of everything imported. The name says what the repository is and
nothing about which device, which is right for a tree that holds every device.

`SD2` **One tree. Settled by the author on 2026-09-15**: both devices move into
this repository beside the shared code, with their histories, and the old
repositories are retired. The alternative, a library repository consumed by
pinned submodules, was specified in the first revision of this document and
withdrawn for the reasons in 1.4. Recorded here so it is not reopened without
new evidence.

`SD3` **Tags.** Proposed: semantic version tags for the whole tree; `v0.x` at
`SE0` to `SE3`; `v1.0.0` when both devices are proven from the tree at `SE4`; a
major tag for a new `protocol.json`. Inside the tree there is no "breaking"
change, because both devices move in the same commit; a tag exists so that a
device's `HARDWARE_COMPATIBILITY.md`, the appliance's, and a third party reading
`shared/` can name a state. Consumers outside the tree, if any ever exist, pin
tags.

`SD4` **The session's shape.** Whether `SE5` happens at all, and if so whether
the device surface is callbacks, a struct of function pointers, or compile time
hooks. **GUIDANCE** Do it, as a struct of function pointers supplied at
initialisation, because it is testable on the host with a table of fake devices
and neither device SDK imposes a pattern. Blocks `SE5` only.

`SD5` **Whether `lib/qrcodegen/` and an encode to matrix step live under
`shared/`.** The encoder is vendored identically in both devices with the same
vectors test; the wrapper geometry is not shareable. **GUIDANCE** Vendor it
under `shared/` with its vectors at `SE1`; add the pixel free encode step only
if both devices can adopt it without changing their scanning behaviour, which
is a gate on each device and therefore worth deferring past `SE5`.

`SD6` **The missing Flipper specification. Settled by the author on
2026-09-16**: `STOPBATH_FLIPPER_SPEC.md`, cited by every other document and
present in no repository or history when this document was written, was
restored to `stopbath-flipper` from the author's copy so the `SE0` import
carries it. Its 2.1 diagram was redrawn with dotted arrows to pass its own 0.8
scan; no normative text changed. The alternative, treating 0.16 above as the
reference for the C rules, is withdrawn.

`SD7` **Which device is proven from the tree first. Settled by the author on
2026-09-16: the Pico**, as guided: its build is CMake, it held the digest
check and the link edge table, its transport is the one the appliance recorded
as handling the first record after re-enumeration correctly, and the author
had already run one session against the appliance on an image built from the
`SE1` tree (evaluation log, "Author's observation after SE1"). The Flipper
second, at `SE4`, carrying the sibling path answer (4.2, the symlink) and its
known receive path fault into a phase where the shared link table's tests
name it.

`SD8` **The appliance's citations.** Whether `docs/peripheral/PROTOCOL.md` in
the StopBath repository names this repository as the reference implementation
of the peripheral side and as the holder of the `protocol.json` copy it
currently says the Flipper holds, and whether its `HARDWARE_COMPATIBILITY.md`,
`docs/PICO_REMOTE_HANDOFF.md` and `docs/SPEC.md` header change "the Flipper
repository" and "the Pico repository" to directories of this one. The
appliance's decision; asked at `SE4`, not taken here.

`SD9` **Amending Flipper 2.9 and Pico Part 3.** Flipper 2.9 requires a
development peer "in the repository that is developed against it"; Pico Part 3
mandates the copy with provenance layout and its Part 10 guidance says the peer
is copied rather than referenced across repositories so the two cannot drift.
One tree satisfies all three by their letter as well as their intent: the peer
is in the repository, and there is one copy. What changes is the layout table
and the sentences about copying. Appendix C lists the edits; the author
confirms them.

`SD10` **Retirement.** Whether the old repositories are archived read only on
GitHub after their retirement notes, or left open. **GUIDANCE** Archive. The
history is in this repository, so the old ones are redirects and nothing else;
an open repository invites a change that will never be merged.

---

# Appendix A: Unverified values

None is an accepted contract. **MUST** replace each with a value read from
source or an explicit author decision before the phase that depends on it.

| Value | Starting point | Settled by |
|---|---|---|
| Flipper warning flag set | the set the Flipper evaluation log 4.1 quotes from `site_scons/cc.scons`, plus `-fshort-enums` | 4.1, `SE1` |
| Pico warning flag set | "the unmodified base warning set" the Pico deviations file mentions; read `firmware/CMakeLists.txt` and `Makefile` | 4.1, `SE1` |
| How a `fap_private_libs` entry reaches `../shared/` | unknown; the builder is documented as taking sources under the application's `lib/`; a `sources` list with relative paths, a symlink, or `fap_extbuild` | 4.2, `SE1` for the build, `SE4` or `SE3` for the gate |
| Pico CMake fragment | `add_subdirectory(../shared shared)` and `target_link_libraries` of the protocol target | 4.2, `SE1` |
| Which copy becomes `shared/`'s, per file | the Pico's, for every file; 4.3 records any exception | 4.3, `SE1` |
| Appliance `protocol.json` digest | `5793e16a0b97cd0c122f53c15a1a76b2272a595df0788d0edf065c02b9009d4a`, as the Pico's `protocol/PROVENANCE.md` records it | 4.3, confirm against the appliance at `main` |
| `lib/qrcodegen/` upstream | Nayuki QR Code generator tag `v1.8.0`, commit `720f62bddb7226106071d4728c292cb1df519ceb`, per the Flipper evaluation log | `SD5`, `SE1` |
| `git filter-repo` version and invocation | unknown; rehearsed on throwaway clones | 4.4, `SE0` |
| The import commits | each old repository's `main` at the moment of import, recorded in `PROVENANCE.md` | `SE0` |
| Continuous integration runner packages and path filter syntax | `gcc`, `make`, Python 3, `libasan`, `libubsan`, as the devices' Linux jobs use; `paths:` filters per job | 4.5, `SE1` |
| Tags | `v0.1.0` at `SE0`, `v0.2.0` at `SE1`, `v0.3.0` at `SE2`, `v1.0.0` at `SE4` | `SD3` |

# Appendix B: Required repository documents

At the root: `README.md` (what each directory is, how to build each from inside
it, where the old repositories went and which commit imported them),
`STOPBATH_PERIPHERAL_SPEC.md`, `CHANGELOG.md` (per tag, with the definition
digest and the gates cleared), `PROVENANCE.md` (the import record and the
extraction record, written once each), `IMPLEMENTATION_DEVIATIONS.md` (for
deviations from this document; the devices keep their own),
`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md` (this document's Plan stage; the
devices keep their own) and `LICENSE`.

Under `shared/`: `PROTOCOL.md` (referencing the definition in the StopBath
repository and stating that this directory holds the copy and the check) and
`TESTING.md`.

Under `flipper/` and `pico/`: exactly what each device's own Appendix B
requires today, unchanged, in its directory. No `HARDWARE_COMPATIBILITY.md` and
no `FIELD_NOTES.md` at the root or under `shared/`: the shared code has no
hardware and no field.

# Appendix C: Changes this asks of the other documents and repositories

Listed so they are applied deliberately, each in the phase named, and not
before `SD1` and `SD9` are settled.

## In this repository, at the device's proving phase (`SE3`, `SE4`)

- `pico/STOPBATH_PICO_SPEC.md`: the Part 3 layout table (the shared directories
  become `../shared/`), the "copied from the Flipper repository with
  provenance" sentences in Part 3 and `KE3`, the Part 10 guidance on copying
  the peer, and the Part 3 sentence about a `PROVENANCE.md` beside copied code;
  `pico/README.md` layout table and build instructions ("from this directory");
  `pico/PROTOCOL.md` "What this repository holds" becomes "what this directory
  holds and what `../shared/` holds"; `pico/HARDWARE_COMPATIBILITY.md` records
  this repository's commit for every gate from `SE3` on.
- `flipper/STOPBATH_FLIPPER_SPEC.md`: 2.9 (the peer is in the repository, under
  `../shared/`) and Part 9 (the second implementation now exists);
  `flipper/README.md` layout table and build instructions; `flipper/PROTOCOL.md`
  "The definition" paragraph; `flipper/HARDWARE_COMPATIBILITY.md` likewise.
- Deleted at `SE1`: `pico/protocol/PROVENANCE.md`, `pico/peer/PROVENANCE.md`,
  `pico/lib/qrcodegen/PROVENANCE.md`, `flipper/lib/qrcodegen/PROVENANCE.md`,
  their content carried into the root `PROVENANCE.md`.

## In the old repositories, at `SE3` and `SE4` respectively (author's git writes)

The first section of each `README.md`, above everything else, so that it is
what a link lands on:

```text
## Retired

This repository is retired. Its history and every file in it were imported into
https://github.com/<owner>/stopbath-peripheral under `pico/` (or `flipper/`) at
commit <import commit>, from this repository's commit <last commit here>, with
`git log --follow` intact. Development continues there. Nothing here will be
merged, no tag here will move, and this repository is archived read only.
```

Then, under `SD10`, the repository is archived on GitHub.

## Asked of the StopBath repository, at `SE4` (`SD8`; its decision)

- `docs/peripheral/PROTOCOL.md`: "The Flipper repository holds a copy of
  `protocol.json`" becomes this repository's `shared/protocol.json`.
- `HARDWARE_COMPATIBILITY.md`: the two remote rows name `stopbath-peripheral`
  with a directory and a commit in place of two repositories and two commits.
- `docs/PICO_REMOTE_HANDOFF.md` and the `docs/SPEC.md` header: "the Flipper
  repository" and "the Pico repository" become directories of this one.
- Nothing in Go changes, by extension 4.1 and the Pico handoff's governing
  fact. Nothing in `docs/PERIPHERAL_EXTENSION.md` changes.
