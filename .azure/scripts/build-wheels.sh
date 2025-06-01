#!/bin/bash
set -e -x

pys=()
echo "Available Python bins:"
ls -d /opt/python/cp*/bin 

for pybin in /opt/python/cp*/bin; do
    # Exclude 3.6, 3.7, 3.8 and any alpha/beta/rc release
    if [[ "$dir" =~ ^cp3(6|7|8|9|14)-cp3(6|7|8|9|14)t?$ ]]; then
        continue
    fi
    pys+=("$pybin")
done

# Show what you found for debugging
echo "Found Python bins:"
printf '%s\n' "${pys[@]}"

# Compile wheels
for PYBIN in "${pys[@]}"; do
    echo "Compile $PYBIN"
    ls -l /io/dist
    "${PYBIN}/pip" install -r /io/dev-requirements.txt
    "${PYBIN}/pip" wheel /io/dist/$package_name-*.tar.gz -w wheelhouse/ -v
done
echo "=============="

# Bundle external shared libraries into the wheels
for whl in wheelhouse/$package_name-*.whl; do
    echo "Audit $whl"
    auditwheel repair --plat $PLAT "$whl" -w /io/wheelhouse/
done
echo "=============="

# Install packages and test
for PYBIN in "${pys[@]}"; do
    echo "Test install $PYBIN $package_name"
    "${PYBIN}/python" -m pip install $package_name --no-index -f /io/wheelhouse
    # Manylinux does not have a JVM so there is no way to test the wheel in the docker
    # "${PYBIN}/pip" install -r /io/test-requirements.txt
    # "${PYBIN}/pytest" /io/test/jpypetest
done
