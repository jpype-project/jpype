$env:Path += ";"+$env:PYTHON

# Check that we have the expected version and architecture for Python
ant.exe -version
python.exe --version
python.exe -c "import struct; print(struct.calcsize('P') * 8)"

# Install the build dependencies of the project. If some dependencies contain
# compiled extensions and are not provided as pre-built wheel packages,
# pip will build them from source using the MSVC compiler matching the
# target Python version and architecture

#seem like pip is not installed in 3.4 
if ($env:CONDA_PY -eq "3.4.3"){
   curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py | python
}

git clone --depth=1 https://github.com/pypa/setuptools.git
python .\setuptools\bootstrap.py
python -m pip install --upgrade .\setuptools

git clone --depth=1 https://github.com/pypa/wheel.git
python -m pip install --upgrade .\wheel

git clone --depth=1 https://github.com/pypa/pip.git
python -m pip install --upgrade .\pip
Remove-Item .\pip -Force -Recurse
Remove-Item .\setuptools -Force -Recurse
Remove-Item .\wheel -Force -Recurse

pip install --upgrade git+https://github.com/pypa/setuptools_scm.git
pip install --upgrade nose -r test-requirements.txt
#pip.exe install -r "test-requirements.txt" # -r dev-requirements.txt

ant.exe -f test\\build.xml

# Build the compiled extension and run the project tests
python.exe setup.py bdist_wheel
dir .\dist
Get-ChildItem -File -Path .\dist\*.whl | Foreach {pip install --upgrade $_.fullname}
