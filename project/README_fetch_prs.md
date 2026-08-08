# fetch_prs.py

Fetches all pull requests from the JPype GitHub repository with complete data for pipeline tracking.

## Basic Usage

```bash
# Fetch open PRs (default)
python3 project/fetch_prs.py

# Fetch closed/merged PRs
python3 project/fetch_prs.py closed

# Fetch all PRs
python3 project/fetch_prs.py all
```

## Authentication (Recommended)

GitHub's unauthenticated API has a limit of **60 requests/hour**. Since fetching PRs requires multiple requests per PR (comments, reviews, commits), you'll hit this limit quickly.

To fetch all data, use a GitHub Personal Access Token:

```bash
export GITHUB_TOKEN=ghp_your_token_here
python3 project/fetch_prs.py
```

With authentication, you get **5000 requests/hour**. See the [fetch_issues README](README_fetch_issues.md) for instructions on creating a token.

## Output Structure

All data is saved to the `pr/` directory (already in `.gitignore`):

- **`pr/README.md`** - Human-readable summary with status, stats, and links
- **`pr/index.json`** - Structured index with metadata
- **`pr/pr_*.json`** - Individual PR files with complete data

### PR JSON Structure

Each `pr_*.json` file contains:

**Basic Info:**
- `number`, `title`, `body` - PR number, title, and description
- `state` - "open" or "closed"
- `draft` - Whether it's a draft PR
- `merged_at`, `closed_at` - Timestamps (null if not merged/closed)
- `user` - Author information
- `labels` - PR labels

**Code Changes:**
- `additions`, `deletions` - Lines added/removed
- `changed_files` - Number of files modified
- `base`, `head` - Source and target branches

**Discussion & Review:**
- **`comment_chain`** - Array of general discussion comments
- **`review_comments`** - Array of inline code review comments (with file/line info)
- **`reviews`** - Array of review decisions (approve/request changes/comment)
- **`commits`** - Array of all commits in the PR

Example structure:
```json
{
  "number": 1391,
  "title": "Add zero-copy support for NumPy arrays",
  "state": "open",
  "draft": false,
  "additions": 827,
  "deletions": 23,
  "changed_files": 5,
  "comment_chain": [
    {
      "user": {"login": "maintainer"},
      "body": "Please add benchmarks before merging",
      "created_at": "2025-01-15T10:30:00Z"
    }
  ],
  "review_comments": [
    {
      "user": {"login": "reviewer"},
      "body": "This could be optimized",
      "path": "jpype/nio.py",
      "line": 42,
      "diff_hunk": "@@ -40,6 +40,8 @@..."
    }
  ],
  "reviews": [
    {
      "user": {"login": "reviewer"},
      "state": "APPROVED",
      "body": "LGTM!"
    }
  ],
  "commits": [
    {
      "sha": "abc123...",
      "commit": {
        "message": "Add zero-copy buffer conversion"
      }
    }
  ]
}
```

## Use Cases for Pipeline Tracking

### 1. See what features are in progress
```bash
python3 project/fetch_prs.py open
cat pr/README.md
```

### 2. Check PR review status
```bash
# Find PRs awaiting review
jq '.[] | select(.state == "open") | {number, title, reviews: (.reviews | length)}' pr/index.json

# Find approved but not merged
jq -r '.[] | select(.reviews[] | select(.state == "APPROVED")) | "PR #\(.number): \(.title)"' pr/pr_*.json
```

### 3. Track work by label
```bash
# Find all "enhancement" PRs
jq '.[] | select(.labels | any(. == "enhancement")) | {number, title, state}' pr/index.json

# Find "bug" fixes
jq '.[] | select(.labels | any(. == "bug")) | {number, title, state}' pr/index.json
```

### 4. Identify stale PRs
```bash
# PRs not updated in 6+ months
jq '.[] | select(.state == "open") | select(.updated_at < "2025-07-01") | {number, title, updated_at}' pr/index.json
```

### 5. Check PR size/complexity
```bash
# Large PRs (>500 line changes)
jq '.[] | select((.additions + .deletions) > 500) | {number, title, size: (.additions + .deletions)}' pr/index.json | jq -s 'sort_by(.size) | reverse'
```

### 6. Find PRs with unresolved discussions
```bash
# PRs with many review comments (potential concerns)
jq '.[] | select(.review_comments > 10) | {number, title, review_comments}' pr/index.json
```

## Example Workflow

```bash
# Set up authentication
export GITHUB_TOKEN=ghp_your_token

# Fetch all PRs (open, closed, merged)
python3 project/fetch_prs.py all

# Get overview
cat pr/README.md

# Find what's ready to merge (approved, no draft)
jq '.[] | select(.state == "open" and .draft == false) | {number, title}' pr/index.json

# Check specific PR details
cat pr/pr_1391.json | jq '{title, state, additions, deletions, comments: (.comment_chain | length), reviews: (.reviews | length)}'
```

## Integration with Issues

Since you have both `issues/` and `pr/` directories:

```bash
# Find PRs that mention specific issue
grep -l "#1234" pr/pr_*.json

# Cross-reference: PRs addressing "performance" issues
grep -l "performance" issues/issue_*.json
grep -l "performance" pr/pr_*.json
```
