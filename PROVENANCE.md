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

## Extraction into `shared/` (`SE1`, `SE2`)

Not yet done. When it is, this section records, per file, which device's copy
became `shared/`'s and which copy was deleted, carrying over the records that
were in `pico/protocol/PROVENANCE.md`, `pico/peer/PROVENANCE.md` and
`pico/lib/qrcodegen/PROVENANCE.md`, including the QR encoder's upstream
(Project Nayuki's QR Code generator, tag `v1.8.0`, commit
`720f62bddb7226106071d4728c292cb1df519ceb`).
