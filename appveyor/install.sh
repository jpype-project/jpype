# Setup cygwin path

export PATH="/bin:/usr/bin:$PATH"

echo JAVA_HOME=$JAVA_HOME
echo ARCH=$ARCH
echo PATH=$PATH
echo PYTHON=$PYTHON
echo ANT_HOME=$ANT_HOME

# If we do not have Java installed we can't proceed
if [ ! -d "$JAVA_HOME" ]; then
	echo "JAVA_HOME is not valid"
	exit -1
fi

# Make sure the jvm.dll is where it should be
find "$JAVA_HOME" -name "jvm.dll"

# Define programs
SETUP=/setup-$ARCH

# Install prereqs
echo "==== update gcc"
$SETUP -q -P gcc-core,gcc-g++,libcrypt-devel
echo "==== update python"
$SETUP -q -P $PYTHON,$PYTHON-numpy,$PYTHON-devel,$PYTHON,$PYTHON-setuptools
echo "==== get modules"
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
$PYTHON get-pip.py
git clone --depth=1 https://github.com/pypa/setuptools.git
cd setuptools
$PYTHON ./bootstrap.py
$PYTHON -m pip install ./
cd ..
rm -r ./setuptools

git clone --depth=1 https://github.com/pypa/wheel.git
git clone --depth=1 https://github.com/pypa/pip.git
git clone --depth=1 https://github.com/pypa/setuptools_scm.git

$PYTHON -m pip install mock
$PYTHON -m pip install --upgrade ./pip ./wheel ./setuptools_scm
$PYTHON -m pip install pytest==4.5.0

rm -r ./pip ./wheel ./setuptools_scm

# Check versions
echo "==== Check versions"
"$ANT_HOME/bin/ant" -version
$PYTHON --version
"$JAVA_HOME/bin/java.exe" -version

echo "==== Check architectures"
file -L `which $PYTHON`
file -L "$JAVA_HOME/bin/java.exe"
file -L `find "$JAVA_HOME" -name "jvm.dll"`

echo "==== Check modules"
$PYTHON -c 'import pip; print(sorted(["%s==%s" % (i.key, i.version) for i in pip.get_installed_distributions()]))'

# Get the arch size
echo "==== Check arch"
$PYTHON -c "import struct; print(struct.calcsize('P') * 8)"

# Build the test harness
echo "==== Build test"
"$ANT_HOME/bin/ant" -f test/build.xml

# Install the package
echo "==== Build module"
$PYTHON ./setup.py --enable-build-jar bdist_wheel
$PYTHON -m pip install --upgrade ./dist/*.whl

echo "==== Verify jvm.dll found"
$PYTHON -c "import jpype; print(jpype.getDefaultJVMPath())"
