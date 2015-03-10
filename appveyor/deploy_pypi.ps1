function deploy() {
    pip install twine wheel
    
    python setup.py bdist_wheel
    
    twine upload -u $env:PYPI_USER -p $env:PYPI_PASS dist/*
}


# if we have a tagged commit (new version) we build and deploy wheels
if ($env:APPVEYOR_REPO_TAG) {
    deploy
}
