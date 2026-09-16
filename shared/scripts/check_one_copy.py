#!/usr/bin/env python3
"""
One tree, one copy (peripheral spec 0.18): a file that both devices need
lives under shared/ and nowhere else. This scan fails when any file under
shared/ has a file of the same name and the same content under a device
directory, which is what a copy left behind by an extraction, or a copy
made instead of a reference, looks like.

Same name and same content, both, so that a device file that merely shares a
name with a shared one (a device's own Makefile against shared/Makefile) is
not a finding, and a shared file that a device has edited its own copy of is
caught by the include check and the build rather than here.

The device directories are walked without following symbolic links, because
flipper/lib/shared is the Flipper build's way into shared/ and is not a copy.

Usage:
    python3 shared/scripts/check_one_copy.py    # exit 1 if any copy exists
"""

import hashlib
import os
import sys
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
SHARED_DIRECTORY = REPOSITORY_ROOT / "shared"
DEVICE_DIRECTORIES = [REPOSITORY_ROOT / "flipper", REPOSITORY_ROOT / "pico"]

# Tool state and build output, never part of either tree.
EXCLUDED_DIRECTORY_NAMES = {".git", ".ufbt", ".toolchain", "build", "dist", "__pycache__", ".idea", ".vscode"}


def digest_of(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def files_under(root: Path):
    for directory, directory_names, file_names in os.walk(root, followlinks=False):
        directory_names[:] = [name for name in directory_names if name not in EXCLUDED_DIRECTORY_NAMES]
        for file_name in file_names:
            file_path = Path(directory) / file_name
            if not file_path.is_symlink():
                yield file_path


def main() -> int:
    shared_digests = {}
    for shared_file in files_under(SHARED_DIRECTORY):
        shared_digests.setdefault(shared_file.name, {})[digest_of(shared_file)] = shared_file

    copy_count = 0
    for device_directory in DEVICE_DIRECTORIES:
        for device_file in files_under(device_directory):
            same_name = shared_digests.get(device_file.name)
            if not same_name:
                continue
            shared_file = same_name.get(digest_of(device_file))
            if shared_file is not None:
                print(f"{device_file.relative_to(REPOSITORY_ROOT)}: copy of {shared_file.relative_to(REPOSITORY_ROOT)}")
                copy_count += 1

    if copy_count:
        print(f"{copy_count} copy/copies of shared files found under device directories")
        return 1
    print("one copy: no shared file is duplicated under a device directory")
    return 0


if __name__ == "__main__":
    sys.exit(main())
