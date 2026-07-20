#!/usr/bin/env python3
"""
Fetch all active (open) issues from jpype-project/jpype GitHub repository.
Saves issues to project/issues/ directory as individual JSON files for analysis.

Usage:
  python3 fetch_issues.py [open|closed|all]

  Optional: Set GITHUB_TOKEN environment variable to avoid rate limiting:
    export GITHUB_TOKEN=ghp_your_token_here
    python3 fetch_issues.py
"""

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
            comments = fetch_comments(issue_num)
            issue['comment_chain'] = comments
        else:
            issue['comment_chain'] = []

    return issues


def save_issues(issues):
    """Save issues to individual JSON files."""
    ISSUES_DIR.mkdir(exist_ok=True)

    print(f"\nSaving {len(issues)} issues to {ISSUES_DIR}/")

    # Get current issue numbers
    current_issue_nums = {issue['number'] for issue in issues}

    # Delete old issue files that are no longer in the fetched set
    old_files = []
    for existing_file in ISSUES_DIR.glob("issue_*.json"):
        issue_num = int(existing_file.stem.replace("issue_", ""))
        if issue_num not in current_issue_nums:
            old_files.append((issue_num, existing_file))

    if old_files:
        old_files.sort()
        print(f"\nRemoving {len(old_files)} closed/outdated issue files:")
        for issue_num, filepath in old_files:
            print(f"  - Removing issue #{issue_num}")
            filepath.unlink()

    # Save individual issue files
    for issue in issues:
        issue_num = issue['number']
        filename = ISSUES_DIR / f"issue_{issue_num}.json"

        with open(filename, 'w', encoding='utf-8') as f:
            json.dump(issue, f, indent=2, ensure_ascii=False)

    # Save index file with summary
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

    # Sort by issue number
    index.sort(key=lambda x: x['number'])

    index_file = ISSUES_DIR / "index.json"
    with open(index_file, 'w', encoding='utf-8') as f:
        json.dump(index, f, indent=2, ensure_ascii=False)

    print(f"Saved index to {index_file}")

    # Create a simple markdown summary
    summary_file = ISSUES_DIR / "README.md"
    with open(summary_file, 'w', encoding='utf-8') as f:
        f.write(f"# JPype Issues ({len(issues)} open)\n\n")
        f.write("Fetched from: https://github.com/jpype-project/jpype/issues\n\n")
        f.write("## Issue List\n\n")

        for item in index:
            labels_str = ', '.join(item['labels']) if item['labels'] else 'no labels'
            f.write(f"- [#{item['number']}]({item['url']}): {item['title']}\n")
            f.write(f"  - Labels: {labels_str}\n")
            f.write(f"  - Comments: {item['comments']}\n")
            f.write(f"  - Updated: {item['updated_at'][:10]}\n\n")

    print(f"Saved summary to {summary_file}")


def main():
    """Main entry point."""
    state = "open"
    if len(sys.argv) > 1:
        state = sys.argv[1]
        if state not in ["open", "closed", "all"]:
            print(f"Usage: {sys.argv[0]} [open|closed|all]")
            print("Default: open")
            sys.exit(1)

    issues = fetch_issues(state)

    if not issues:
        print("No issues found.")
        return

    save_issues(issues)

    print(f"\n✓ Successfully fetched and saved {len(issues)} issues")
    print(f"  View summary: {ISSUES_DIR / 'README.md'}")
    print(f"  View index: {ISSUES_DIR / 'index.json'}")
    print(f"  Individual issues: {ISSUES_DIR / 'issue_*.json'}")


if __name__ == "__main__":
    main()
