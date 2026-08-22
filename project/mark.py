#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path


def build_header(relative_path: Path, comment_prefix: str, comment_suffix: str = "") -> str:
    normalized = relative_path.as_posix()
    middle = f"--- file: {normalized} ---"
    if comment_suffix:
        return f"{comment_prefix} {middle} {comment_suffix}"
    return f"{comment_prefix} {middle}"


def update_file(
    file_path: Path,
    root: Path,
    comment_prefix: str,
    comment_suffix: str = "",
    dry_run: bool = False,
) -> bool:
    relative_path = file_path.relative_to(root)
    expected_header = build_header(relative_path, comment_prefix, comment_suffix)

    try:
        original = file_path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        print(f"Skipping non-UTF-8 file: {file_path}")
        return False

    lines = original.splitlines(keepends=True)
    newline = "\n"
    if lines:
        if lines[0].endswith("\r\n"):
            newline = "\r\n"
        elif lines[0].endswith("\n"):
            newline = "\n"

    changed = False
    marker_core = "--- file: "

    if lines:
        first_line = lines[0].rstrip("\r\n")
        if marker_core in first_line:
            if first_line != expected_header:
                lines[0] = expected_header + newline
                changed = True
        else:
            lines.insert(0, expected_header + newline)
            changed = True
    else:
        lines = [expected_header + newline]
        changed = True

    if changed and not dry_run:
        file_path.write_text("".join(lines), encoding="utf-8")

    return changed


def iter_files(root: Path, extension: str):
    extension = extension if extension.startswith(".") else f".{extension}"
    for path in root.rglob(f"*{extension}"):
        if path.is_file():
            yield path


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Ensure files have a standardized first-line file header."
    )
    parser.add_argument("root", type=Path, help="Root directory to scan recursively")
    parser.add_argument("extension", help="File extension to match, example: .h, .py, .md")
    parser.add_argument("comment_style", help='Comment prefix, example: c, py, html')
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would change without modifying files",
    )
    args = parser.parse_args()

    root = args.root.resolve()

    if not root.is_dir():
        raise SystemExit(f"Not a directory: {root}")

    changed_count = 0
    scanned_count = 0

    comment_prefix = ""
    comment_suffix = ""
    if args.comment_style == 'c':
        comment_prefix = "//"
    elif args.comment_style == "py":
        comment_prefix = "#"
    elif args.comment_style == "html":
        comment_prefix = "<!--"
        comment_suffix = "-->"

    for file_path in iter_files(root, args.extension):
        scanned_count += 1
        changed = update_file(
            file_path=file_path,
            root=root,
            comment_prefix=comment_prefix,
            comment_suffix=comment_suffix,
            dry_run=args.dry_run,
        )
        if changed:
            changed_count += 1
            action = "Would update" if args.dry_run else "Updated"
            print(f"{action}: {file_path}")

    print(f"Scanned {scanned_count} file(s), changed {changed_count} file(s).")


if __name__ == "__main__":
    main()
