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
VER=0.7.1
for PLAT in x86_64 i686
do
	DOCKER_IMAGE=quay.io/pypa/manylinux1_$PLAT
	for PY in cp35-cp35m cp36-cp36m cp37-cp37m cp38-cp38
	do
		docker run --rm -e VER=$VER -e PY=$PY -e PLAT=$PLAT -v `pwd`:/io $DOCKER_IMAGE /io/build.sh
	done
done
