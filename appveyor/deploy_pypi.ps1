function deploy() {
    pip install twine wheel
    
    # with numpy
    python setup.py bdist_wheel
    
    # now again without numpy
    rm -r build
    python setup.py bdist_wheel --disable-numpy
    
    # upload to pypi
    ls dist
    twine upload -u $env:PYPI_USER -p $env:PYPI_PASS dist/*
}


# if we have a tagged commit (new version) we build and deploy wheels
if ($env:APPVEYOR_REPO_TAG) {
    deploy
}
