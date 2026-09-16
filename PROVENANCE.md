# Provenance

Where every file in this repository came from, written once at `SE0` (the
import) and extended once at `SE1` and `SE2` (the extraction of the shared
code). After that the repository's own history is the provenance and this
file does not grow.

## The import (`SE0`)

Both device repositories were imported with their full histories on
2026-09-16, every path rewritten under a device directory, and merged into
this repository as two merges with unrelated histories, tagged `v0.1.0`.

| Directory | Source repository | Last commit there | Commits | Files | Import merge here |
|---|---|---|---|---|---|
| `flipper/` | `https://github.com/TheScottBot/stopbath-flipper` | `815bd690a122d7d2af3f07c628f933cdb02600c3` (2026-09-16, "Added missing spec") | 19 | 68 | `a4cfbf0b76cadebc1dff26b1a4cea5c74dbb684b` |
| `pico/` | `https://github.com/StopBath/stopbath-pico` | `641b869c2cb0aa5c9a436455e9131845075f65ac` (2026-09-15, "docs(performance): record appliance acceptance timing") | 14 | 87 | `8514e4e80a2e1792c17de7ac7954d5336b3ba203` |

The rewrite was `git filter-repo --to-subdirectory-filter <directory>` on a
fresh `--no-local` clone of each repository (`git-filter-repo` 2.47.0, commit
`a40bce548d2c`, from scoop; `git 2.55.0.windows.3`). It changes paths and
nothing else: every blob under `flipper/` and `pico/` has the hash it had in
the source repository, and every commit keeps its author, date and message.
The rehearsal that proved this, and the exact command sequence, are in
`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md` 4.4.

Each device directory therefore contains, byte for byte, what its repository's
`main` contained at the commit above, including that repository's own
`.gitignore` and `.gitattributes`, which git applies beneath the directory.
The untracked tool state each repository kept (`flipper/.ufbt/`,
`pico/.toolchain/`, both `build/` and `dist/`) was never in git and is not
here; each is recreated by that directory's build instructions.

## Extraction into `shared/` (`SE1`)

On 2026-09-16 the files both devices carried were moved into `shared/`, one
copy each, and the other copy deleted. The decision per file is evaluation
log 4.3: the Pico's copy for everything that differed, since it carried the
frozen definition and the digest check; where the two copies were identical
the choice is immaterial and the Pico's was moved. Digests are of the bytes
in the tree (LF line endings, as `.gitattributes` requires).

