#!/usr/bin/env python3
"""
Fetch issues from jpype-project/jpype GitHub repository.
Saves issues to project/issues/ directory as individual JSON files for analysis.

Usage:
  python3 fetch_issues.py fetch [open|closed|all]   Bulk (re)fetch issues by state (default: open).
                                                     Writes/overwrites the fetched issues; issues
                                                     already cached locally but not returned this
                                                     run are left alone (use cleanup to prune those).
  python3 fetch_issues.py refresh <number> [...]    Re-fetch specific issue(s) by number and update
                                                     their local file + the index/summary in place.
  python3 fetch_issues.py update                    Re-fetch every issue currently cached locally
                                                     (equivalent to `refresh` on all cached numbers).
  python3 fetch_issues.py cleanup                   Remove locally cached issues that are no longer
                                                     open on GitHub (closed, or deleted).

  For backward compatibility, `python3 fetch_issues.py [open|closed|all]` is treated as
  `python3 fetch_issues.py fetch [open|closed|all]`.

  Optional: Set GITHUB_TOKEN environment variable to avoid rate limiting:
    export GITHUB_TOKEN=ghp_your_token_here
    python3 fetch_issues.py fetch
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
ISSUES_DIR = Path(__file__).parent / "issues"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")


def make_request(url):
    """Make an authenticated API request."""
    headers = {'User-Agent': 'JPype-Issue-Fetcher'}
    if GITHUB_TOKEN:
        headers['Authorization'] = f'token {GITHUB_TOKEN}'

    req = Request(url, headers=headers)
    return urlopen(req)


def fetch_comments(issue_number):
    """Fetch all comments for a given issue."""
    comments = []
    page = 1
    per_page = 100

    while True:
        url = f"{API_BASE}/issues/{issue_number}/comments?per_page={per_page}&page={page}"

        try:
            with make_request(url) as response:
                page_comments = json.loads(response.read().decode('utf-8'))

                if not page_comments:
                    break

                comments.extend(page_comments)
                page += 1

        except HTTPError as e:
            print(f"    Warning: Failed to fetch comments for issue #{issue_number}: {e.code}")
            if e.code == 403:
                print(f"    Rate limit hit. Set GITHUB_TOKEN environment variable to continue.")
            break
        except URLError as e:
            print(f"    Warning: Failed to fetch comments for issue #{issue_number}: {e.reason}")
            break

    return comments


def fetch_issue_details(issue_number):
    """Fetch a single issue's top-level data. Returns None if it doesn't exist or is a PR."""
    url = f"{API_BASE}/issues/{issue_number}"
    try:
        with make_request(url) as response:
            issue = json.loads(response.read().decode('utf-8'))
            if 'pull_request' in issue:
                return None
            return issue
    except HTTPError as e:
        if e.code == 404:
            return None
        print(f"    Warning: Failed to fetch issue #{issue_number}: {e.code}")
        if e.code == 403:
            print(f"    Rate limit hit. Set GITHUB_TOKEN environment variable to continue.")
        return None
    except URLError as e:
        print(f"    Warning: Failed to fetch issue #{issue_number}: {e.reason}")
        return None


def enrich_issue(issue):
    """Attach the comment chain to an issue dict, in place."""
    issue['comment_chain'] = fetch_comments(issue['number']) if issue['comments'] > 0 else []
    return issue


def fetch_issues(state="open"):
    """Fetch all issues with the given state (open/closed/all)."""
    issues = []
    page = 1
    per_page = 100  # GitHub max

    auth_status = "authenticated" if GITHUB_TOKEN else "unauthenticated (60 req/hour limit)"
    print(f"Fetching {state} issues from {REPO_OWNER}/{REPO_NAME} ({auth_status})...")

    while True:
        url = f"{API_BASE}/issues?state={state}&per_page={per_page}&page={page}"

        try:
            with make_request(url) as response:
                page_issues = json.loads(response.read().decode('utf-8'))

                if not page_issues:
                    break

                # Filter out pull requests (they appear in issues endpoint)
                page_issues = [i for i in page_issues if 'pull_request' not in i]

                issues.extend(page_issues)
                print(f"  Fetched page {page}: {len(page_issues)} issues")

                page += 1

        except HTTPError as e:
            print(f"HTTP Error: {e.code} - {e.reason}")
            if e.code == 403:
                print("Rate limit exceeded. Set GITHUB_TOKEN environment variable for higher limits.")
            sys.exit(1)
        except URLError as e:
            print(f"URL Error: {e.reason}")
            sys.exit(1)

    # Fetch comments for each issue
    print(f"\nFetching comments for {len(issues)} issues...")
    for i, issue in enumerate(issues, 1):
        issue_num = issue['number']
        comment_count = issue['comments']

        if comment_count > 0:
            print(f"  [{i}/{len(issues)}] Issue #{issue_num}: fetching {comment_count} comments...")
        enrich_issue(issue)

    return issues


def load_local_issues():
    """Load every currently-cached issue from project/issues/issue_*.json, keyed by number."""
    issues = {}
    if not ISSUES_DIR.exists():
        return issues
    for filepath in ISSUES_DIR.glob("issue_*.json"):
        try:
            issue_num = int(filepath.stem.replace("issue_", ""))
        except ValueError:
            continue
        with open(filepath, encoding='utf-8') as f:
            issues[issue_num] = json.load(f)
    return issues


def write_issue_file(issue):
    """Write a single issue's data to its JSON file."""
    ISSUES_DIR.mkdir(exist_ok=True)
    filename = ISSUES_DIR / f"issue_{issue['number']}.json"
    with open(filename, 'w', encoding='utf-8') as f:
        json.dump(issue, f, indent=2, ensure_ascii=False)


