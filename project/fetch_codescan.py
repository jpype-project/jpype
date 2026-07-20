#!/usr/bin/env python3
"""
Fetch code scanning alerts (CodeQL etc.) from jpype-project/jpype's
GitHub Security tab: https://github.com/jpype-project/jpype/security/code-scanning

Saves alerts to project/codescan/ directory as individual JSON files for analysis.

Usage:
  python3 fetch_codescan.py fetch [open|dismissed|fixed|all]  Bulk (re)fetch alerts by state
                                                                (default: open). Writes/overwrites
                                                                the fetched alerts; alerts already
                                                                cached locally but not returned this
                                                                run are left alone (use cleanup to
                                                                prune those).
  python3 fetch_codescan.py refresh <number> [...]             Re-fetch specific alert(s) by number
                                                                and update them in place.
  python3 fetch_codescan.py update                             Re-fetch every alert currently cached
                                                                locally (refresh on all cached numbers).
  python3 fetch_codescan.py cleanup                            Remove locally cached alerts that are
                                                                no longer open on GitHub (dismissed,
                                                                fixed, or gone).

  Note on states: code scanning alerts use "open"/"dismissed"/"fixed", not the
  "open"/"closed" vocabulary fetch_prs.py/fetch_issues.py use - there's no single
  "closed" bucket, dismissed (a human said "not an issue") and fixed (the code
  changed) are worth keeping distinct for triage.

  Requires a GitHub token with the `security_events` scope (classic tokens) - the
  default no-scope token that's enough for fetch_prs.py/fetch_issues.py is NOT
  enough here; GitHub returns 403 "Resource not accessible by personal access
  token" if the scope is missing.

    export GITHUB_TOKEN=ghp_your_token_with_security_events_scope
    python3 fetch_codescan.py fetch
"""

import argparse
import json
import os
import sys
from pathlib import Path
from urllib.request import urlopen, Request
from urllib.error import HTTPError, URLError


REPO_OWNER = "jpype-project"
REPO_NAME = "jpype"
API_BASE = f"https://api.github.com/repos/{REPO_OWNER}/{REPO_NAME}"
CODESCAN_DIR = Path(__file__).parent / "codescan"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")

STATES = ("open", "dismissed", "fixed", "all")


def make_request(url):
    """Make an authenticated API request."""
    headers = {
        'User-Agent': 'JPype-CodeScan-Fetcher',
        'Accept': 'application/vnd.github+json',
    }
    if GITHUB_TOKEN:
        headers['Authorization'] = f'token {GITHUB_TOKEN}'

    req = Request(url, headers=headers)
    return urlopen(req)


def _explain_403(e):
    if e.code == 403:
        print("    This endpoint needs a GitHub token with the `security_events` scope "
              "(classic tokens) - a token that works fine for fetch_prs.py/fetch_issues.py "
              "may still be rejected here. Regenerate the token with that scope added.")


def fetch_alerts(state="open"):
    """Fetch all code scanning alerts with the given state (open/dismissed/fixed/all)."""
    alerts = []
    page = 1
    per_page = 100  # GitHub max

    auth_status = "authenticated" if GITHUB_TOKEN else "unauthenticated (60 req/hour limit)"
    print(f"Fetching {state} code scanning alerts from {REPO_OWNER}/{REPO_NAME} ({auth_status})...")

    state_param = "" if state == "all" else f"&state={state}"

    while True:
        url = f"{API_BASE}/code-scanning/alerts?per_page={per_page}&page={page}{state_param}"

        try:
            with make_request(url) as response:
                page_alerts = json.loads(response.read().decode('utf-8'))

                if not page_alerts:
                    break

                alerts.extend(page_alerts)
                print(f"  Fetched page {page}: {len(page_alerts)} alerts")

                page += 1

        except HTTPError as e:
            print(f"HTTP Error: {e.code} - {e.reason}")
            _explain_403(e)
            sys.exit(1)
        except URLError as e:
            print(f"URL Error: {e.reason}")
            sys.exit(1)

    return alerts


def fetch_alert_details(alert_number):
    """Fetch a single alert's data. Returns None if it doesn't exist."""
    url = f"{API_BASE}/code-scanning/alerts/{alert_number}"
    try:
        with make_request(url) as response:
            return json.loads(response.read().decode('utf-8'))
    except HTTPError as e:
        if e.code == 404:
            return None
        print(f"    Warning: Failed to fetch alert #{alert_number}: {e.code}")
        _explain_403(e)
        return None
    except URLError as e:
        print(f"    Warning: Failed to fetch alert #{alert_number}: {e.reason}")
        return None


