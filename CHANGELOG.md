# Changelog

One entry per tag (`SD3`). Each names the appliance definition digest the tag
was built against, every change under `shared/` since the previous tag, and
which device gates were cleared at which commit since the previous tag.

## v0.4.0, SE3: the Pico proven from the tree (pending the gate and the author's tag)

Appliance definition: unchanged, sha256
`5793e16a0b97cd0c122f53c15a1a76b2272a595df0788d0edf065c02b9009d4a`.

Under `shared/`: nothing changed. Under `pico/`: the documents amended for
the one tree (specification Part 3, `KE1`, `KE3`, Part 10; `README.md`;
`PROTOCOL.md`; `TESTING.md`; `HARDWARE_COMPATIBILITY.md` gains the `SE3` row);
`scripts/check_link_map.py`, run by `ci-pico.yml` after every firmware
build. The image is unchanged from `v0.3.0`.

Gates: the `SE3` gate (`KE4` against the peer built from `shared/`, then
`KE6` against the appliance) is the author's on this tag's image, recorded in
`pico/HARDWARE_COMPATIBILITY.md` with the commit. `stopbath-pico` is retired
once it is cleared.

## v0.3.0, SE2: the development peer and the link edge table (2026-09-16, `ff4375c`, pending the author's tag)

Appliance definition: unchanged, sha256
`5793e16a0b97cd0c122f53c15a1a76b2272a595df0788d0edf065c02b9009d4a`.

Under `shared/`: `peer/` (the development peer, core and POSIX shell) and
`link/` (the decision table from transport facts to session events), each
from the Pico's copy, byte for byte; their suites under `shared/tests/`, the
link suite gaining three named cases for the Flipper's hardware findings (a
physical pull with no DTR drop, a resume with no preceding suspend, a
re-enumeration with stale cached DTR), all of which the table already
satisfied. `make peer` builds the peer from `shared/`. `stopbath_shared_link`
is the Pico's third static library target. The Flipper's transport is
unchanged and does not yet adopt the table (`FD` work).

Both images byte for byte the size they were; neither link map names a peer,
fuzz or test symbol.

Gates cleared since `v0.2.0`: none; `SE2` has no hardware gate.

## v0.2.0, SE1: the shared code extracted (2026-09-16, `ea3b0d1`, pending the author's tag)

Appliance definition: `shared/protocol.json`, sha256
`5793e16a0b97cd0c122f53c15a1a76b2272a595df0788d0edf065c02b9009d4a`, frozen at
`FD20` on 2026-09-12.

Under `shared/`, all new to the tree at this tag: the protocol library and its
generated tables, `protocol.json` and its digest check, the generator, the
fuzz harness, the test harness, the protocol suite, the published QR vectors,
the vendored QR encoder, the tree wide typography scan, the one copy and
include checks, `Makefile` and `CMakeLists.txt`. Every one came from a device
directory (`PROVENANCE.md`); no shared code changed in behaviour.

Both devices compile the shared code from `shared/` and no longer carry a
copy: the Flipper through `flipper/lib/shared` and its manifest, the Pico
through `add_subdirectory` and two static library targets. Both images are
byte for byte the size they were: `stopbath_remote.fap` 32872 bytes,
`stopbath_pico.uf2` 113664 bytes.

Continuous integration moved to the root as three workflows.

Gates cleared since `v0.1.0`: none; `SE1` has no hardware gate.

## v0.1.0, SE0: the import (2026-09-16, `8514e4e80a2e1792c17de7ac7954d5336b3ba203`)

Appliance definition: as held by the Pico at import, sha256
`5793e16a0b97cd0c122f53c15a1a76b2272a595df0788d0edf065c02b9009d4a`.

`stopbath-flipper` (19 commits, at `815bd690`) and `stopbath-pico` (14
commits, at `641b869c`) imported with their full histories under `flipper/`
and `pico/`, byte for byte. Nothing under `shared/` yet.

Gates cleared: none at this tag. The devices' own gates as recorded in their
`HARDWARE_COMPATIBILITY.md` files predate the import and name the old
repositories' commits.
