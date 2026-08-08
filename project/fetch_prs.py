#!/usr/bin/env python3
"""
Fetch all pull requests from jpype-project/jpype GitHub repository.
Saves PRs to ../pr/ directory as individual JSON files for analysis.

Usage:
  python3 fetch_prs.py [open|closed|all]

  Optional: Set GITHUB_TOKEN environment variable to avoid rate limiting:
    export GITHUB_TOKEN=ghp_your_token_here
    python3 fetch_prs.py
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
PRS_DIR = Path(__file__).parent.parent / "pr"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")


def make_request(url):
    """Make an authenticated API request."""
    headers = {'User-Agent': 'JPype-PR-Fetcher'}
    if GITHUB_TOKEN:
        headers['Authorization'] = f'token {GITHUB_TOKEN}'

    req = Request(url, headers=headers)
    return urlopen(req)


def fetch_comments(pr_number):
    """Fetch all issue comments for a given PR."""
    comments = []
    page = 1
    per_page = 100

    while True:
        url = f"{API_BASE}/issues/{pr_number}/comments?per_page={per_page}&page={page}"

        try:
            with make_request(url) as response:
                page_comments = json.loads(response.read().decode('utf-8'))

                if not page_comments:
                    break

                comments.extend(page_comments)
                page += 1

        except HTTPError as e:
            print(f"    Warning: Failed to fetch comments for PR #{pr_number}: {e.code}")
            if e.code == 403:
                print(f"    Rate limit hit. Set GITHUB_TOKEN environment variable to continue.")
            break
        except URLError as e:
            print(f"    Warning: Failed to fetch comments for PR #{pr_number}: {e.reason}")
            break

    return comments


def fetch_review_comments(pr_number):
    """Fetch all review comments (inline code comments) for a given PR."""
    comments = []
    page = 1
    per_page = 100

    while True:
        url = f"{API_BASE}/pulls/{pr_number}/comments?per_page={per_page}&page={page}"

        try:
            with make_request(url) as response:
                page_comments = json.loads(response.read().decode('utf-8'))

                if not page_comments:
                    break

                comments.extend(page_comments)
                page += 1

        except HTTPError as e:
            print(f"    Warning: Failed to fetch review comments for PR #{pr_number}: {e.code}")
            break
        except URLError as e:
            print(f"    Warning: Failed to fetch review comments for PR #{pr_number}: {e.reason}")
            break

    return comments


def fetch_reviews(pr_number):
    """Fetch all reviews for a given PR."""
    reviews = []
    page = 1
    per_page = 100

    while True:
        url = f"{API_BASE}/pulls/{pr_number}/reviews?per_page={per_page}&page={page}"

        try:
            with make_request(url) as response:
                page_reviews = json.loads(response.read().decode('utf-8'))

                if not page_reviews:
                    break

                reviews.extend(page_reviews)
                page += 1

        except HTTPError as e:
            print(f"    Warning: Failed to fetch reviews for PR #{pr_number}: {e.code}")
            break
        except URLError as e:
            print(f"    Warning: Failed to fetch reviews for PR #{pr_number}: {e.reason}")
            break

    return reviews


def fetch_commits(pr_number):
    """Fetch all commits for a given PR."""
    commits = []
    page = 1
    per_page = 100

    while True:
        url = f"{API_BASE}/pulls/{pr_number}/commits?per_page={per_page}&page={page}"

        try:
            with make_request(url) as response:
                page_commits = json.loads(response.read().decode('utf-8'))

                if not page_commits:
                    break

                commits.extend(page_commits)
                page += 1

        except HTTPError as e:
            print(f"    Warning: Failed to fetch commits for PR #{pr_number}: {e.code}")
            break
        except URLError as e:
            print(f"    Warning: Failed to fetch commits for PR #{pr_number}: {e.reason}")
            break

    return commits


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

        # Fetch issue comments (general discussion)
        pr['comment_chain'] = fetch_comments(pr_num)

        # Fetch review comments (inline code comments)
        pr['review_comments'] = fetch_review_comments(pr_num)

        # Fetch reviews (approve/request changes/comment)
        pr['reviews'] = fetch_reviews(pr_num)

        # Fetch commits
        pr['commits'] = fetch_commits(pr_num)

        total = (len(pr['comment_chain']) + len(pr['review_comments']) +
                 len(pr['reviews']) + len(pr['commits']))
        if total > 0:
            print(f"    → {len(pr['comment_chain'])} comments, {len(pr['review_comments'])} review comments, "
                  f"{len(pr['reviews'])} reviews, {len(pr['commits'])} commits")

    return prs


def save_prs(prs):
    """Save PRs to individual JSON files."""
    PRS_DIR.mkdir(exist_ok=True)

    print(f"\nSaving {len(prs)} PRs to {PRS_DIR}/")

    # Save individual PR files
    for pr in prs:
        pr_num = pr['number']
        filename = PRS_DIR / f"pr_{pr_num}.json"

        with open(filename, 'w', encoding='utf-8') as f:
            json.dump(pr, f, indent=2, ensure_ascii=False)

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


def main():
    """Main entry point."""
    state = "open"
    if len(sys.argv) > 1:
        state = sys.argv[1]
        if state not in ["open", "closed", "all"]:
            print(f"Usage: {sys.argv[0]} [open|closed|all]")
            print("Default: open")
            sys.exit(1)

    prs = fetch_prs(state)

    if not prs:
        print("No PRs found.")
        return

    save_prs(prs)

    print(f"\n✓ Successfully fetched and saved {len(prs)} PRs")
    print(f"  View summary: {PRS_DIR / 'README.md'}")
    print(f"  View index: {PRS_DIR / 'index.json'}")
    print(f"  Individual PRs: {PRS_DIR / 'pr_*.json'}")


if __name__ == "__main__":
    main()
