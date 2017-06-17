# Check versions
ant -version
python --version

# Get the arch size
python.exe -c "import struct; print(struct.calcsize('P') * 8)"

# Install prereqs
pip install nose
pip install setuptools

# Build the test harness
ant -f test/build.xml

# Install the package
python setup.py install

