#!/usr/bin/env python3
"""
Tree wide scan for specification 0.8 (Flipper 0.8, which the Pico and
peripheral specifications apply): no em dash in any tracked text file, and no
converter-mangled double or triple hyphen in prose either, since those are
the same defect wearing a different coat. Run once from the root over the
whole tree: shared/, flipper/, pico/ and the root documents alike.

Exemptions are exactly two, chosen because each is a place a run of hyphens
has an unambiguous non-prose meaning:

1. A Markdown table separator row, which consists only of pipes, hyphens,
   colons and spaces.
2. A command line style flag: a hyphen run at the start of a token that is
   immediately followed by a letter or digit, such as --url or --hw-target.
   This also covers a C prefix decrement, which is unavoidable in the language.

Everything else that contains two or more consecutive hyphens fails, including
a postfix decrement in C, which this codebase therefore does not use.

Exit status is zero when clean and one when any defect is found, with each
defect printed as path:line: reason.

The excluded directory set and the text file set are the union of what the
two devices' copies of this scan named (peripheral evaluation log 4.3,
decision C): every device's tool state and build output is skipped, and
every kind of text file either device commits is scanned.
"""

import os
import re
import sys
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent

EXCLUDED_DIRECTORY_NAMES = {".git", ".ufbt", ".toolchain", "build", "dist", "__pycache__", ".idea"}

# Third party sources vendored verbatim (the build system reserves "lib" for
# them). Their digests are recorded, so they are not edited to satisfy the
# hyphen rule, which exists for converter mangled prose that they do not
# contain. The em dash rule still applies to them in full. Recorded in
# IMPLEMENTATION_DEVIATIONS.md.
THIRD_PARTY_DIRECTORY_NAMES = {"lib"}

TEXT_FILE_SUFFIXES = {
    ".c",
    ".h",
    ".md",
    ".py",
    ".yml",
    ".yaml",
    ".fam",
    ".txt",
    ".json",
    ".toml",
    ".env",
    ".sh",
    ".cmd",
    ".cmake",
    ".ps1",
}
TEXT_FILE_NAMES = {"Makefile", "LICENSE", ".gitignore", ".env", ".gitattributes", ".editorconfig", "CMakeLists.txt"}

# Written as escapes so this file passes its own scan.
EM_DASH = "\u2014"
EN_DASH = "\u2013"

TABLE_SEPARATOR_ROW = re.compile(r"^\s*\|?[\s:|-]+\|?\s*$")
HYPHEN_RUN = re.compile(r"-{2,}")


def is_text_file(path: Path) -> bool:
    return path.suffix in TEXT_FILE_SUFFIXES or path.name in TEXT_FILE_NAMES


def iter_text_files(root: Path):
    # Symbolic links are never followed: flipper/lib/shared is the Flipper
    # build's way into shared/, and following it would scan shared/ twice.
    for directory, directory_names, file_names in os.walk(root, followlinks=False):
        directory_names[:] = sorted(name for name in directory_names if name not in EXCLUDED_DIRECTORY_NAMES)
        for file_name in sorted(file_names):
            path = Path(directory) / file_name
            if is_text_file(path) and not path.is_symlink():
                yield path


def hyphen_run_is_a_flag(line: str, match: re.Match) -> bool:
    starts_a_token = match.start() == 0 or line[match.start() - 1].isspace() or line[match.start() - 1] in "\"'`([=,"
    followed_by_word = match.end() < len(line) and (line[match.end()].isalnum() or line[match.end()] == "_")
    return starts_a_token and followed_by_word


def defects_in_line(line: str, third_party: bool):
    if EM_DASH in line:
        yield "em dash"
    if EN_DASH in line:
        yield "en dash"
    if third_party or TABLE_SEPARATOR_ROW.match(line):
        return
    for match in HYPHEN_RUN.finditer(line):
        if not hyphen_run_is_a_flag(line, match):
            yield f"hyphen run '{match.group(0)}' outside a flag or table row"


def main() -> int:
    defect_count = 0
    for path in iter_text_files(REPOSITORY_ROOT):
        third_party = any(part in THIRD_PARTY_DIRECTORY_NAMES for part in path.relative_to(REPOSITORY_ROOT).parts)
        try:
            content = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            print(f"{path.relative_to(REPOSITORY_ROOT)}: not valid UTF-8")
            defect_count += 1
            continue
        for line_number, line in enumerate(content.splitlines(), start=1):
            for reason in defects_in_line(line, third_party):
                print(f"{path.relative_to(REPOSITORY_ROOT)}:{line_number}: {reason}")
                defect_count += 1
    if defect_count:
        print(f"{defect_count} typography defect(s) found")
        return 1
    print("typography scan clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
