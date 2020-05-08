# This script was used to scrub the harness of old code
#   First a set of sed patterns found all the harness classes
#   that are in used directly.  They were moved out of place using,
#   this script, then those that were used indirectly were moved
#   back in place.
import glob
import re
import os

with open("keep", "r") as fd:
    lines = fd.readlines()
    lines = [i.strip() for i in lines]
    keep = set(lines)

for fl in glob.iglob('harness/**/*.java', recursive=True):
    m = re.match('.*(jpype.*).java', fl)
    name = m.group(1).replace('/', '.')
    if name in keep:
        continue
    m = re.match('(.*)/[^/]*.java', fl)
    os.makedirs(os.path.join("old", m.group(1)), exist_ok=True)
    os.rename(fl, os.path.join("old", fl))
