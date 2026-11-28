#!/usr/bin/env python3
"""Download the wheels for one platform from an Azure ``jpype.release`` build.

Used by ``.github/workflows/test-release-wheels.yml``.  Without ``--build-id``
the newest completed build that has wheels for the platform is taken.  Release
builds are often reported as "failed" because a single platform job failed, so
the build result is not used for this choice.

Writes the wheels to ``--out`` and, with ``--github-output``, appends
``build_id`` and ``python`` (a JSON list such as ``["3.10", "3.11"]``) to that
file.  Free-threaded wheels (``cp314t``) are ignored.
"""

import argparse
import io
import json
import re
import sys
import urllib.request
import zipfile
from pathlib import Path

API = "https://dev.azure.com/jpype-project/jpype/_apis/build"
RELEASE_DEFINITION = 2  # jpype.release


def get_json(url):
    with urllib.request.urlopen(url, timeout=60) as resp:
        return json.load(resp)


def platform_artifacts(build_id, platform):
    data = get_json(f"{API}/builds/{build_id}/artifacts?api-version=7.1")
    return [a for a in data["value"] if platform in a["name"]]


def find_build(platform, branch, limit=20):
    url = (f"{API}/builds?definitions={RELEASE_DEFINITION}"
           f"&statusFilter=completed&$top={limit}&api-version=7.1")
    if branch:
        url += f"&branchName=refs/heads/{branch}"
    for build in get_json(url)["value"]:
        if platform_artifacts(build["id"], platform):
            return build["id"]
    sys.exit(f"No completed jpype.release build among the last {limit} "
             f"has wheels for {platform}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--platform", required=True,
                        help="wheel platform tag, e.g. manylinux_2_28_aarch64")
    parser.add_argument("--build-id", default="", help="Azure build id")
    parser.add_argument("--branch", default="",
                        help="only consider builds of this branch")
    parser.add_argument("--out", default="wheels", type=Path)
    parser.add_argument("--github-output", type=Path)
    args = parser.parse_args()

    build_id = args.build_id or find_build(args.platform, args.branch)
    artifacts = platform_artifacts(build_id, args.platform)
    if not artifacts:
        sys.exit(f"Azure build {build_id} has no artifacts for {args.platform}")

    args.out.mkdir(parents=True, exist_ok=True)
    versions = set()
    for artifact in artifacts:
        print(f"Build {build_id}: {artifact['name']}")
        url = artifact["resource"]["downloadUrl"]
        with urllib.request.urlopen(url, timeout=300) as resp:
            archive = zipfile.ZipFile(io.BytesIO(resp.read()))
        for name in archive.namelist():
            wheel = Path(name).name
            if not (wheel.endswith(".whl") and args.platform in wheel):
                continue
            m = re.search(r"-cp(\d)(\d+)-cp\d+(t?)-", wheel)
            if not m or m.group(3):
                print(f"  skip {wheel}")
                continue
            (args.out / wheel).write_bytes(archive.read(name))
            versions.add((int(m.group(1)), int(m.group(2))))
            print(f"  got  {wheel}")

    if not versions:
        sys.exit(f"No {args.platform} wheels found in build {build_id}")
    python = [f"{major}.{minor}" for major, minor in sorted(versions)]
    print(f"Python versions: {' '.join(python)}")
    if args.github_output:
        with args.github_output.open("a") as fh:
            fh.write(f"build_id={build_id}\n")
            fh.write(f"python={json.dumps(python)}\n")


if __name__ == "__main__":
    main()
