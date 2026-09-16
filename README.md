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
| `shared/` | the code both devices compile, held once: the protocol library and the appliance's frozen `protocol.json`, the development peer, the link edge decision table, the fuzz harness, the test harness, the published QR vectors, the vendored QR encoder, and the tree wide checks | `STOPBATH_PERIPHERAL_SPEC.md` Part 2, `shared/PROTOCOL.md`, `shared/TESTING.md` |
| `docs/evaluation/` | this repository's Plan stage record | `STOPBATH_PERIPHERAL_SPEC.md` Part 4 |

`PROVENANCE.md` records where every file came from, `CHANGELOG.md` what each
tag holds, and `IMPLEMENTATION_DEVIATIONS.md` where the tree departs from its
specification and why.

## Status

`SE0`, the import with history, is tagged `v0.1.0` (2026-09-16). `SE1`
(the protocol library, harness, fuzz, QR encoder and checks into `shared/`)
and `SE2` (the development peer and the link edge table) are done on the
automated side: both device images build from the tree at exactly the size
they were, every suite is green from every directory, and no shared file has
a copy under a device directory. `SE3`, the first device proven from the
tree and its old repository retired, is next and needs `SD7`.

## Building

Each device builds from inside its own directory, with the tool state it
keeps there, and compiles `shared/` by relative path. Follow that directory's
`README.md`; in short:

Flipper, from `flipper/`:

```bash
py -3 -m ufbt
```

Pico, from `pico/`, after `scripts/setup_toolchain.sh` has run once:

```bash
scripts/build_firmware.sh
```

Host tests for either, from inside its directory, and the shared suites
under that device's flags:

```bash
make test test-shared
```

Everything under `shared/`, including the tree wide checks, from `shared/`
(on Windows add `PYTHON="py -3"`):

```bash
make check
```

The Flipper reaches `shared/` through the symlink `flipper/lib/shared`. On
Windows that needs Developer Mode (so a standard user may create symlinks)
and `git config --global core.symlinks true` before cloning; otherwise the
link checks out as a text file and `ufbt` finds no shared sources.

On Windows, keep the checkout at a short path (directly under a
`Documents\GitHub` folder is short enough): `ufbt`'s bundled SCons fails to
import one of its own modules from a deep path, and the Pico toolchain's C++
include paths approach the 260 character limit.

## Where the old repositories went

`stopbath-flipper` and `stopbath-pico` are imported here in full, with every
commit, under `flipper/` and `pico/`. `git log --follow` on any file reaches
its history in the old repository. The commits imported are named in
`PROVENANCE.md`. The old repositories are retired at `SE3` and `SE4`, each
after its device has been proven to build from this tree.

## Licence

MIT, `LICENSE`, the same holder and text as the three repositories this one
draws on.
