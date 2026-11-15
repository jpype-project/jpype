#!/bin/bash
# Runs every project/benchmark/graalpy/*.py benchmark sequentially (never
# concurrently -- this machine has only 7.7GB RAM, see this repo's
# CLAUDE.md and the OOM incident during this harness's own setup) with a
# capped JVM heap, logging each script's stdout/stderr to its own file
# next to its CSV output.
set -u
# Always the GraalVM CE JDK, regardless of what this shell's profile sets
# JAVA_HOME to elsewhere (this repo's normal JAVA_HOME, e.g. a Temurin
# JDK, has no bundled libgraal -- running under it silently falls back to
# an interpreter-only "fallback runtime", defeating the entire point of
# this comparison; see pom.xml's version-pin comment).
export JAVA_HOME="$HOME/.local/graalvm-community-openjdk-25.0.2+10.1"
cd "$(dirname "$0")"
CP="target/classes:target/lib/*:../../../test/classes:../../../test/harness"
LOGDIR="run_logs"
mkdir -p "$LOGDIR"

FILES="int.py double.py strings.py object.py dispatch.py proxy.py array_flat.py array_multidim.py array_ragged.py array_noncontig.py array_shape.py"

for f in $FILES; do
    echo "=== running $f ==="
    "$JAVA_HOME/bin/java" -Xmx3g -Dpolyglot.engine.CompilerThreads=2 \
        --enable-native-access=ALL-UNNAMED -cp "$CP" \
        org.jpype.bench.graalpy.Bench "$f" > "$LOGDIR/${f%.py}.log" 2>&1
    echo "=== $f exit code: $? ==="
done
echo "=== ALL DONE ==="
