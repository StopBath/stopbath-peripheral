# Changelog

One entry per tag (`SD3`). Each names the appliance definition digest the tag
was built against, every change under `shared/` since the previous tag, and
which device gates were cleared at which commit since the previous tag.

## v0.2.0, SE1: the shared code extracted (pending the author's tag)

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
