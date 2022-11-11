#!/usr/bin/env bash

PYTHON_VERSION="$1"

case $PYTHON_VERSION in
3.7)
  FULL_VERSION=3.7.9
  INSTALLER_NAME=python-$FULL_VERSION-macosx10.9.pkg
  ;;
3.8)
  FULL_VERSION=3.8.10
  INSTALLER_NAME=python-$FULL_VERSION-macosx10.9.pkg
  ;;
3.9)
  FULL_VERSION=3.9.12
  INSTALLER_NAME=python-$FULL_VERSION-macosx10.9.pkg
  ;;
3.10)
  FULL_VERSION=3.10.4
  INSTALLER_NAME=python-$FULL_VERSION-macos11.pkg
  ;;
3.11)
  FULL_VERSION=3.11.0
  INSTALLER_NAME=python-$FULL_VERSION-macos11.pkg
  ;;
esac

URL=https://www.python.org/ftp/python/$FULL_VERSION/$INSTALLER_NAME

PY_PREFIX=/Library/Frameworks/Python.framework/Versions

set -e -x

curl $URL > $INSTALLER_NAME

sudo installer -pkg $INSTALLER_NAME -target /

sudo rm /usr/local/bin/python
sudo ln -s /usr/local/bin/python$PYTHON_VERSION /usr/local/bin/python

which python
python --version
python -m ensurepip
python -m pip install setuptools wheel
