#!/bin/bash

# make TARGET overrideable with env
: ${TARGET:=$HOME/miniconda}

function install_miniconda {
	if [ -d $TARGET ]; then echo "file exists"; return; fi
	echo "installing miniconda to $TARGET"
        if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then                                 
             platform="Linux"
        elif [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
             platform="MacOSX"
        fi     
	wget http://repo.continuum.io/miniconda/Miniconda3-latest-$platform-x86_64.sh -O mc.sh -o /dev/null
	bash mc.sh -b -f -p $TARGET
}

install_miniconda
export PATH=$TARGET/bin:$PATH
hash -r
conda config --set always_yes True
conda config --set quiet True
conda config --add channels conda-forge

conda install conda-build -c defaults

