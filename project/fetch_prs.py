#!/usr/bin/env python3
"""
Fetch pull requests from jpype-project/jpype GitHub repository.
Saves PRs to project/pr/ directory as individual JSON files for analysis.

Usage:
  python3 fetch_prs.py fetch [open|closed|all]   Bulk (re)fetch PRs by state (default: open).
                                                  Writes/overwrites the fetched PRs; PRs already
                                                  cached locally but not returned this run are left
                                                  alone (use cleanup to prune those).
  python3 fetch_prs.py refresh <number> [...]    Re-fetch specific PR(s) by number and update
                                                  their local file + the index/summary in place.
  python3 fetch_prs.py update                    Re-fetch every PR currently cached locally
                                                  (equivalent to `refresh` on all cached numbers).
  python3 fetch_prs.py cleanup                   Remove locally cached PRs that are no longer
                                                  open on GitHub (merged, closed, or deleted).

  For backward compatibility, `python3 fetch_prs.py [open|closed|all]` is treated as
  `python3 fetch_prs.py fetch [open|closed|all]`.

  Optional: Set GITHUB_TOKEN environment variable to avoid rate limiting:
    export GITHUB_TOKEN=ghp_your_token_here
    python3 fetch_prs.py fetch
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
PRS_DIR = Path(__file__).parent / "pr"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")


def make_request(url):
    """Make an authenticated API request."""
    headers = {'User-Agent': 'JPype-PR-Fetcher'}
    if GITHUB_TOKEN:
        headers['Authorization'] = f'token {GITHUB_TOKEN}'

    req = Request(url, headers=headers)
    return urlopen(req)


def _fetch_paginated(url_template, label, pr_number):
    """Fetch every page of a paginated sub-resource for a PR."""
    items = []
    page = 1
    per_page = 100

    while True:
        url = f"{url_template}?per_page={per_page}&page={page}"

        try:
            with make_request(url) as response:
                page_items = json.loads(response.read().decode('utf-8'))

                if not page_items:
                    break

                items.extend(page_items)
                page += 1

        except HTTPError as e:
            print(f"    Warning: Failed to fetch {label} for PR #{pr_number}: {e.code}")
            if e.code == 403:
                print(f"    Rate limit hit. Set GITHUB_TOKEN environment variable to continue.")
            break
        except URLError as e:
            print(f"    Warning: Failed to fetch {label} for PR #{pr_number}: {e.reason}")
            break

    return items


def fetch_comments(pr_number):
    """Fetch all issue comments for a given PR."""
    return _fetch_paginated(f"{API_BASE}/issues/{pr_number}/comments", "comments", pr_number)


def fetch_review_comments(pr_number):
    """Fetch all review comments (inline code comments) for a given PR."""
    return _fetch_paginated(f"{API_BASE}/pulls/{pr_number}/comments", "review comments", pr_number)


def fetch_reviews(pr_number):
    """Fetch all reviews for a given PR."""
    return _fetch_paginated(f"{API_BASE}/pulls/{pr_number}/reviews", "reviews", pr_number)


def fetch_commits(pr_number):
    """Fetch all commits for a given PR."""
    return _fetch_paginated(f"{API_BASE}/pulls/{pr_number}/commits", "commits", pr_number)


def fetch_pr_details(pr_number):
    """Fetch a single PR's top-level data. Returns None if it doesn't exist."""
    url = f"{API_BASE}/pulls/{pr_number}"
    try:
        with make_request(url) as response:
            return json.loads(response.read().decode('utf-8'))
    except HTTPError as e:
        if e.code == 404:
            return None
        print(f"    Warning: Failed to fetch PR #{pr_number}: {e.code}")
        if e.code == 403:
            print(f"    Rate limit hit. Set GITHUB_TOKEN environment variable to continue.")
        return None
    except URLError as e:
        print(f"    Warning: Failed to fetch PR #{pr_number}: {e.reason}")
        return None


def enrich_pr(pr):
    """Attach comment/review/commit sub-resources to a PR dict, in place."""
    pr_num = pr['number']
    pr['comment_chain'] = fetch_comments(pr_num)
    pr['review_comments'] = fetch_review_comments(pr_num)
    pr['reviews'] = fetch_reviews(pr_num)
    pr['commits'] = fetch_commits(pr_num)
    return pr


def fetch_prs(state="open"):
    """Fetch all PRs with the given state (open/closed/all)."""
    prs = []
    page = 1
    per_page = 100  # GitHub max

    auth_status = "authenticated" if GITHUB_TOKEN else "unauthenticated (60 req/hour limit)"
    print(f"Fetching {state} PRs from {REPO_OWNER}/{REPO_NAME} ({auth_status})...")

    while True:
        url = f"{API_BASE}/pulls?state={state}&per_page={per_page}&page={page}"

        try:
            with make_request(url) as response:
                page_prs = json.loads(response.read().decode('utf-8'))

                if not page_prs:
                    break

                prs.extend(page_prs)
                print(f"  Fetched page {page}: {len(page_prs)} PRs")

                page += 1

        except HTTPError as e:
            print(f"HTTP Error: {e.code} - {e.reason}")
            if e.code == 403:
                print("Rate limit exceeded. Set GITHUB_TOKEN environment variable for higher limits.")
            sys.exit(1)
        except URLError as e:
            print(f"URL Error: {e.reason}")
            sys.exit(1)

    # Fetch additional data for each PR
    print(f"\nFetching additional data for {len(prs)} PRs...")
    for i, pr in enumerate(prs, 1):
        pr_num = pr['number']
        print(f"  [{i}/{len(prs)}] PR #{pr_num}: fetching comments, reviews, commits...")
        enrich_pr(pr)

        total = (len(pr['comment_chain']) + len(pr['review_comments']) +
                 len(pr['reviews']) + len(pr['commits']))
        if total > 0:
            print(f"    → {len(pr['comment_chain'])} comments, {len(pr['review_comments'])} review comments, "
                  f"{len(pr['reviews'])} reviews, {len(pr['commits'])} commits")

    return prs


def load_local_prs():
    """Load every currently-cached PR from project/pr/pr_*.json into a dict keyed by number."""
    prs = {}
    if not PRS_DIR.exists():
        return prs
    for filepath in PRS_DIR.glob("pr_*.json"):
        try:
            pr_num = int(filepath.stem.replace("pr_", ""))
        except ValueError:
            continue
        with open(filepath, encoding='utf-8') as f:
            prs[pr_num] = json.load(f)
    return prs


def write_pr_file(pr):
    """Write a single PR's data to its JSON file."""
    PRS_DIR.mkdir(exist_ok=True)
    filename = PRS_DIR / f"pr_{pr['number']}.json"
    with open(filename, 'w', encoding='utf-8') as f:
        json.dump(pr, f, indent=2, ensure_ascii=False)


