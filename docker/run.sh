#!/bin/sh
# Pull the required images
#docker pull quay.io/pypa/manylinux1_x86_64
#docker pull quay.io/pypa/manylinux1_i686

# Available in manylinux
#   cp27-cp27m
#   cp27-cp27mu
#   cp34-cp34m
#   cp35-cp35m
#   cp36-cp36m
#   cp37-cp37m
#   cp38-cp38

# Build each wheel
VER=0.7.3
root=`pwd`
root=${root#/mnt}
for PLAT in x86_64 i686
do
	DOCKER_IMAGE=quay.io/pypa/manylinux1_$PLAT
	# Different versions need different NUMPY as NUMPY support for Python varies by version
	docker run --rm -e VER=$VER -e PY=cp35-cp35m -e NUMPY=1.15.0  -e PLAT=$PLAT -v $root:/io $DOCKER_IMAGE /io/build.sh
	docker run --rm -e VER=$VER -e PY=cp36-cp36m -e NUMPY=1.15.0  -e PLAT=$PLAT -v $root:/io $DOCKER_IMAGE /io/build.sh
	docker run --rm -e VER=$VER -e PY=cp37-cp37m -e NUMPY=1.15.0  -e PLAT=$PLAT -v $root:/io $DOCKER_IMAGE /io/build.sh
	docker run --rm -e VER=$VER -e PY=cp38-cp38  -e NUMPY=1.18.0  -e PLAT=$PLAT -v $root:/io $DOCKER_IMAGE /io/build.sh
done
