#!/bin/bash
# Run the test suite against a downloaded release wheel inside a manylinux
# container (used for i686, which has no GitHub-hosted runner).
#
# Usage: test-in-manylinux.sh <python version, e.g. 3.12>
# Expects the repository at /io and the wheels in /io/wheels.
set -euo pipefail

pyver="$1"
tag="cp${pyver/./}"
PY="/opt/python/${tag}-${tag}/bin/python"
cd /io

# AlmaLinux 8 serves i686 packages only from vault.almalinux.org, which is
# occasionally unreachable; retry before giving up.
for attempt in 1 2 3 4 5; do
    yum install -y java-11-openjdk-devel ant gdb && break
    [ "$attempt" = 5 ] && exit 1
    echo "yum failed, retrying in 30s"; sleep 30
done
export JAVA_HOME=$(ls -d /usr/lib/jvm/java-11-openjdk* | head -n 1)
export PATH=$JAVA_HOME/bin:$PATH
java -version

"$PY" -m pip install --upgrade pip
version=$(ls wheels/jpype1-*.whl | head -n1 | sed -E 's/.*jpype1-([^-]+)-.*/\1/')
# Pin the version and refuse the sdist, so pip fails instead of
# silently falling back to a source build or an older PyPI release.
"$PY" -m pip install --only-binary jpype1 --find-links wheels "jpype1==${version}"
(cd /tmp && "$PY" -c "import jpype, _jpype; print(jpype.__version__, _jpype.__file__)")

# Several scientific packages no longer publish 32-bit wheels; do not build
# them from source, let the corresponding tests skip instead.
grep -vE '^\s*(#|$)' test-requirements.txt | while read -r req; do
    "$PY" -m pip install --only-binary :all: "$req" || echo "::warning::No i686 wheel for $req, skipped"
done

mkdir -p lib
wget -q --no-clobber "https://repo1.maven.org/maven2/org/apache/ivy/ivy/2.5.3/ivy-2.5.3.jar" -P lib \
    || curl -fsSL -o lib/ivy-2.5.3.jar "https://repo1.maven.org/maven2/org/apache/ivy/ivy/2.5.3/ivy-2.5.3.jar"
java -jar lib/ivy-2.5.3.jar -ivy ivy.xml -retrieve 'lib/[artifact]-[revision](-[classifier]).[ext]'
ant -f test/build.xml

ulimit -c unlimited || true
cd test
status=0
"$PY" -m pytest -v --junit-xml=test.xml jpypetest --checkjni -n2 --dist loadfile || status=$?
if [ "$status" != 0 ]; then
    core=$(ls core* 2>/dev/null | head -n1 || true)
    if [ -n "$core" ]; then gdb -batch -ex "thread apply all bt" "$PY" "$core"; fi
fi
exit "$status"