def remove_pr_file(pr_number):
    """Delete a cached PR's JSON file, if present."""
    filename = PRS_DIR / f"pr_{pr_number}.json"
    if filename.exists():
        filename.unlink()


def build_index_and_summary(prs):
    """(Re)build index.json and README.md from the given PRs (dict or list)."""
    PRS_DIR.mkdir(exist_ok=True)
    if isinstance(prs, dict):
        prs = list(prs.values())

    print(f"\nWriting index/summary for {len(prs)} cached PRs to {PRS_DIR}/")

    # Save index file with summary
    index = []
    for pr in prs:
        index.append({
            'number': pr['number'],
            'title': pr['title'],
            'state': pr['state'],
            'draft': pr.get('draft', False),
            'created_at': pr['created_at'],
            'updated_at': pr['updated_at'],
            'merged_at': pr.get('merged_at'),
            'closed_at': pr.get('closed_at'),
            'user': pr['user']['login'],
            'labels': [label['name'] for label in pr['labels']],
            'comments': len(pr.get('comment_chain', [])),
            'review_comments': len(pr.get('review_comments', [])),
            'reviews': len(pr.get('reviews', [])),
            'commits': len(pr.get('commits', [])),
            'additions': pr.get('additions', 0),
            'deletions': pr.get('deletions', 0),
            'changed_files': pr.get('changed_files', 0),
            'url': pr['html_url']
        })

    # Sort by PR number
    index.sort(key=lambda x: x['number'])

    index_file = PRS_DIR / "index.json"
    with open(index_file, 'w', encoding='utf-8') as f:
        json.dump(index, f, indent=2, ensure_ascii=False)

    print(f"Saved index to {index_file}")

    # Create a simple markdown summary
    summary_file = PRS_DIR / "README.md"
    with open(summary_file, 'w', encoding='utf-8') as f:
        merged_count = sum(1 for item in index if item['merged_at'])
        open_count = sum(1 for item in index if item['state'] == 'open')
        closed_count = sum(1 for item in index if item['state'] == 'closed' and not item['merged_at'])

        f.write(f"# JPype Pull Requests\n\n")
        f.write(f"Total: {len(prs)} PRs\n")
        f.write(f"- Open: {open_count}\n")
        f.write(f"- Merged: {merged_count}\n")
        f.write(f"- Closed (not merged): {closed_count}\n\n")
        f.write("Fetched from: https://github.com/jpype-project/jpype/pulls\n\n")
        f.write("## PR List\n\n")

        for item in index:
            status = "🟢 OPEN" if item['state'] == 'open' else (
                "🟣 MERGED" if item['merged_at'] else "🔴 CLOSED"
            )
            draft = " [DRAFT]" if item['draft'] else ""
            labels_str = ', '.join(item['labels']) if item['labels'] else 'no labels'

            f.write(f"### {status}{draft} [#{item['number']}]({item['url']}): {item['title']}\n")
            f.write(f"- **Author**: {item['user']}\n")
            f.write(f"- **Labels**: {labels_str}\n")
            f.write(f"- **Stats**: +{item['additions']}/-{item['deletions']} ({item['changed_files']} files)\n")
            f.write(f"- **Discussion**: {item['comments']} comments, {item['review_comments']} review comments\n")
            f.write(f"- **Created**: {item['created_at'][:10]}\n")
            if item['merged_at']:
                f.write(f"- **Merged**: {item['merged_at'][:10]}\n")
            elif item['closed_at']:
                f.write(f"- **Closed**: {item['closed_at'][:10]}\n")
            f.write("\n")

    print(f"Saved summary to {summary_file}")


