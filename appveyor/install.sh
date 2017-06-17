# Setup cygwin path

export PATH="$ANT_BIN/bin:/bin:/usr/bin"

echo ARCH=$ARCH
echo PATH=$PATH
echo PYTHON=$PYTHON

# Install prereqs
if [ $PYTHON = "python3" ]; then
	/setup-$ARCH -q -P python3,python3-numpy,python3-devel,python3,python3-setuptools,python3-nose
	PIP=pip3
else
	/setup-$ARCH -q -P python,python-numpy,python-devel,python,python-setuptools,python-nose
	PIP=pip
fi

# Check versions
"$ANT_HOME"/bin/ant -version
$PYTHON --version

# Get the arch size
$PYTHON -c "import struct; print(struct.calcsize('P') * 8)"

# Build the test harness
"$ANT_HOME"/bin/ant -f test/build.xml

# Install the package
$PYTHON setup.py install

