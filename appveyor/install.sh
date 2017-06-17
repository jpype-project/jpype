# Setup cygwin path
export PATH="$ANT_HOME/bin:/bin:/usr/bin"

echo /bin
ls /bin

echo ANT_HOME=$ANT_HOME
echo PATH=$PATH
echo PYTHON=$PYTHON
echo PIP=$PIP

# Check versions
ant -version
$PYTHON --version

# Get the arch size
$PYTHON -c "import struct; print(struct.calcsize('P') * 8)"

# Install prereqs
$PIP install nose
$PIP install setuptools

# Build the test harness
ant -f test/build.xml

# Install the package
$PYTHON setup.py install

