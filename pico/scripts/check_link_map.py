#!/usr/bin/env python3
"""
The device build must exclude ../shared/tests/, ../shared/fuzz/ and
../shared/peer/ (peripheral spec 2.4), and a build system that gathered
sources by glob could silently compile a host test into the firmware. This
check reads each image's linker map and fails if any input object came from
one of those directories, or if any symbol the map names belongs to the test
harness, the fuzz driver or the development peer.

Usage:
    python3 scripts/check_link_map.py                 # the three images under build/firmware
    python3 scripts/check_link_map.py path/to/a.map   # any map files named

Exit status is zero when every map is clean and one otherwise.
"""

import re
import sys
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MAPS = [
    REPOSITORY_ROOT / "build" / "firmware" / "stopbath_pico.elf.map",
    REPOSITORY_ROOT / "build" / "firmware" / "stopbath_pico_first_light.elf.map",
    REPOSITORY_ROOT / "build" / "firmware" / "stopbath_pico_layout_demo.elf.map",
]

# Object paths from these directories under shared/ must never appear. CMake
# writes an object as shared/CMakeFiles/<target>.dir/<directory>/<file>.obj,
# so the target segment is optional in the pattern, with either separator.
FORBIDDEN_OBJECT_PATH = re.compile(r"shared[/\\](?:CMakeFiles[/\\][^/\\]+\.dir[/\\])?(tests|fuzz|peer)[/\\]")

# Symbol prefixes owned by the harness, the fuzz driver and the peer.
FORBIDDEN_SYMBOL_PREFIXES = ("remote_test_", "fuzz_remote_", "development_peer_")

SYMBOL_LINE = re.compile(r"^\s+0x[0-9a-fA-F]+\s+(\S+)\s*$")


def defects_in(map_path: Path):
    for line_number, line in enumerate(map_path.read_text(encoding="utf-8", errors="replace").splitlines(), start=1):
        object_match = FORBIDDEN_OBJECT_PATH.search(line)
        if object_match:
            yield line_number, f"object from shared/{object_match.group(1)}/: {line.strip()}"
            continue
        symbol_match = SYMBOL_LINE.match(line)
        if symbol_match and symbol_match.group(1).startswith(FORBIDDEN_SYMBOL_PREFIXES):
            yield line_number, f"symbol {symbol_match.group(1)}"


def main(arguments) -> int:
    map_paths = [Path(argument) for argument in arguments] or DEFAULT_MAPS
    defect_count = 0
    for map_path in map_paths:
        if not map_path.is_file():
            print(f"{map_path}: missing; build the firmware first")
            defect_count += 1
            continue
        for line_number, reason in defects_in(map_path):
            print(f"{map_path}:{line_number}: {reason}")
            defect_count += 1
    if defect_count:
        print(f"{defect_count} link map defect(s): the image carries shared test, fuzz or peer code")
        return 1
    print(f"link maps clean: no shared test, fuzz or peer code in {len(map_paths)} image(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
