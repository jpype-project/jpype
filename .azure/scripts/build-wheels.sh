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
    include_versions=("310" "311" "312" "313" "314")
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

yum install -y java-11-openjdk-devel ant
export JAVA_HOME=$(ls -d /usr/lib/jvm/java-11-openjdk-* | head -n 1)
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
for whl in wheelhouse/${package_name}-*.whl; do
    echo "Audit $whl"
    # Repair and move to the final output directory /io/wheelhouse
    auditwheel repair --plat "$PLAT" "$whl" -w /io/wheelhouse/
done
echo "=============="

# Simple verification
for PYBIN in "${pys[@]}"; do
    echo "Verifying installation for $PYBIN"
    "${PYBIN}/python" -m pip install ${package_name} --no-index -f /io/wheelhouse
    "${PYBIN}/python" -c "import jpype; print(\"JPype version {} installed successfully\".format(jpype.__version__))"
done

