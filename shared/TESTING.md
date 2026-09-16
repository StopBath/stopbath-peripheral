# Testing under shared/

Nothing here needs a device, so there are no testing deviations in this
directory (peripheral spec Part 6). Every suite is a table driven C program
run by `tests/test_support.h`, a case with no assertions fails, and every
suite runs in well under a second. Both device directories run the same
suites again under their own host flag set (`make test-shared` in each), and
the device images compile the same sources under the real toolchains.

## Suites and checks

| What | Taken from | Command, from `shared/` |
|---|---|---|
| protocol suite: every verb round trips, every bound, every refusal, no allocation in the parse or encode path | both devices, identical | `make test` |
| development peer suite: answers the handshake, drives every display state, records every event, produces each misbehaviour mode, bounded log; bytes in through `development_peer_feed`, so the shared protocol layer is driven through a byte pipe | both devices, identical | `make test` |
| link edge suite: the decision table total over every prior state and observation, opens only on cable and DTR together, each edge once, a pull and reinsertion a close then an open; plus the three Flipper hardware findings as named cases | the Pico, extended here | `make test` |
| probe report suite: what the peer's shell says while it searches for the application's node (a denied open once per node per search then silent retry, a node that opens but sends no HELLO named after three passes, a find resets, every message fits, no allocation) | this repository, from the Flipper's `SE4` gate run | `make test` |
| the development peer itself, a host tool (POSIX shell, termios) | both devices | `make peer` (Linux or WSL) |
| the same under the address and undefined behaviour sanitisers | both devices | `make test-sanitise` (Linux or WSL; the MinGW compiler on Windows cannot link the sanitiser runtime, as both devices' `TESTING.md` already record) |
| fuzz: the parser against 200000 deterministic hostile inputs | both devices, identical | `make fuzz`, and `make fuzz-sanitise` under the sanitisers |
| both devices' warning flag sets, optimisation level, `NDEBUG` and short enums, approximated on the host | this repository (evaluation log 4.1, 4.5) | `make check-device-flags` |
| the generated tables match `protocol.json` | the Pico | `make check-protocol-tables` |
| `protocol.json` matches the appliance's frozen digest | the Pico | `make check-protocol-definition` |
| typography: no em dash, no en dash, no mangled hyphen run, over the whole tree | both devices, union of their sets | `make check-typography` |
| one copy: no file under `shared/` is duplicated under a device directory | this repository (peripheral spec 0.18) | `make check-one-copy` |
| includes: no device includes the other; `shared/` includes only itself and the C standard library | this repository (peripheral spec 0.18, Part 3) | `make check-includes` |
| all of the above except the sanitiser build | | `make check` |

On Windows the Python scripts need `make PYTHON="py -3" ...`, because the
`python3` on `PATH` there is the Windows Store alias.

## What the devices still test themselves

The published QR vectors in `tests/qr_published_vectors.h` are shared data,
but the test that proves an encoder against them drives each device's own QR
wrapper (`remote_display/remote_qr.c`, the display facts), so
`tests/test_remote_qr_vectors.c` lives in each device directory and includes
the header from here. It moves here when `SD5` adds the pixel free encode
step. Recorded in the root `IMPLEMENTATION_DEVIATIONS.md`.

## Continuous integration

`.github/workflows/ci-shared.yml` runs every row above on every push and
pull request, not path filtered, on `ubuntu-latest`. The device workflows
(`ci-flipper.yml`, `ci-pico.yml`) run `make test-shared` from inside their
directories as well, and build the images.
