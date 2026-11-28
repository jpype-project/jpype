#!/bin/bash
set -e -x

# Ensure the package name is available (usually passed from Azure env)
package_name=${package_name:-jpype1}

# BUILD_PY_VERSIONS: space-separated minor versions to build.
# e.g. "38 39" or "310 311 312 313 314"
# Defaults to modern versions if not passed in.
if [ -n "$BUILD_PY_VERSIONS" ]; then
    IFS=' ' read -r -a include_versions <<< "$BUILD_PY_VERSIONS"
else
    include_versions=("310" "311" "312" "313" "314" "315")
fi

echo "Building for Python versions: ${include_versions[*]}"

pys=()
echo "Available Python bins in ManyLinux image:"
ls -d /opt/python/cp*/bin

for pybin in /opt/python/cp*/bin; do
    dir=$(basename $(dirname "$pybin"))
    matched=false
    for v in "${include_versions[@]}"; do
        if [[ "$dir" =~ ^cp${v}- ]]; then
            matched=true
            break
        fi
    done
    if $matched; then
        echo "Including: $dir"
        pys+=("$pybin")
    else
        echo "Skipping:  $dir"
    fi
done

echo "Found Python bins for build: ${pys[@]}"

if command -v apk >/dev/null 2>&1; then
    # musllinux images are Alpine based
    apk add --no-cache openjdk11-jdk apache-ant || apk add --no-cache openjdk17-jdk apache-ant
    export JAVA_HOME=$(ls -d /usr/lib/jvm/java-11-openjdk* /usr/lib/jvm/java-17-openjdk* 2>/dev/null | head -n 1)
else
    # AlmaLinux 8 serves i686 packages only from vault.almalinux.org; retry
    # and show name resolution if it is unreachable.
    for attempt in 1 2 3 4 5; do
        yum install -y java-11-openjdk-devel ant && break
        getent hosts vault.almalinux.org || cat /etc/resolv.conf
        [ "$attempt" = 5 ] && exit 1
        echo "yum failed, retrying in 30s"; sleep 30
    done
    export JAVA_HOME=$(ls -d /usr/lib/jvm/java-11-openjdk* | head -n 1)
fi
export JAVAC=$JAVA_HOME/bin/javac
export PATH=$JAVA_HOME/bin:$PATH

which javac
java -version
javac -version


# Compile wheels
for PYBIN in "${pys[@]}"; do
    echo "=================================================="
    echo "Processing: $PYBIN"

    # 1. Extraction (No-collision quoting)
    PYTHON_INCLUDE=$("${PYBIN}/python" -c "import sysconfig; print(sysconfig.get_paths()[\"include\"])")
    
    echo "--- Forensic Path Interrogation ---"
    echo "INCLUDE:   $PYTHON_INCLUDE"

    # 2. Build with 'Module' strategy
    # We drop -DPython3_LIBRARY because the file is missing from the image.
    # On Linux, 'Development.Module' only requires headers to build the extension.
    "${PYBIN}/pip" wheel /io/ -w wheelhouse/ -v --no-deps \
        --config-setting=cmake.args="-DPython3_EXECUTABLE=${PYBIN}/python;\
        -DPython3_INCLUDE_DIR=${PYTHON_INCLUDE};\
        -DPython3_FIND_STRATEGY=LOCATION;\
        -DPython3_FIND_COMPONENTS='Interpreter;Development.Module';\
        -DJAVA_HOME=${JAVA_HOME}"

done

echo "=============="
echo "Repairing wheels with auditwheel"
# Bundle external shared libraries into the wheels (standard ManyLinux procedure)
repair_args=()
if [[ "$PLAT" == musllinux* ]]; then
    # The musllinux policy would graft a private libstdc++/libgcc_s into the wheel,
    # while the JVM's libjvm.so loads the system ones: two C++ runtimes in one
    # process. Link against the system runtime instead (always present wherever an
    # Alpine JDK is installed), as manylinux does.
    repair_args+=(--exclude libstdc++.so.6 --exclude libgcc_s.so.1)
fi
for whl in wheelhouse/${package_name}-*.whl; do
    echo "Audit $whl"
    # Repair and move to the final output directory /io/wheelhouse
    auditwheel repair --plat "$PLAT" "${repair_args[@]}" "$whl" -w /io/wheelhouse/
done
echo "=============="

# Simple verification
for PYBIN in "${pys[@]}"; do
    echo "Verifying installation for $PYBIN"
    "${PYBIN}/python" -m pip install ${package_name} --no-index -f /io/wheelhouse
    "${PYBIN}/python" -c "import jpype; print(\"JPype version {} installed successfully\".format(jpype.__version__))"
done

if [[ "$PLAT" == musllinux* ]]; then
    # Smoke test: start the JVM and unwind a Java exception through _jpype.so.
    for PYBIN in "${pys[@]}"; do
        "${PYBIN}/python" - <<'PYEOF'
import jpype
jpype.startJVM()
from java.lang import Integer, NumberFormatException
try:
    Integer.parseInt("not a number")
except NumberFormatException:
    print("JVM start and exception round trip OK")
else:
    raise SystemExit("expected NumberFormatException")
PYEOF
    done
fi

