#!/bin/sh
# Render the pending news fragments into doc/CHANGELOG.rst as an "unreleased"
# section, so documentation built from a PR or from master shows them.  Used
# by the Azure documentation job and by Read the Docs (.readthedocs.yml).
# --keep leaves the fragments in place; the edit is only meant for a CI
# checkout and must not be committed.
set -e

# No fragments (right after a release): nothing to render.
if ! find doc/changes -name '*.rst' ! -name template.rst | grep -q .; then
    echo "No news fragments, changelog left as is."
    exit 0
fi

python -m pip install towncrier
# Current version from jpype/__init__.py (e.g. 1.7.2.dev0), used as the
# section heading: "1.7.2.dev0 (unreleased) - <today>".
version=$(python -c "import re; print(re.search(r'__version__ = \"(.+)\"', open('jpype/__init__.py').read())[1])")
# Insert below ".. towncrier release notes start"; --keep: no prompt, do not
# delete the fragments.
python -m towncrier build --keep --version "$version (unreleased)"