def load_local_alerts():
    """Load every currently-cached alert from project/codescan/alert_*.json, keyed by number."""
    alerts = {}
    if not CODESCAN_DIR.exists():
        return alerts
    for filepath in CODESCAN_DIR.glob("alert_*.json"):
        try:
            alert_num = int(filepath.stem.replace("alert_", ""))
        except ValueError:
            continue
        with open(filepath, encoding='utf-8') as f:
            alerts[alert_num] = json.load(f)
    return alerts


def write_alert_file(alert):
    """Write a single alert's data to its JSON file."""
    CODESCAN_DIR.mkdir(exist_ok=True)
    filename = CODESCAN_DIR / f"alert_{alert['number']}.json"
    with open(filename, 'w', encoding='utf-8') as f:
        json.dump(alert, f, indent=2, ensure_ascii=False)


def remove_alert_file(alert_number):
    """Delete a cached alert's JSON file, if present."""
    filename = CODESCAN_DIR / f"alert_{alert_number}.json"
    if filename.exists():
        filename.unlink()


def build_index_and_summary(alerts):
    """(Re)build index.json and README.md from the given alerts (dict or list)."""
    CODESCAN_DIR.mkdir(exist_ok=True)
    if isinstance(alerts, dict):
        alerts = list(alerts.values())

    print(f"\nWriting index/summary for {len(alerts)} cached alerts to {CODESCAN_DIR}/")

    index = []
    for alert in alerts:
        rule = alert.get('rule') or {}
        instance = alert.get('most_recent_instance') or {}
        location = instance.get('location') or {}
        index.append({
            'number': alert['number'],
            'state': alert['state'],
            'rule_id': rule.get('id'),
            'rule_description': rule.get('description'),
            'severity': rule.get('severity'),
            'security_severity_level': rule.get('security_severity_level'),
            'tool': (alert.get('tool') or {}).get('name'),
            'path': location.get('path'),
            'start_line': location.get('start_line'),
            'message': (instance.get('message') or {}).get('text'),
            'created_at': alert['created_at'],
            'updated_at': alert['updated_at'],
            'dismissed_at': alert.get('dismissed_at'),
            'dismissed_reason': alert.get('dismissed_reason'),
            'dismissed_comment': alert.get('dismissed_comment'),
            'fixed_at': alert.get('fixed_at'),
            'url': alert['html_url'],
        })

    index.sort(key=lambda x: x['number'])

    index_file = CODESCAN_DIR / "index.json"
    with open(index_file, 'w', encoding='utf-8') as f:
        json.dump(index, f, indent=2, ensure_ascii=False)

    print(f"Saved index to {index_file}")

    summary_file = CODESCAN_DIR / "README.md"
    with open(summary_file, 'w', encoding='utf-8') as f:
        open_count = sum(1 for item in index if item['state'] == 'open')
        dismissed_count = sum(1 for item in index if item['state'] == 'dismissed')
        fixed_count = sum(1 for item in index if item['state'] == 'fixed')

        f.write(f"# JPype Code Scanning Alerts\n\n")
        f.write(f"Total cached: {len(index)}\n")
        f.write(f"- Open: {open_count}\n")
        f.write(f"- Dismissed: {dismissed_count}\n")
        f.write(f"- Fixed: {fixed_count}\n\n")
        f.write("Fetched from: https://github.com/jpype-project/jpype/security/code-scanning\n\n")
        f.write("## Alert List\n\n")

        for item in index:
            status = {"open": "🟢 OPEN", "dismissed": "⚪ DISMISSED", "fixed": "🟣 FIXED"}.get(
                item['state'], item['state'].upper())
            f.write(f"### {status} [#{item['number']}]({item['url']}): {item['rule_description']}\n")
            f.write(f"- **Rule**: `{item['rule_id']}` ({item['tool']}, severity: {item['severity']})\n")
            f.write(f"- **Location**: `{item['path']}:{item['start_line']}`\n")
            if item['message']:
                f.write(f"- **Message**: {item['message']}\n")
            f.write(f"- **Created**: {item['created_at'][:10]}\n")
            if item['dismissed_at']:
                f.write(f"- **Dismissed**: {item['dismissed_at'][:10]} ({item['dismissed_reason']})"
                        f"{': ' + item['dismissed_comment'] if item['dismissed_comment'] else ''}\n")
            if item['fixed_at']:
                f.write(f"- **Fixed**: {item['fixed_at'][:10]}\n")
            f.write("\n")

    print(f"Saved summary to {summary_file}")


