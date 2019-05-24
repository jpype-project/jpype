$env:Path += ";"+$env:PYTHON

# Check that we have the expected version and architecture for Python
ant.exe -version
python.exe --version
python.exe -c "import struct; print(struct.calcsize('P') * 8)"

# Install the build dependencies of the project. If some dependencies contain
# compiled extensions and are not provided as pre-built wheel packages,
# pip will build them from source using the MSVC compiler matching the
# target Python version and architecture
pip.exe install setuptools -r test-requirements.txt
pip.exe install pytest==4.5.0
#pip.exe install -r "test-requirements.txt" # -r dev-requirements.txt

ant.exe -f test\\build.xml

# Build the compiled extension and run the project tests
python.exe setup.py --enable-build-jar bdist_wheel
dir .\dist
Get-ChildItem -File -Path .\dist\*.whl | Foreach {pip install --upgrade $_.fullname}
