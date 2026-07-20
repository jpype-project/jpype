# fetch_codescan.py

Fetches code scanning alerts (CodeQL and any other configured tools) from the JPype
GitHub Security tab: https://github.com/jpype-project/jpype/security/code-scanning

## Basic Usage

```bash
# Fetch open alerts (default)
python3 project/fetch_codescan.py

# Fetch dismissed alerts
python3 project/fetch_codescan.py dismissed

# Fetch fixed alerts
python3 project/fetch_codescan.py fixed

# Fetch everything regardless of state
python3 project/fetch_codescan.py all
```

Note the state vocabulary is `open`/`dismissed`/`fixed`/`all` - not `open`/`closed`/`all`
like `fetch_prs.py`/`fetch_issues.py`. Code scanning alerts don't have a single "closed"
bucket: dismissed (a human decided it's not an issue - false positive, won't fix, used in
tests, etc.) and fixed (the flagged code changed) are worth keeping distinct for triage.

## Commands

The bare `[open|dismissed|fixed|all]` form above is shorthand for `fetch [state]`. The full
set of subcommands:

```bash
# Bulk (re)fetch by state - writes/overwrites what's fetched, leaves anything
# already cached but not returned this run alone.
python3 project/fetch_codescan.py fetch [open|dismissed|fixed|all]

# Re-fetch specific alert(s) by number and update them in place - e.g. after
# dismissing/fixing one and wanting the local cache to reflect it right away.
python3 project/fetch_codescan.py refresh 42 [43 ...]

# Re-fetch every alert currently cached locally (refresh applied to all cached numbers).
python3 project/fetch_codescan.py update

# Remove cached alerts that are no longer open on GitHub (dismissed, fixed, or gone).
python3 project/fetch_codescan.py cleanup
```

`refresh`/`update`/`cleanup` always rebuild `index.json`/`README.md` from the full local
cache afterward, not just the numbers touched.

## Authentication (Required, and a Stricter Scope Than the Sibling Scripts)

This endpoint needs a GitHub **classic** token with the **`security_events`** scope.
The no-scope token that's enough for `fetch_prs.py`/`fetch_issues.py` is **not** enough
here - GitHub returns `403 Resource not accessible by personal access token` if the scope
is missing, even though the token works fine for issues/PRs.

```bash
export GITHUB_TOKEN=ghp_your_token_with_security_events_scope
python3 project/fetch_codescan.py fetch
```

To add the scope: https://github.com/settings/tokens → edit the token → check
`security_events` → regenerate.

## Output Structure

All data is saved to the `project/codescan/` directory (already in `.gitignore`):

- **`project/codescan/README.md`** - Human-readable summary with rule, location, and message
- **`project/codescan/index.json`** - Structured index with metadata
- **`project/codescan/alert_*.json`** - Individual alert files with complete data

### Alert JSON Structure

Each `alert_*.json` file is GitHub's raw code scanning alert object:

- `number`, `state` - Alert number and state ("open"/"dismissed"/"fixed")
- `rule` - `{id, description, severity, security_severity_level, tags}`
- `tool` - `{name, version}` (e.g. CodeQL)
- `most_recent_instance` - `{location: {path, start_line, end_line}, message: {text}}`
- `created_at`, `updated_at`
- `dismissed_at`, `dismissed_by`, `dismissed_reason`, `dismissed_comment` (null unless dismissed)
- `fixed_at` (null unless fixed)
- `html_url` - link to the alert on GitHub

## Example Analysis Workflow

```bash
export GITHUB_TOKEN=ghp_your_token_with_security_events_scope
python3 project/fetch_codescan.py fetch

# Overview
cat project/codescan/README.md

# Group open alerts by rule
jq '.[] | select(.state == "open") | .rule_id' project/codescan/index.json | sort | uniq -c | sort -rn

# Find alerts in a specific file
jq '.[] | select(.path | startswith("native/"))' project/codescan/index.json

# Cross-reference a PR's flagged file with any matching cached alert
grep -l "gc_exception_race_soak.py" project/codescan/alert_*.json
```
