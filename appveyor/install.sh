# Setup cygwin path

export PATH="$ANT_BIN/bin:/bin:/usr/bin"

echo ARCH=$ARCH
echo PATH=$PATH
echo PYTHON=$PYTHON

# Define programs
SETUP=/setup-$ARCH
if [ $PYTHON = "python3" ]; then
	PIP=pip3
else
	PIP=pip
fi

# Install prereqs
$SETUP -q -P gcc-core, gcc-g++
$SETUP -q -P $PYTHON,$PYTHON-numpy,$PYTHON-devel,$PYTHON,$PYTHON-setuptools,$PYTHON-nose

# Check versions
"$ANT_HOME"/bin/ant -version
$PYTHON --version

# Get the arch size
$PYTHON -c "import struct; print(struct.calcsize('P') * 8)"

# Build the test harness
"$ANT_HOME"/bin/ant -f test/build.xml

# Install the package
$PYTHON setup.py install

