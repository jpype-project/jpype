# fetch_issues.py

Fetches all active issues from the JPype GitHub repository with full comment chains.

## Basic Usage

```bash
# Fetch open issues (default)
python3 project/fetch_issues.py

# Fetch closed issues
python3 project/fetch_issues.py closed

# Fetch all issues
python3 project/fetch_issues.py all
```

## Avoiding Rate Limits

GitHub's unauthenticated API has a limit of **60 requests/hour**. Since fetching comments requires one request per issue, you'll hit this limit quickly.

To fetch all comments, use a GitHub Personal Access Token:

### 1. Create a token
- Go to https://github.com/settings/tokens
- Click "Generate new token (classic)"
- Give it a name like "JPype Issue Fetcher"
- Select **no scopes** (read-only public data doesn't need permissions)
- Click "Generate token"
- Copy the token (starts with `ghp_...`)

### 2. Use the token

```bash
export GITHUB_TOKEN=ghp_your_token_here
python3 project/fetch_issues.py
```

With authentication, you get **5000 requests/hour**.

## Output Structure

All data is saved to the `project/issues/` directory (already in `.gitignore`):

- **`project/issues/README.md`** - Human-readable summary with links
- **`project/issues/index.json`** - Structured index with metadata
- **`project/issues/issue_*.json`** - Individual issue files with full data

### Issue JSON Structure

Each `issue_*.json` file contains:
- `title`, `body` - Issue title and description
- `state`, `created_at`, `updated_at` - Status and timestamps
- `labels` - Issue labels
- `user` - Author information
- `comments` - Number of comments
- **`comment_chain`** - Array of all comments with full text, author, timestamps

Example comment structure:
```json
{
  "comment_chain": [
    {
      "id": 123456,
      "user": {"login": "username"},
      "body": "Full comment text here...",
      "created_at": "2025-01-15T10:30:00Z",
      "updated_at": "2025-01-15T10:30:00Z"
    }
  ]
}
```

## Example Analysis Workflow

```bash
# Fetch issues with authentication
export GITHUB_TOKEN=ghp_your_token
python3 project/fetch_issues.py

# Find issues with most comments
jq '.[] | {number, title, comments}' project/issues/index.json | jq -s 'sort_by(.comments) | reverse | .[0:10]'

# Search for keyword in issue bodies
grep -l "performance" project/issues/issue_*.json

# Find all issues labeled "bug"
jq '.[] | select(.labels | any(. == "bug")) | {number, title}' project/issues/index.json
```