| Now at | Became `shared/`'s from | Deleted | sha256 in the tree |
|---|---|---|---|
| `shared/protocol/remote_protocol.c` | `pico/protocol/` (identical to the Flipper's) | `flipper/protocol/` | `c11d3214f83da2527514253b7929a3009ee0514cf8c33bae779ea97bb68ff5f8` |
| `shared/protocol/remote_protocol.h` | `pico/protocol/` (identical) | `flipper/protocol/` | `020d58d1e86ddae7f162c89c51ffda118a842efaa569fd41b6f9d477d08f3518` |
| `shared/protocol/remote_protocol_tables.c` | `pico/protocol/` (identical) | `flipper/protocol/` | `2e94c041402abfab14b940b907c03042a0abbf293389610d620e9a1f8eaebbd8` |
| `shared/protocol/remote_protocol_tables.h` | `pico/protocol/` (the Flipper's lacked the `maximum_wifi_payload_length_for_qr` line) | `flipper/protocol/` | `69eec5a3661ebce005c7b6e4a2b41e2a5015aaeffb49f8918fe029311babc63f` |
| `shared/protocol.json` | `pico/` (the appliance's frozen definition; the Flipper's was the pre-freeze draft) | `flipper/protocol.json` | `5793e16a0b97cd0c122f53c15a1a76b2272a595df0788d0edf065c02b9009d4a` |
| `shared/scripts/generate_protocol_tables.py` | `pico/scripts/` (identical) | `flipper/scripts/` | `51b0f6df936ead403f8d4eacd114607db805fb73e304bf2077b7f181e4951cbb` |
| `shared/scripts/check_protocol_definition.py` | `pico/scripts/` (Pico only) | nothing | `f2d537e8614119ff5fea420f54347b5e637acdcb4525034e11b4ba2593d2e907` |
| `shared/scripts/check_typography.py` | the union of both copies' exclusion and file sets (evaluation log decision C), walking without following symlinks | `flipper/scripts/`, `pico/scripts/` | see the file's docstring |
| `shared/fuzz/fuzz_remote_protocol.c` | `pico/fuzz/` (identical) | `flipper/fuzz/` | `a8af169e7f8f3f1facd98c3eee148230a183f7d242da206e4eadd81096d30426` |
| `shared/tests/test_support.h` | `flipper/tests/` (the Pico's added a provenance comment, code identical) | `pico/tests/` | `afe73945b79612eadf21c763ed76712db5e1f54f33295581d8c121b989e7c6fb` |
| `shared/tests/test_remote_protocol.c` | `pico/tests/` (identical) | `flipper/tests/` | `57a1711f1f8bc48ff980eb8b7f37ef81cfa47f88f5dbadc81d566e5a9e027f3b` |
| `shared/tests/qr_published_vectors.h` | `flipper/tests/` (identical) | `pico/tests/` | `03f521fdaf9bd695611f36741c9c38eece73449d225cac01f55441be7f903f30` |
| `shared/lib/qrcodegen/qrcodegen.c`, `.h`, `LICENSE.txt` | `pico/lib/qrcodegen/` (identical) | `flipper/lib/qrcodegen/` | see below |

Also deleted at `SE1`: `pico/protocol/PROVENANCE.md` and
`pico/lib/qrcodegen/PROVENANCE.md`, whose records are carried here. The
Flipper's copies of these had no provenance file of their own; the Flipper
was the origin, and its `docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md`
carries the dependency record for the encoder. `pico/peer/PROVENANCE.md` goes
at `SE2` with the peer.

### The QR encoder

Vendored verbatim, unmodified, from Project Nayuki's QR Code generator, the C
implementation. Not to be edited: a fix goes upstream or into a device's
wrapper (Flipper 0.12).

| Field | Value |
|---|---|
| Source | https://github.com/nayuki/QR-Code-generator |
| Pinned at | tag `v1.8.0`, commit `720f62bddb7226106071d4728c292cb1df519ceb` (2022-04-17) |
| Files | `c/qrcodegen.c`, `c/qrcodegen.h`; the MIT text from the upstream readme as `LICENSE.txt` |
| Licence | MIT, the same as this repository |
| Path here | first `stopbath-flipper/lib/qrcodegen/` (vendored 2026-09-11 under Flipper 0.12), then `stopbath-pico/lib/qrcodegen/` byte for byte on 2026-09-15, now `shared/lib/qrcodegen/` |

Two digest forms, because upstream ships the two sources with CRLF line
endings and git stores them LF normalised in this tree:

| File | sha256 as upstream ships it (CRLF; what the Pico's record named) | sha256 in this tree (LF) |
|---|---|---|
| `qrcodegen.c` | `9b2f2f9dc36dc1804be715b729d820fbfa26a8e4e1aa5316511c1971aaebb58a` | `300eff07ee25baaa7578f20284411638154716379437391e7e689c0e6ce81403` |
| `qrcodegen.h` | `26e76f083ad582f246c5a4a002e34eaeb55e2f96a065552b6455424b04a03519` | `e82df4bff37d18b5863b9e7486fe6bda1b6cda8c3b9ecebfec473907265cb589` |
| `LICENSE.txt` | `527e09cc1cadbe29d0368a41e7fec6f728a19b03b3cb08839205a18a0d267166` | the same |

To verify against upstream, convert the tree's file to CRLF first
(`sed 's/$/\r/'`) and digest that; the CRLF column is what results.

What both devices rely on: `qrcodegen_encodeText` with caller supplied
buffers sized by `qrcodegen_BUFFER_LEN_FOR_VERSION` for the device's ceiling
version; no heap allocation anywhere in the library (its header says so and
the Flipper confirmed it by grep). It refuses, rather than overruns, a text
that does not fit the version range.

## Extraction into `shared/` (`SE2`)

On 2026-09-16 the development peer and the link edge decision table moved
into `shared/`. Every file is byte for byte what the import carried; the
digests match those the Pico's `peer/PROVENANCE.md` recorded from the
Flipper at `3677e61`, which is now deleted and carried here.

| Now at | Became `shared/`'s from | Deleted | sha256 in the tree |
|---|---|---|---|
| `shared/peer/development_peer_core.c` | `pico/peer/` (identical to the Flipper's) | `flipper/peer/` | `d00929f9eebcdc38929c4b537c32fc59bc1704a46ad4a061f3934620017cad2e` |
| `shared/peer/development_peer_core.h` | `pico/peer/` (identical) | `flipper/peer/` | `ab0f6ba7dd8bd4de79424927f3d590b8cc02b79ca7551b0a6168f9fb494901ba` |
| `shared/peer/development_peer_shell.c` | `pico/peer/` (identical) | `flipper/peer/` | `f8dcb5c7a22a69200898ce761770b1642e1ec3ed6bd1365b0edd54fb2af298c4` |
| `shared/tests/test_development_peer.c` | `pico/tests/` (identical) | `flipper/tests/` | `1680ef97def6e0f689992094e84269a03124a8f546473e06e76a48bd6d8cce1c` |
| `shared/link/remote_link_edge.c` | `pico/transport/` (Pico only; the Flipper's transport carries the same rule inline, `remote_transport.c:220-228`) | nothing | `b4e4715ea42419efa1612068d219f26ebc66e1332ecfd184997562ab8d76ec4f` |
| `shared/link/remote_link_edge.h` | `pico/transport/` | nothing | `186d21db38b6d262634da3dae59484dca9c4f2c33a237f7f494cb04251ab3a52` |
| `shared/tests/test_remote_link_edge.c` | `pico/tests/`, with three cases added for the Flipper's hardware findings | nothing | changed at `SE2`; the history is the record from here |

The peer is a test double on the terms of Flipper 2.9 and extension 1.4: it
interprets no button, holds no session, and emits what its shell is told to
emit. It is never part of a device image, which the link maps at `SE2`
confirm.

## Later

The session (`SE5`) is written fresh against both devices' session tests and
is not a move; it will have no provenance entry.
