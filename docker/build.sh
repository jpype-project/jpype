#!/bin/bash
set -e -x
echo ====== $PY $VER
PYBIN=/opt/python/$PY/bin
"${PYBIN}/pip" install -r /io/dev-requirements.txt
"${PYBIN}/pip" wheel /io/JPype1-$VER.tar.gz -w /io/wheelhouse
