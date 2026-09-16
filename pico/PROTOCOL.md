# Protocol

This directory implements the StopBath peripheral protocol, version 1. It
does not define it.

The definition is owned by the StopBath repository (extension 4.1) and lives
there in `docs/peripheral/PROTOCOL.md` (the prose) and
`docs/peripheral/protocol.json` (the single normative table). It was frozen
at `FD20` on 2026-09-12 and is a sacred contract under Flipper 0.11: nothing
in it is renamed, aliased, normalised or improved by an implementer.

## What this directory holds, and what `../shared/` holds

This directory holds the `session/` that speaks the protocol for this
device, and `firmware/usb_link.c`, the USB CDC edge that carries it.

`../shared/` holds, once for both devices (since 2026-09-16; from `KE3` to
then this directory carried its own copies): `protocol.json`, byte for byte
the appliance's frozen copy, held to its digest by
`scripts/check_protocol_definition.py`; the parser and encoder in
`protocol/`, with their tables generated from the table by
`scripts/generate_protocol_tables.py`, which `--check` holds in step; the
fuzz harness for the parser; the link decision table in `link/` that
`usb_link.c` consumes; and the development peer in `peer/`, so the device
can be developed with no appliance present. `../shared/PROTOCOL.md` says how
each is held.

## What this device sends

| Verb | Fields this device sends | Note |
|---|---|---|
| `HELLO` | `version=1 peripheral=stopbath-pico locked=0` | on attach and restart (`KD7`) |
| `BUTTON` | `event=<one of the four> foregrounded=1 unlocked=1` | both flags always true, spec 2.4 |
| `STATE` | never | there is no lock, `KD5` |

Which event Key1 sends is decided by the page in the last `DISPLAY` received
(spec 2.3, a recorded deviation, provisional until `KD9`).

## What this directory never proposes

A protocol change. If the field shows one is needed, it is raised in the
StopBath repository as a version 2 decision (spec 2.8, the StopBath repository's `docs/PICO_REMOTE_HANDOFF.md`
Part 2).
