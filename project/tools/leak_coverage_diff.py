#!/usr/bin/env python
"""Diff two gcovr --json reports to find leak-sweep coverage gaps.

Usage: leak_coverage_diff.py full.json leak_targets.json [--top N]

full.json:          gcovr --json report from the full pytest suite.
leak_targets.json:  gcovr --json report from running ONLY the tests named
                     in test/jpypetest/leak_targets.txt (once each, no
                     batching -- this is a reachability signal, not a
                     leak measurement).

Reports, per native/ source file: how many lines the full suite executes
that no leak-sweep target executes at all. A high count means real,
reachable code with zero leak-check regression coverage -- a candidate
for a new leak_targets.txt entry (or, if the code isn't leak-prone by its
nature, at least a documented "not applicable" decision).

--memory-only restricts everything (both the summary percentage and the
gap list) to "memory-relevant" lines (see memory_relevant_lines.py) --
lines near an actual reference/resource acquire-or-release, not raw
line count across all of native/. This is the number that actually
matters: a giant mechanical dispatch table can sit at 0% leak-target
coverage forever without that meaning anything, while a small function
that acquires a reference and can throw matters a great deal even if
it's one line out of a 2000-line file.
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from memory_relevant_lines import memory_relevant_lines  # noqa: E402


def load_line_hits(path):
    with open(path) as f:
        data = json.load(f)
    # file -> {lineno: hit_count}
    out = {}
    for f in data['files']:
        lines = {}
        for line in f['lines']:
            lines[line['line_number']] = line['count']
        out[f['file']] = lines
    return out


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('full_json')
    p.add_argument('leak_json')
    p.add_argument('--top', type=int, default=30,
                    help='show the N files with the most gap lines (default 30)')
    p.add_argument('--memory-only', action='store_true',
                    help='restrict to memory-relevant lines only (see '
                         'memory_relevant_lines.py) -- the number that '
                         'actually matters, not raw line count')
    p.add_argument('--repo-root', default='.',
                    help='root to resolve file paths against for --memory-only '
                         '(default: current directory)')
    args = p.parse_args(argv)

    full = load_line_hits(args.full_json)
    leak = load_line_hits(args.leak_json)

    mem_lines_cache = {}

    def mem_filter(fname, line_dict):
        if not args.memory_only:
            return line_dict
        if fname not in mem_lines_cache:
            path = os.path.join(args.repo_root, fname)
            try:
                mem_lines_cache[fname] = memory_relevant_lines(path)
            except OSError:
                mem_lines_cache[fname] = set()
        relevant = mem_lines_cache[fname]
        return {ln: cnt for ln, cnt in line_dict.items() if ln in relevant}

    gaps = {}
    full_covered_total = 0
    leak_covered_total = 0
    for fname, full_lines_raw in full.items():
        full_lines = mem_filter(fname, full_lines_raw)
        leak_lines = mem_filter(fname, leak.get(fname, {}))
        covered_full = [ln for ln, cnt in full_lines.items() if cnt > 0]
        covered_leak = [ln for ln, cnt in leak_lines.items() if cnt > 0]
        full_covered_total += len(covered_full)
        leak_covered_total += len(covered_leak)
        gap_lines = [ln for ln in covered_full if leak_lines.get(ln, 0) == 0]
        if gap_lines:
            gaps[fname] = sorted(gap_lines)

    total_gap = sum(len(v) for v in gaps.values())
    scope = "memory-relevant" if args.memory_only else "all"
    print(f"{len(gaps)} files have {scope} lines covered by the full suite but "
          f"never touched by any leak-sweep target ({total_gap} lines total).")
    if full_covered_total:
        pct = 100.0 * leak_covered_total / full_covered_total
        print(f"leak-sweep {scope}-line coverage: {leak_covered_total}/"
              f"{full_covered_total} ({pct:.1f}%) of what the full suite covers.\n")
    else:
        print()

    ranked = sorted(gaps.items(), key=lambda kv: -len(kv[1]))
    for fname, lines in ranked[:args.top]:
        print(f"{len(lines):5d}  {fname}")

    print()
    print("Full detail (file: line numbers) written for files above the top-N cut.")
    return 0


if __name__ == '__main__':
    sys.exit(main())