def cmd_fetch(state):
    """Bulk (re)fetch alerts by state. Merges into (doesn't wipe) the local cache."""
    alerts = fetch_alerts(state)

    if not alerts:
        print("No alerts found.")
        return

    for alert in alerts:
        write_alert_file(alert)

    local = load_local_alerts()
    local.update({alert['number']: alert for alert in alerts})
    build_index_and_summary(local)

    print(f"\n✓ Successfully fetched and saved {len(alerts)} alerts")
    print(f"  View summary: {CODESCAN_DIR / 'README.md'}")
    print(f"  View index: {CODESCAN_DIR / 'index.json'}")
    print(f"  Individual alerts: {CODESCAN_DIR / 'alert_*.json'}")


def cmd_refresh(numbers):
    """Re-fetch specific alert(s) by number and update them in the local cache."""
    local = load_local_alerts()
    refreshed = 0

    for alert_num in numbers:
        print(f"Refreshing alert #{alert_num}...")
        alert = fetch_alert_details(alert_num)
        if alert is None:
            print(f"  Alert #{alert_num} not found - left untouched locally. "
                  f"Run cleanup to prune it if it's gone for good.")
            continue

        write_alert_file(alert)
        local[alert_num] = alert
        refreshed += 1
        rule = alert.get('rule') or {}
        print(f"  → #{alert_num}: {rule.get('description')} ({alert['state']})")

    build_index_and_summary(local)
    print(f"\n✓ Refreshed {refreshed}/{len(numbers)} requested alerts")


def cmd_update():
    """Refresh every alert currently cached locally."""
    local = load_local_alerts()
    if not local:
        print(f"Nothing cached in {CODESCAN_DIR}/ yet - run `fetch` first.")
        return
    numbers = sorted(local.keys())
    print(f"Updating {len(numbers)} cached alerts...")
    cmd_refresh(numbers)


def cmd_cleanup():
    """Remove locally cached alerts that are no longer open on GitHub."""
    local = load_local_alerts()
    if not local:
        print(f"Nothing cached in {CODESCAN_DIR}/.")
        return

    removed = []
    kept = {}
    for alert_num in sorted(local.keys()):
        print(f"Checking alert #{alert_num}...")
        alert = fetch_alert_details(alert_num)
        if alert is None or alert.get('state') != 'open':
            removed.append(alert_num)
            remove_alert_file(alert_num)
        else:
            kept[alert_num] = alert

    build_index_and_summary(kept)

    if removed:
        print(f"\n✓ Removed {len(removed)} no-longer-open alert(s): {', '.join(f'#{n}' for n in removed)}")
    else:
        print("\n✓ Nothing to remove - every cached alert is still open")


def main():
    """Main entry point."""
    argv = sys.argv[1:]

    # Backward-compat with the sibling scripts' bare-state shorthand.
    if argv and argv[0] in STATES:
        argv = ["fetch"] + argv

    parser = argparse.ArgumentParser(description="Fetch/refresh/update/cleanup JPype code scanning alert data.")
    subparsers = parser.add_subparsers(dest="command")

    fetch_parser = subparsers.add_parser("fetch", help="Bulk (re)fetch alerts by state (default: open).")
    fetch_parser.add_argument("state", nargs="?", default="open", choices=list(STATES))

    refresh_parser = subparsers.add_parser("refresh", help="Re-fetch specific alert(s) by number.")
    refresh_parser.add_argument("numbers", nargs="+", type=int)

    subparsers.add_parser("update", help="Refresh every alert currently cached locally.")
    subparsers.add_parser("cleanup", help="Remove cached alerts that are no longer open on GitHub.")

    args = parser.parse_args(argv)

    if args.command is None:
        args = parser.parse_args(["fetch"])

    if args.command == "fetch":
        cmd_fetch(args.state)
    elif args.command == "refresh":
        cmd_refresh(args.numbers)
    elif args.command == "update":
        cmd_update()
    elif args.command == "cleanup":
        cmd_cleanup()


if __name__ == "__main__":
    main()
