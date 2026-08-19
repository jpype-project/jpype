"""Parse raw benchmark script output (stdout .log for jpype/jpy/pyjnius,
jep.Run's out_path .txt files) into a single parsed.json, keyed by
[library][script] -> [{label, best, median, ns_per_element}, ...].

Feeds gen_results.py, which turns parsed.json into RESULTS.md's tables.
Every `label + best=... ns/call  median=... ns/call` line any of the
project/benchmark/{jpype,jpy,jep,pyjnius}/*.py scripts print (or write
to their out_path, for jep) matches the same regex, so this is
library-agnostic -- it doesn't know or care what a given label means,
only that it matches that shape.

Usage:
    python3 parse_logs.py <logdir> [parsed.json output path]

<logdir> must contain one subdirectory per library (jpype/, jpy/, jep/,
pyjnius/), each holding that library's raw output files -- run each
script and redirect its stdout (or, for jep, its out_path) into
<logdir>/<library>/<script-name>.log (or .txt for jep).
"""
import re
import sys
import json
import glob
import os

ROW_RE = re.compile(
    r'^(?P<label>.+?)\s+best=\s*([\d,]+\.\d)\s*ns/call\s+median=\s*([\d,]+\.\d)\s*ns/call'
    r'(?:\s+\(\s*([\d.]+)\s*ns/element\))?')

LIBS = ['jpype', 'jpy', 'jep', 'pyjnius']


def parse_file(path):
    rows = []
    with open(path, errors='replace') as f:
        for line in f:
            m = ROW_RE.match(line.strip())
            if m:
                label = m.group('label').strip()
                best = float(m.group(2).replace(',', ''))
                median = float(m.group(3).replace(',', ''))
                ns_el = m.group(4)
                rows.append({'label': label, 'best': best, 'median': median,
                             'ns_per_element': float(ns_el) if ns_el else None})
    return rows


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    logroot = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(logroot, 'parsed.json')

    out = {}
    for lib in LIBS:
        libdir = os.path.join(logroot, lib)
        out[lib] = {}
        for path in sorted(glob.glob(os.path.join(libdir, '*.log'))) + \
                sorted(glob.glob(os.path.join(libdir, '*.txt'))):
            script = os.path.splitext(os.path.basename(path))[0]
            if script in out[lib]:
                continue
            out[lib][script] = parse_file(path)

    with open(out_path, 'w') as f:
        json.dump(out, f, indent=1)

    for lib in LIBS:
        for script, rows in out[lib].items():
            print(f"{lib}/{script}: {len(rows)} rows")
    print(f"\nwrote {out_path}")


if __name__ == '__main__':
    main()
