#!/usr/bin/env python3
"""
The include rules of the tree, from the peripheral specification:

1. flipper/ and pico/ depend on shared/ and on nothing in each other (0.18):
   no include in a device directory may resolve into the other device's
   directory.
2. Every directory under shared/ other than lib/ is free of any header that
   is not this repository's own or the C standard library's (Part 3, 0.16):
   a quoted include in shared/ must resolve inside shared/, and an angle
   bracket include must name a C standard header. The peer shell's termios
   is the one POSIX exception and is listed by file.

Includes are resolved textually, relative to the including file, which is
how every include in this tree is written. An include the compiler finds
through a search path (the device wrappers' <qrcodegen.h>) is an angle
bracket include of a device file and is outside rule 2.

Usage:
    python3 shared/scripts/check_includes.py    # exit 1 on any violation
"""

import os
import re
import sys
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
SHARED_DIRECTORY = REPOSITORY_ROOT / "shared"
DEVICE_DIRECTORIES = {
    "flipper": REPOSITORY_ROOT / "flipper",
    "pico": REPOSITORY_ROOT / "pico",
}

EXCLUDED_DIRECTORY_NAMES = {".git", ".ufbt", ".toolchain", "build", "dist", "__pycache__", ".idea", ".vscode"}
SOURCE_SUFFIXES = {".c", ".h"}

QUOTED_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
ANGLE_INCLUDE = re.compile(r"^\s*#\s*include\s*<([^>]+)>")

# ISO C headers through C23. Nothing else reaches shared/.
C_STANDARD_HEADERS = {
    "assert.h", "complex.h", "ctype.h", "errno.h", "fenv.h", "float.h",
    "inttypes.h", "iso646.h", "limits.h", "locale.h", "math.h", "setjmp.h",
    "signal.h", "stdalign.h", "stdarg.h", "stdatomic.h", "stdbit.h",
    "stdbool.h", "stdckdint.h", "stddef.h", "stdint.h", "stdio.h",
    "stdlib.h", "stdnoreturn.h", "string.h", "tgmath.h", "threads.h",
    "time.h", "uchar.h", "wchar.h", "wctype.h",
}

# Files under shared/ allowed headers beyond the C standard, each with the
# reason. The peer shell is a host tool and is never part of a device build.
SHARED_EXCEPTIONS = {
    "peer/development_peer_shell.c": {"termios.h", "unistd.h", "fcntl.h", "poll.h", "sys/select.h", "sys/time.h", "sys/types.h"},
}


def source_files_under(root: Path):
    for directory, directory_names, file_names in os.walk(root, followlinks=False):
        directory_names[:] = [name for name in directory_names if name not in EXCLUDED_DIRECTORY_NAMES]
        for file_name in file_names:
            file_path = Path(directory) / file_name
            if file_path.suffix in SOURCE_SUFFIXES and not file_path.is_symlink():
                yield file_path


def is_under(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
        return True
    except ValueError:
        return False


def check_device_includes() -> int:
    violation_count = 0
    for device_name, device_directory in DEVICE_DIRECTORIES.items():
        other_directories = [other for other_name, other in DEVICE_DIRECTORIES.items() if other_name != device_name]
        for source_file in source_files_under(device_directory):
            for line_number, line in enumerate(source_file.read_text(encoding="utf-8").splitlines(), start=1):
                match = QUOTED_INCLUDE.match(line)
                if not match:
                    continue
                resolved = (source_file.parent / match.group(1)).resolve()
                for other in other_directories:
                    if is_under(resolved, other):
                        print(f"{source_file.relative_to(REPOSITORY_ROOT)}:{line_number}: includes the other device's {resolved.relative_to(REPOSITORY_ROOT)}")
                        violation_count += 1
    return violation_count


def check_shared_includes() -> int:
    violation_count = 0
    for source_file in source_files_under(SHARED_DIRECTORY):
        relative = source_file.relative_to(SHARED_DIRECTORY).as_posix()
        if relative.startswith("lib/"):
            continue
        allowed_beyond_standard = SHARED_EXCEPTIONS.get(relative, set())
        for line_number, line in enumerate(source_file.read_text(encoding="utf-8").splitlines(), start=1):
            quoted = QUOTED_INCLUDE.match(line)
            if quoted:
                resolved = (source_file.parent / quoted.group(1)).resolve()
                if not is_under(resolved, SHARED_DIRECTORY):
                    print(f"shared/{relative}:{line_number}: includes {quoted.group(1)}, which is outside shared/")
                    violation_count += 1
                continue
            angled = ANGLE_INCLUDE.match(line)
            if angled and angled.group(1) not in C_STANDARD_HEADERS and angled.group(1) not in allowed_beyond_standard:
                print(f"shared/{relative}:{line_number}: <{angled.group(1)}> is not a C standard header")
                violation_count += 1
    return violation_count


def main() -> int:
    violation_count = check_device_includes() + check_shared_includes()
    if violation_count:
        print(f"{violation_count} include violation(s) found")
        return 1
    print("includes: no device includes the other, and shared/ includes only itself and the C standard library")
    return 0


if __name__ == "__main__":
    sys.exit(main())
