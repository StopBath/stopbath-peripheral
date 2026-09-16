# StopBath Peripheral

The peripheral side of the StopBath protocol: the two physical remotes for a
StopBath appliance and, once extracted, the code they share. The appliance
(`stopbath.photo`) owns the protocol and depends on nothing here; this
repository implements it.

A peripheral reports what physically happened and renders what it is sent.
StopBath remains authoritative for every session, every guest and the meaning
of every button. The specification for this repository is
`STOPBATH_PERIPHERAL_SPEC.md`; each device keeps its own specification in its
directory.

## What is where

| Directory | What it is | Its specification |
|---|---|---|
| `flipper/` | the Flipper Zero application, formerly the `stopbath-flipper` repository | `flipper/STOPBATH_FLIPPER_SPEC.md` |
| `pico/` | the Raspberry Pi Pico 2 W remote with a 4.2 inch e-paper panel, formerly the `stopbath-pico` repository | `pico/STOPBATH_PICO_SPEC.md` |
| `shared/` | not yet present; created at `SE1` to hold the protocol library, the development peer, the link table and the test harness once, for both devices | `STOPBATH_PERIPHERAL_SPEC.md` Part 2 |
| `docs/evaluation/` | this repository's Plan stage record | `STOPBATH_PERIPHERAL_SPEC.md` Part 4 |

`PROVENANCE.md` records where every file came from. `CHANGELOG.md` arrives
with the first tag.

## Status

`SE0`, the import with history, is done and tagged `v0.1.0` (2026-09-16).
Each device directory is byte for byte its old repository's `main` and builds
and tests from inside its own directory exactly as before. `SE1`, the
extraction of the shared code into `shared/`, is the current phase; nothing
has been moved yet.

## Building

Each device builds from inside its own directory, with the tool state it
keeps there. Follow that directory's `README.md`; in short:

Flipper, from `flipper/`:

```bash
py -3 -m ufbt
```

Pico, from `pico/`, after `scripts/setup_toolchain.sh` has run once:

```bash
scripts/build_firmware.sh
```

Host tests for either, from inside its directory:

```bash
make test
```

On Windows, keep the checkout at a short path (for example
`C:\Users\<you>\Documents\GitHub\stopbath-peripheral` is short enough): `ufbt`'s
bundled SCons fails to import one of its own modules from a deep path.

## Where the old repositories went

`stopbath-flipper` and `stopbath-pico` are imported here in full, with every
commit, under `flipper/` and `pico/`. `git log --follow` on any file reaches
its history in the old repository. The commits imported are named in
`PROVENANCE.md`. The old repositories are retired at `SE3` and `SE4`, each
after its device has been proven to build from this tree.

## Licence

MIT, `LICENSE`, the same holder and text as the three repositories this one
draws on.
