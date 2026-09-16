# Implementation deviations

Deviations from `STOPBATH_PERIPHERAL_SPEC.md`, each with the rule it departs
from, a one line justification and what covers it. The devices keep their
own files for deviations from their own specifications.

## SE1

### 1. `flipper/lib/shared` is a symlink into `shared/`

Rule: 2.4, "reached by a relative path (`../shared/...`) and by no other
means: no copy, no symlink into the device tree unless Appendix A finds the
Flipper's build tool needs one, in which case the symlink is recorded as a
deviation".

Appendix A found it needs one: a `fap_private_libs` entry's sources are only
ever gathered from under the application's `lib/` (evaluation log 4.2,
experiments 2 to 4, real `ufbt` runs). The symlink `flipper/lib/shared` to
`../../shared` is the relative path, expressed the one way the build tool can
follow it. Chosen by the author over computed absolute paths (decision A).

Cost, recorded in `flipper/README.md` at `SE3` or `SE4`: on Windows the link
needs Developer Mode (or an elevated shell) to be created and
`git config core.symlinks true` before the clone, or it checks out as a text
file. Linux and continuous integration need nothing.

Covered by: the FAP build in `ci-flipper.yml`, and the one copy check, which
walks without following the link.

### 2. One `Lib` entry, not one per shared module

Rule: Part 3, "for the Flipper it is one source directory per private
library entry".

Each `Lib` entry's sources are gathered from `lib/<its name>`, so one entry
per module would need one symlink per module. One entry named `shared` lists
each module's sources explicitly instead (`protocol/*.c`,
`lib/qrcodegen/qrcodegen.c`), and a device that needs fewer modules drops
their lines; the separability the rule wants is by source line rather than by
entry. The Pico side keeps one CMake target per module as the rule says.

Covered by: `flipper/application.fam` names no directory of `shared/` as a
whole, so `shared/tests`, `fuzz` and `peer` cannot reach the image; the link
map check at `SE3` proves it.

### 3. The QR vectors test and its generator stay with the Flipper

Rule: 2.2, "`tests/qr_published_vectors.h` and its test | both, identical".

The header is shared and lives under `shared/tests/`. The test that consumes
it, `tests/test_remote_qr_vectors.c`, drives each device's own
`remote_qr_encode()`, the geometry bearing wrapper 2.3 keeps per device, so
under `shared/` it would include a device header. It stays in each device's
`tests/`, including the shared header by relative path, until `SD5` adds the
pixel free encode step. `flipper/scripts/generate_qr_vectors.sh` compiles the
Flipper's wrapper for the same reason and stays with it, writing the header
into `shared/tests/`.

Covered by: the include check (the shared header has no device dependence);
both devices' vector suites, green from inside their directories.

### 4. The Pico's toolchain directory is renamed by `setup_toolchain.sh`

Rule: `SE1` "Excluded: any change to any moved file beyond a path in an
include line", and the general shape that `SE1` changes device builds only to
re-point them at `shared/`.

`pico/scripts/setup_toolchain.sh` now unpacks the Arm toolchain into
`.toolchain/arm-gnu-toolchain-<version>` rather than a directory named after
the archive. With the extra `stopbath-peripheral/pico/` in the path, the
compiler's unnormalised C++ include path crossed Windows' 260 character limit
and pico-sdk's `new_delete.cpp` could not find `<bits/c++config.h>`; the old
location was 8 characters under the limit. A consequence of the move, fixed
where it arises. Nothing else in the script changed; the version is pinned
exactly as before.

Covered by: the firmware build from `pico/` on Windows, evaluation log SE1,
and `ci-pico.yml` on Linux.

### 5. The device workflow files are removed

Rule: `SE0` and `SE1` leave the device directories as they were beyond the
re-pointing.

GitHub reads workflows only from the root `.github/workflows/`, so
`flipper/.github/workflows/ci.yml` and `pico/.github/workflows/ci.yml` were
inert from the moment of the import. Their jobs live on in `ci-flipper.yml`
and `ci-pico.yml` at the root, run from inside the device directories.
Keeping the old files would have described a check that never runs.

### 6. `pico/peer/PROVENANCE.md` was deleted at `SE2`, not `SE1`

Rule: Appendix C, "Deleted at `SE1`: ... `pico/peer/PROVENANCE.md`".

The peer moved at `SE2`. Deleting its provenance record one tag before the
peer moved would have left the Pico's copy unrecorded for that tag, so the
file went with the peer, its record carried into the root `PROVENANCE.md`.

### 7. The typography scan changed beyond the union of the two sets

Rule: `SE1` "Excluded: any change to any moved file beyond a path in an
include line" (the union of sets itself is decision C in the evaluation log).

`shared/scripts/check_typography.py` walks with `os.walk(followlinks=False)`
instead of `Path.rglob`, because the tree now contains a directory symlink
and following it would scan `shared/` twice, and because whether `rglob`
follows links differs between the Python versions the machine and the
runner have.

### 8. The device documents are stale until `SE3` and `SE4`

Rule: Appendix C schedules every edit to the device documents at the
device's proving phase.

The Pico's were amended at `SE3` (2026-09-16). Until `SE4`,
`flipper/TESTING.md` names `make fuzz`, `make check-protocol-tables` running
a local script, and `tests/test_support.h` as local files. `make fuzz` no
longer exists in the device Makefile (the fuzz harness runs from `shared/`);
the other commands still work from the device directory through the
forwarding targets in its Makefile.

## SE3

### 9. The Pico's specification records the move as dated amendments

Rule: Appendix C names the sentences to change in `pico/STOPBATH_PICO_SPEC.md`.

They are changed, but the phase texts that describe copying (`KE1`, `KE3`)
are kept as written and carry a dated parenthesis saying where the copies
went, rather than being rewritten as if the copies never existed. A phase
record is history; the layout table, the `PROVENANCE.md` rule and the Part
10 guidance, which state the present, are rewritten outright.
