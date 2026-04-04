#!/bin/bash
set -e -x

# Ensure the package name is available (usually passed from Azure env)
package_name=${package_name:-jpype1}

pys=()
echo "Available Python bins in ManyLinux image:"
ls -d /opt/python/cp*/bin

for pybin in /opt/python/cp*/bin; do
    # Get the directory name (e.g., cp310-cp310)
    dir=$(basename $(dirname "$pybin"))

    # EXCLUSION LOGIC:
    # Drop EOL versions (3.6, 3.7, 3.8, 3.9)
    # Note: 3.9 is EOL as of late 2025, but keep it if your users require it.
    if [[ "$dir" =~ ^cp3(6|7|8|9)- ]]; then
        echo "Skipping legacy version: $dir"
        continue
    fi

    # Include 3.10, 3.11, 3.12, 3.13, 3.14
    if [[ "$dir" =~ ^cp3(10|11|12|13|14) ]]; then
        pys+=("$pybin")
    fi
done

echo "Found Python bins for build: ${pys[@]}"

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
-Dskbuild.cmake_define.Python3_FIND_COMPONENTS=Interpreter;Development.Module"

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
    "${PYBIN}/python" -c "import jpype; print(f'JPype version {jpype.__version__} installed successfully')"
done

