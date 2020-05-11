#!/bin/bash
set -e -x
echo ====== $PY $VER
PYBIN=/opt/python/$PY/bin
if [ ! -e /io/wheelhouse/JPype1-$VER-$PY-linux_$PLAT.whl ]; then
	"${PYBIN}/pip" install numpy==$NUMPY
	"${PYBIN}/pip" wheel /io/JPype1-$VER.tar.gz --no-index -w /io/wheelhouse
fi
auditwheel repair /io/wheelhouse/JPype1-$VER-$PY-linux_$PLAT.whl -w /io/wheelhouse
rm /io/wheelhouse/JPype1-$VER-$PY-linux_$PLAT.whl
