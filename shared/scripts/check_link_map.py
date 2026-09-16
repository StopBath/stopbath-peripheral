#!/usr/bin/env python3
"""
A device build must exclude shared/tests/, shared/fuzz/ and shared/peer/
(peripheral spec 2.4), and a build system that gathered sources by glob could
silently compile a host test into an image. This check reads each image's
linker map and fails if any input object came from one of those directories,
or if any symbol the map names belongs to the test harness, the fuzz driver
or the development peer. One check for both devices (peripheral spec SE3 and
SE4): the Pico's maps are under pico/build/firmware/, the Flipper's is
flipper/.ufbt/build/stopbath_remote_d.elf.map, and each device's job names
its own.

Usage:
    python3 ../shared/scripts/check_link_map.py build/firmware/stopbath_pico.elf.map ...
    python3 ../shared/scripts/check_link_map.py .ufbt/build/stopbath_remote_d.elf.map

Exit status is zero when every map named is clean, one otherwise, and one
with a message when no map is named or a named map is missing.
"""

import re
import sys
from pathlib import Path

# Object paths from these directories under shared/ must never appear. CMake
# writes an object as shared/CMakeFiles/<target>.dir/<directory>/<file>.obj
# and ufbt as .../lib/shared/<directory>/<file>.o, so the target segment is
# optional in the pattern, with either separator.
FORBIDDEN_OBJECT_PATH = re.compile(r"shared[/\\](?:CMakeFiles[/\\][^/\\]+\.dir[/\\])?(tests|fuzz|peer)[/\\]")

# ufbt links the shared sources as one archive and the map names its members
# without their directory, shared(remote_protocol.o), so a member is judged
# by the file name the harness, the fuzz driver and the peer sources have.
FORBIDDEN_ARCHIVE_MEMBER = re.compile(r"shared\((test_|fuzz_|development_peer_)[^)]*\)")

# Symbol prefixes owned by the harness, the fuzz driver and the peer.
FORBIDDEN_SYMBOL_PREFIXES = ("remote_test_", "fuzz_remote_", "development_peer_")

SYMBOL_LINE = re.compile(r"^\s+0x[0-9a-fA-F]+\s+(\S+)\s*$")


def defects_in(map_path: Path):
    for line_number, line in enumerate(map_path.read_text(encoding="utf-8", errors="replace").splitlines(), start=1):
        object_match = FORBIDDEN_OBJECT_PATH.search(line)
        if object_match:
            yield line_number, f"object from shared/{object_match.group(1)}/: {line.strip()}"
            continue
        member_match = FORBIDDEN_ARCHIVE_MEMBER.search(line)
        if member_match:
            yield line_number, f"archive member {member_match.group(0)}: {line.strip()}"
            continue
        symbol_match = SYMBOL_LINE.match(line)
        if symbol_match and symbol_match.group(1).startswith(FORBIDDEN_SYMBOL_PREFIXES):
            yield line_number, f"symbol {symbol_match.group(1)}"


def main(arguments) -> int:
    map_paths = [Path(argument) for argument in arguments]
    if not map_paths:
        print("name at least one linker map file")
        return 1
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