def cmd_fetch(state):
    """Bulk (re)fetch PRs by state. Merges into (doesn't wipe) the local cache."""
    prs = fetch_prs(state)

    if not prs:
        print("No PRs found.")
        return

    for pr in prs:
        write_pr_file(pr)

    local = load_local_prs()
    local.update({pr['number']: pr for pr in prs})
    build_index_and_summary(local)

    print(f"\n✓ Successfully fetched and saved {len(prs)} PRs")
    print(f"  View summary: {PRS_DIR / 'README.md'}")
    print(f"  View index: {PRS_DIR / 'index.json'}")
    print(f"  Individual PRs: {PRS_DIR / 'pr_*.json'}")


def cmd_refresh(numbers):
    """Re-fetch specific PR(s) by number and update them in the local cache."""
    local = load_local_prs()
    refreshed = 0

    for pr_num in numbers:
        print(f"Refreshing PR #{pr_num}...")
        pr = fetch_pr_details(pr_num)
        if pr is None:
            print(f"  PR #{pr_num} not found (deleted, or a bad number?) - left untouched locally. "
                  f"Run cleanup to prune it if it's gone for good.")
            continue

        enrich_pr(pr)
        write_pr_file(pr)
        local[pr_num] = pr
        refreshed += 1
        print(f"  → #{pr_num}: {pr['title']} ({pr['state']})")

    build_index_and_summary(local)
    print(f"\n✓ Refreshed {refreshed}/{len(numbers)} requested PRs")


def cmd_update():
    """Refresh every PR currently cached locally."""
    local = load_local_prs()
    if not local:
        print(f"Nothing cached in {PRS_DIR}/ yet - run `fetch` first.")
        return
    numbers = sorted(local.keys())
    print(f"Updating {len(numbers)} cached PRs...")
    cmd_refresh(numbers)


def cmd_cleanup():
    """Remove locally cached PRs that are no longer open on GitHub."""
    local = load_local_prs()
    if not local:
        print(f"Nothing cached in {PRS_DIR}/.")
        return

    removed = []
    kept = {}
    for pr_num in sorted(local.keys()):
        print(f"Checking PR #{pr_num}...")
        pr = fetch_pr_details(pr_num)
        if pr is None or pr.get('state') != 'open':
            removed.append(pr_num)
            remove_pr_file(pr_num)
        else:
            # Keep the freshly-checked top-level data; sub-resources are unchanged.
            pr['comment_chain'] = local[pr_num].get('comment_chain', [])
            pr['review_comments'] = local[pr_num].get('review_comments', [])
            pr['reviews'] = local[pr_num].get('reviews', [])
            pr['commits'] = local[pr_num].get('commits', [])
            kept[pr_num] = pr

    build_index_and_summary(kept)

    if removed:
        print(f"\n✓ Removed {len(removed)} no-longer-open PR(s): {', '.join(f'#{n}' for n in removed)}")
    else:
        print("\n✓ Nothing to remove - every cached PR is still open")


def main():
    """Main entry point."""
    argv = sys.argv[1:]

    # Backward compatibility: `fetch_prs.py [open|closed|all]` == `fetch_prs.py fetch [state]`
    if argv and argv[0] in ("open", "closed", "all"):
        argv = ["fetch"] + argv

    parser = argparse.ArgumentParser(description="Fetch/refresh/update/cleanup JPype PR data.")
    subparsers = parser.add_subparsers(dest="command")

    fetch_parser = subparsers.add_parser("fetch", help="Bulk (re)fetch PRs by state (default: open).")
    fetch_parser.add_argument("state", nargs="?", default="open", choices=["open", "closed", "all"])

    refresh_parser = subparsers.add_parser("refresh", help="Re-fetch specific PR(s) by number.")
    refresh_parser.add_argument("numbers", nargs="+", type=int)

    subparsers.add_parser("update", help="Refresh every PR currently cached locally.")
    subparsers.add_parser("cleanup", help="Remove cached PRs that are no longer open on GitHub.")

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
