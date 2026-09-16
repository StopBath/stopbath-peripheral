# StopBath Pico Remote

A Raspberry Pi Pico 2 W with a Waveshare 4.2 inch e-paper module, acting as a
physical control surface for a StopBath appliance: present the join code,
start a session, end it, without a phone in the photographer's hand, and with
a code large enough for a stranger to scan at arm's length.

The Pico is a peripheral. It reports what physically happened and renders
what it is sent. StopBath remains authoritative for every session, every
guest and the meaning of every button. The specification is
`STOPBATH_PICO_SPEC.md`; it applies `../flipper/STOPBATH_FLIPPER_SPEC.md`
(the first peripheral) by reference and implements the protocol the StopBath
repository owns in `docs/peripheral/`. What this project asks of the
appliance is in the StopBath repository's `docs/PICO_REMOTE_HANDOFF.md`.

This directory is the `pico/` directory of the `stopbath-peripheral`
repository, which holds both peripherals and the code they share
(`../shared/`) since 2026-09-16; before that it was the `stopbath-pico`
repository, whose history is here in full. Everything below is run from
this directory.

## Status

`KE1` (foundation and first light) is done: the host tests, the typography
scan and the firmware build run on a machine with no Pico attached, and the
author cleared the hardware gate on 2026-09-15 (`HARDWARE_COMPATIBILITY.md`:
the V2 panel driver, both keys, short and long). `KE2` (layouts and the
refresh policy) is done and its gate cleared the same day: every state
judged on the panel, partial refresh about 480 ms with the code still, full
about 1580 ms, no residue. `KE3` (protocol library, session, development
peer) is done. `KE4` (the USB transport) is done and its gate cleared on the appliance's
own Pi: `dist/stopbath_pico.uf2` is the remote, speaking protocol version 1
over USB CDC, proven against the development peer through twenty three
cable pulls. `KE5` (the QR code) is done and its gate cleared: the code region draws a
real code, version 3 at 7 pixels a module for the appliance's Wi-Fi payload,
proven against an independent encoder's matrices and scanned from arm's
length on several phones. `KE6`, the first sessions against the real appliance, is done and its gate
cleared: sessions started and ended from the Pico and from the dashboard, a
guest joined and saw photographs from the panel's codes, and the
photographer's verdict is in `FIELD_NOTES.md`. Nothing the field showed
needed a change. Nothing in this repository claims a hardware gate
has passed except where that document records the author saying so.

The Wi-Fi radio is unused. A network transport is a later phase (spec Part 9).

## Hardware

| Item | Value | Source |
|---|---|---|
| Board | Raspberry Pi Pico 2 W (RP2350A, 4 MB flash) | pico-sdk `boards/pico2_w.h` |
| Display | Waveshare Pico-ePaper-4.2, 400 by 300, black and white | Waveshare wiki |
| Panel pins | DIN GP11, CLK GP10, CS GP9, DC GP8, RST GP12, BUSY GP13, on SPI1 | wiki pinout, schematic |
| Keys | KEY0 GP15, KEY1 GP17, to ground, internal pull ups | schematic |

Evidence for each is in `docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md`.

## Building the firmware

From this directory. Everything is pinned in `scripts/toolchain_versions.env`:
pico-sdk `2.3.1` and the Arm GNU Toolchain `15.2.rel1`. You need `cmake`,
`ninja` and `git` on PATH (on Windows, `scoop install cmake ninja`, and run
the scripts from Git Bash). The setup script downloads the compiler into
`.toolchain/` (never committed), verifies Arm's published digest, and finds
or clones the SDK at the pinned commit. The build compiles the shared code
from `../shared/` as static libraries (`firmware/CMakeLists.txt`).

```bash
bash scripts/setup_toolchain.sh
```

```bash
bash scripts/build_firmware.sh
```

The result is `dist/stopbath_pico.uf2`, the remote; and two check images,
`dist/stopbath_pico_first_light.uf2` (KE1, the panel and keys) and
`dist/stopbath_pico_layout_demo.uf2` (KE2, every display state and the
refresh policy; controls in `HARDWARE_COMPATIBILITY.md`). To flash: hold BOOTSEL
on the Pico, plug it into the PC, release, and copy the `.uf2` onto the
`RP2350` drive that appears. The Pico restarts into it. Use a micro USB cable
that carries data; two of the author's charging cables produced no drive at
all (`HARDWARE_COMPATIBILITY.md`).

## Host tests

The pure logic under `remote_input/`, `remote_display/` and `session_device/` has
no SDK dependency and is tested on the development machine with `gcc` and
`make`, from this directory:

```bash
make test
```

The shared suites (protocol, peer, link table) under this device's host
flag set, built into this directory's `build/`:

```bash
make test-shared
```

The sanitiser build needs a compiler with `libasan` and `libubsan`, which the
MinGW `gcc` on Windows does not ship. Run it under WSL or on Linux:

```bash
make test-sanitise
```

A first look at the link from this PC, with the remote flashed and on a COM
port (it opens the port with DTR, answers the HELLO, and prints what the
keys send for twenty seconds):

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\link_check.ps1 -Port COM8
```

The protocol checks (the generated tables against `protocol.json`, and
`protocol.json` against the appliance's frozen definition) and the typography
scan required by Flipper 0.8 live under `../shared/scripts/` and run over the
whole tree; these targets forward to them:

```bash
make check-protocol-tables check-protocol-definition check-typography PYTHON="py -3"
```

The fuzz harness and the development peer build from `../shared/`
(`make -C ../shared fuzz`, `make peer` here forwards).

## Layout

| Path | Contents |
|---|---|
| `firmware/` | the pico-sdk facing application, kept thin: the remote and the two check programs, the keys, the hardware layer the vendored driver expects, the non-blocking panel driving and refresher, the USB CDC link and descriptors, the build |
| `remote_input/` | pure logic: two keys, debounce, short and long classification, no SDK |
| `remote_display/` | pure logic: the frame buffer in the panel's packing, a bitmap font, the layout with its regions, the fixtures, the refresh policy, no SDK |
| `session_device/` | this device's side of the shared client session (`../shared/session/`): the token, the fixed guard values, the keys to events including the Key1 choice, and the display composition, no SDK |
| `lib/waveshare/` | the vendored panel driver, unmodified, with provenance |
| `tests/` | this device's host tests; the harness and the published QR vectors are under `../shared/tests/` |
| `scripts/` | the toolchain setup and firmware build scripts, the version pin, the Windows link check |
| `../shared/` | held once for both devices: the protocol library and `protocol.json`, the link decision table (`link/`, once `transport/` here), the development peer, the fuzz harness, the test harness, the vendored QR encoder, the typography scan and the tree wide checks; see `../shared/PROTOCOL.md` and `../shared/TESTING.md` |
| `docs/evaluation/` | the Plan stage evidence log |

## Documents

`STOPBATH_PICO_SPEC.md`, `PROTOCOL.md`, `TESTING.md`,
`IMPLEMENTATION_DEVIATIONS.md`, `HARDWARE_COMPATIBILITY.md`,
`FIELD_NOTES.md` and
`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md` are the documents the
specification requires. Each says what it is for at the top.

## Licence

MIT. See `LICENSE`. Code copied from the `stopbath-flipper` repository is
MIT and says where it came from in its header; the vendored Waveshare
drivers carry their own permission notice.
