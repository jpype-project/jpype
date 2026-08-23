#!/usr/bin/env python
"""Print pytest node ids for every entry in a leak_targets.txt config.

Usage: leak_target_ids.py test/jpypetest/leak_targets.txt

One node id per line, e.g. `jpypetest/test_proxy.py::ProxyTestCase::testProxyLeak`,
suitable for `pytest $(leak_target_ids.py ... | tr '\\n' ' ')` run from the
`test/` directory. Used by leak_coverage.sh to run each leak-sweep target
exactly once (a reachability check, not a leak measurement) under a
coverage-instrumented build.
"""
import sys


def target_ids(config_path):
    ids = []
    with open(config_path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split(':')
            if parts[0] == 'GENERIC':
                module, target = parts[1], parts[2]
            else:
                module, target = parts[0], parts[1]
            if '.' in target:
                cls, method = target.rsplit('.', 1)
                ids.append(f"jpypetest/{module}::{cls}::{method}")
            else:
                ids.append(f"jpypetest/{module}::{target}")
    return ids


if __name__ == '__main__':
    for node_id in target_ids(sys.argv[1]):
        print(node_id)
