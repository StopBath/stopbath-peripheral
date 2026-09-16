# StopBath Remote

A Flipper Zero application that acts as a small physical control surface for a
StopBath appliance, so the photographer can present, start, and terminate a
session without taking a phone out.

The Flipper is a peripheral. It reports what physically happened and renders
what it is sent. StopBath remains authoritative for every session, every guest,
and the meaning of every button. The specification is
`STOPBATH_FLIPPER_SPEC.md`; the appliance side is specified in
`docs/PERIPHERAL_EXTENSION.md` in the StopBath repository.

This directory is the `flipper/` directory of the `stopbath-peripheral`
repository, which holds both peripherals and the code they share
(`../shared/`) since 2026-09-16; before that it was the `stopbath-flipper`
repository, whose history is here in full. Everything below is run from this
directory.

## Status

Phases `FE1` to `FE4` are done on the automated side. The application talks to
the appliance (or the development peer) over the USB link: it handshakes,
renders the display state it is sent, draws the page's QR and presents it over
NFC, reports button events under the guard, and reconnects without a repair
step. The protocol was frozen by the appliance at `FD20` on 2026-09-12; the
definition this directory compiles against is `../shared/protocol.json`, held
to the appliance's digest. The FE4 hardware gate, twenty cable pulls against
the peer, is the author's on the device.

Hardware gates cleared by the author are recorded in
`HARDWARE_COMPATIBILITY.md`. Nothing in this repository claims a hardware gate
has passed.

## Supported firmware

Unleashed `unlshd-086`, API `87.6`, firmware commit `e7e4e179`. No other
distribution or version is claimed. The evidence is in
`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md`.

## Building the application

The build tool is `ufbt`, pinned to the Unleashed SDK for the version above.
The SDK is kept per project under `.ufbt/` (ignored by git) and selected by the
tracked `.env` file.

```bash
py -3 -m pip install --user ufbt==0.2.6
```

```bash
py -3 -m ufbt update --hw-target f7 --url "https://github.com/DarkFlippers/unleashed-firmware/releases/download/unlshd-086/flipper-z-f7-sdk-unlshd-086.zip"
```

```bash
py -3 -m ufbt
```

The first `ufbt` run downloads the pinned ARM toolchain (`gcc-arm-none-eabi
12.3`, package 39) into `~/.ufbt/toolchain` and links it into `.ufbt/`. The
result is `dist/stopbath_remote.fap`. On Linux and in continuous integration
replace `py -3 -m ufbt` with `ufbt`.

To install onto an attached Flipper and launch it:

```bash
py -3 -m ufbt launch
```

## Host tests

The pure logic under `remote_input/`, `remote_display/` and `session/` has no
SDK dependency and is tested on the development machine with `gcc` and
`make`, from this directory:

```bash
make test
```

The shared suites (protocol, peer, link table) under this device's host flag
set, built into this directory's `build/`:

```bash
make test-shared
```

The sanitiser build needs a compiler with `libasan` and `libubsan`, which the
MinGW `gcc` on Windows does not ship. Run it under WSL or on Linux:

```bash
make test-sanitise
```

The font metrics table used for text truncation is generated from the pinned
firmware's font data and checked in. To regenerate it, check the firmware out
at the pinned tag beside this repository and point the generator at its font
source:

```bash
git clone --depth 1 --branch unlshd-086 https://github.com/DarkFlippers/unleashed-firmware.git ../unleashed-firmware
```

```bash
py -3 scripts/generate_font_metrics.py ../unleashed-firmware/lib/u8g2/u8g2_fonts.c
```

The typography scan required by specification 0.8, and the check that the
generated protocol tables match `protocol.json`, live under `../shared/scripts/`
and run over the whole tree; these targets forward to them:

```bash
make check-typography check-protocol-tables PYTHON="py -3"
```

The fuzz harness and the development peer build from `../shared/`
(`make -C ../shared fuzz`; `make peer` here forwards).

The build reaches `../shared/` through the symlink `lib/shared`. On Windows,
creating or checking it out needs Developer Mode (so a standard user may
create symlinks) and `git config --global core.symlinks true` set before the
clone; otherwise the link checks out as a text file and `ufbt` reports the
protocol symbols unresolved.

## Layout

| Path | Contents |
|---|---|
| `application.fam` | the application manifest |
| `stopbath_remote.c` | the SDK facing application, kept thin |
| `remote_input/` | pure logic: what a press does to the device, no SDK |
| `remote_display/` | pure logic: what goes where on the screen, the shared display fixtures, the generated font metrics, the QR wrapper, and the NDEF builder, no SDK |
| `session/` | the client session: handshake, reconnection, the guard, no SDK |
| `remote_transport.c` | the USB CDC transport, the one SDK edge of the link |
| `lib/shared` | a symlink to `../shared`, the way `application.fam` reaches the shared sources |
| `tests/` | this device's host tests; the harness and the published QR vectors are under `../shared/tests/` |
| `scripts/` | the font metrics, icon and NDEF vector generators, and the QR vectors generator that writes into `../shared/tests/` |
| `../shared/` | held once for both devices: the protocol library and `protocol.json`, the development peer, the link decision table, the fuzz harness, the test harness, the vendored QR encoder, the typography scan and the tree wide checks; see `../shared/PROTOCOL.md` and `../shared/TESTING.md` |
| `docs/evaluation/` | the Plan stage evidence log |

## Documents

`PROTOCOL.md`, `TESTING.md`, `IMPLEMENTATION_DEVIATIONS.md`,
`HARDWARE_COMPATIBILITY.md`, `FIELD_NOTES.md`, and
`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md` are the documents the
specification requires. Each says what it is for at the top.

## Licence

MIT. See `LICENSE`.
