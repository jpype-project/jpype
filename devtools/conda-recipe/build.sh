which g++

export CCACHE_BASEDIR=${SRC_DIR}
export CCACHE_NOHASHDIR="true"
export CCACHE_DIR=$HOME/.ccache_conda_bld

python setup.py install --single-version-externally-managed --record record.txt

ccache -s