def remove_issue_file(issue_number):
    """Delete a cached issue's JSON file, if present."""
    filename = ISSUES_DIR / f"issue_{issue_number}.json"
    if filename.exists():
        filename.unlink()


def build_index_and_summary(issues):
    """(Re)build index.json and README.md from the given issues (dict or list)."""
    ISSUES_DIR.mkdir(exist_ok=True)
    if isinstance(issues, dict):
        issues = list(issues.values())

    print(f"\nWriting index/summary for {len(issues)} cached issues to {ISSUES_DIR}/")

    index = []
    for issue in issues:
        index.append({
            'number': issue['number'],
            'title': issue['title'],
            'state': issue['state'],
            'created_at': issue['created_at'],
            'updated_at': issue['updated_at'],
            'comments': issue['comments'],
            'labels': [label['name'] for label in issue['labels']],
            'url': issue['html_url']
        })

    index.sort(key=lambda x: x['number'])

    index_file = ISSUES_DIR / "index.json"
    with open(index_file, 'w', encoding='utf-8') as f:
        json.dump(index, f, indent=2, ensure_ascii=False)

    print(f"Saved index to {index_file}")

    summary_file = ISSUES_DIR / "README.md"
    with open(summary_file, 'w', encoding='utf-8') as f:
        open_count = sum(1 for item in index if item['state'] == 'open')
        f.write(f"# JPype Issues ({open_count} open, {len(index)} cached)\n\n")
        f.write("Fetched from: https://github.com/jpype-project/jpype/issues\n\n")
        f.write("## Issue List\n\n")

        for item in index:
            labels_str = ', '.join(item['labels']) if item['labels'] else 'no labels'
            state_marker = "" if item['state'] == 'open' else " [CLOSED]"
            f.write(f"- [#{item['number']}]({item['url']}): {item['title']}{state_marker}\n")
            f.write(f"  - Labels: {labels_str}\n")
            f.write(f"  - Comments: {item['comments']}\n")
            f.write(f"  - Updated: {item['updated_at'][:10]}\n\n")

    print(f"Saved summary to {summary_file}")


def cmd_fetch(state):
    """Bulk (re)fetch issues by state. Merges into (doesn't wipe) the local cache."""
    issues = fetch_issues(state)

    if not issues:
        print("No issues found.")
        return

    for issue in issues:
        write_issue_file(issue)

    local = load_local_issues()
    local.update({issue['number']: issue for issue in issues})
    build_index_and_summary(local)

    print(f"\n✓ Successfully fetched and saved {len(issues)} issues")
    print(f"  View summary: {ISSUES_DIR / 'README.md'}")
    print(f"  View index: {ISSUES_DIR / 'index.json'}")
    print(f"  Individual issues: {ISSUES_DIR / 'issue_*.json'}")


def cmd_refresh(numbers):
    """Re-fetch specific issue(s) by number and update them in the local cache."""
    local = load_local_issues()
    refreshed = 0

    for issue_num in numbers:
        print(f"Refreshing issue #{issue_num}...")
        issue = fetch_issue_details(issue_num)
        if issue is None:
            print(f"  Issue #{issue_num} not found (deleted, a PR, or a bad number?) - left "
                  f"untouched locally. Run cleanup to prune it if it's gone for good.")
            continue

        enrich_issue(issue)
        write_issue_file(issue)
        local[issue_num] = issue
        refreshed += 1
        print(f"  → #{issue_num}: {issue['title']} ({issue['state']})")

    build_index_and_summary(local)
    print(f"\n✓ Refreshed {refreshed}/{len(numbers)} requested issues")


def cmd_update():
    """Refresh every issue currently cached locally."""
    local = load_local_issues()
    if not local:
        print(f"Nothing cached in {ISSUES_DIR}/ yet - run `fetch` first.")
        return
    numbers = sorted(local.keys())
    print(f"Updating {len(numbers)} cached issues...")
    cmd_refresh(numbers)


def cmd_cleanup():
    """Remove locally cached issues that are no longer open on GitHub."""
    local = load_local_issues()
    if not local:
        print(f"Nothing cached in {ISSUES_DIR}/.")
        return

    removed = []
    kept = {}
    for issue_num in sorted(local.keys()):
        print(f"Checking issue #{issue_num}...")
        issue = fetch_issue_details(issue_num)
        if issue is None or issue.get('state') != 'open':
            removed.append(issue_num)
            remove_issue_file(issue_num)
        else:
            issue['comment_chain'] = local[issue_num].get('comment_chain', [])
            kept[issue_num] = issue

    build_index_and_summary(kept)

    if removed:
        print(f"\n✓ Removed {len(removed)} no-longer-open issue(s): {', '.join(f'#{n}' for n in removed)}")
    else:
        print("\n✓ Nothing to remove - every cached issue is still open")


def main():
    """Main entry point."""
    argv = sys.argv[1:]

    # Backward compatibility: `fetch_issues.py [open|closed|all]` == `fetch_issues.py fetch [state]`
    if argv and argv[0] in ("open", "closed", "all"):
        argv = ["fetch"] + argv

    parser = argparse.ArgumentParser(description="Fetch/refresh/update/cleanup JPype issue data.")
    subparsers = parser.add_subparsers(dest="command")

    fetch_parser = subparsers.add_parser("fetch", help="Bulk (re)fetch issues by state (default: open).")
    fetch_parser.add_argument("state", nargs="?", default="open", choices=["open", "closed", "all"])

    refresh_parser = subparsers.add_parser("refresh", help="Re-fetch specific issue(s) by number.")
    refresh_parser.add_argument("numbers", nargs="+", type=int)

    subparsers.add_parser("update", help="Refresh every issue currently cached locally.")
    subparsers.add_parser("cleanup", help="Remove cached issues that are no longer open on GitHub.")

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
