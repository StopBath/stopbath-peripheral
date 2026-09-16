# The protocol, as held under shared/

The StopBath appliance owns the peripheral protocol. Its definition is
`docs/peripheral/protocol.json` in the StopBath repository, frozen at `FD20`
on 2026-09-12, and the prose that explains it is `docs/peripheral/PROTOCOL.md`
there. Neither is restated here (peripheral spec 0.17, 1.3): this directory
holds a copy of the definition and the one C implementation of its peripheral
side, and defines nothing.

## What this directory holds

| Path | What it is |
|---|---|
| `protocol.json` | the appliance's frozen definition, byte for byte; sha256 `5793e16a0b97cd0c122f53c15a1a76b2272a595df0788d0edf065c02b9009d4a` |
| `scripts/check_protocol_definition.py` | holds the copy to that digest; a change to the definition is a new protocol version decided in the StopBath repository, never here |
| `scripts/generate_protocol_tables.py` | generates `protocol/remote_protocol_tables.h` and `.c` from `protocol.json`; `--check` fails if they differ from what the table says |
| `protocol/remote_protocol.c`, `.h` | the parser and encoder: bounded, table driven, no allocation, untrusted bytes in |
| `protocol/remote_protocol_tables.c`, `.h` | generated; every bound the parser enforces comes from the table through the generator, never by hand (peripheral spec Part 5) |
| `fuzz/fuzz_remote_protocol.c` | the parser's proof against hostile bytes, run under the sanitisers on every change |
| `tests/test_remote_protocol.c` | the protocol suite, run from here and again from each device directory under that device's host flags |

Both devices compile `protocol/` from here by relative path (peripheral spec
2.4): the Flipper through `flipper/lib/shared`, a symlink to this directory
named in `flipper/application.fam`; the Pico through
`add_subdirectory(../shared)` in `pico/firmware/CMakeLists.txt` and the
`stopbath_shared_protocol` target.

## What is not here

What any button means. Interpretation is the appliance's (extension 2.1),
and a device's own choice, such as the Pico's Key1 mapping, is that device's
recorded deviation living under its directory. The session, which decodes
records into what a display shows, is each device's own until `SE5` moves the
display agnostic part here.

## Checking

From this directory, with `python3` (or `make PYTHON="py -3"` on Windows):

```bash
make check-protocol-definition
```

```bash
make check-protocol-tables
```
