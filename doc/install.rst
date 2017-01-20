Installation
============

Get JPype from the `github <https://github.com/originell/jpype>`__ or
from `PyPi <http://pypi.python.org/pypi/JPype1>`__. If you are using `Anaconda <https://anaconda.org>`_ Python stack,
you can install pre-compiled binaries from conda-forge for Linux, OSX and Windows.

Binary Install
--------------
1. Ensure you have installed Anaconda/Miniconda. Instructions can be found `here <http://conda.pydata.org/docs/install/quick.html>`_.
2. Install from the conda-forge software channel::

    conda install -c conda-forge jpype1

From source - Requirements
--------------------------

Either the Sun/Oracle JDK/JRE Variant or OpenJDK. Python 2.6+ (including Python 3+).

Debian/Ubuntu
~~~~~~~~~~~~~

Debian/Ubuntu users will have to install ``g++`` and ``python-dev``
first:

::

    sudo apt-get install g++ python-dev

Windows
~~~~~~~

Windows users need a Python installation and C++ compilers:

1. Install some version of Python (2.7 or higher), e.g., `Anaconda
   <https://www.continuum.io/downloads>`_ is a good choice for users not yet
   familiar with the language
2. Install a `Windows C++ Compiler
   <http://landinghub.visualstudio.com/visual-cpp-build-tools>`_

Install
-------

Should be easy as

::

    python setup.py install


If it fails...
~~~~~~~~~~~~~~

This happens mostly due to the setup not being able to find your
``JAVA_HOME``. In case this happens, please do two things:

1. You can continue the installation by finding the ``JAVA_HOME`` on
   your own ( the place where the headers etc. are) and explicitly
   setting it for the installation:

   ``JAVA\_HOME=/usr/lib/java/jdk1.6.0/ python setup.py install``
2. Please create an Issue `on
   github <https://github.com/originell/jpype/issues?state=open>`__ and
   post all the information you have.

Tested on
---------

-  OS X 10.7.4 with Sun/Oracle JDK 1.6.0
-  OSX 10.8.1-10.8.4 with Sun/Oracle JDK 1.6.0
-  OSX 10.9 DP5
-  Debian 6.0.4/6.0.5 with Sun/Oracle JDK 1.6.0
-  Debian 7.1 with OpenJDK 1.6.0
-  Ubuntu 12.04 with Sun/Oracle JDK 1.6.0


Known Bugs/Limitations
----------------------

-  Java classes outside of a package (in the ``<default>``) cannot be
   imported.
-  unable to access a field or method if it conflicts with a python
   keyword.
-  Because of lack of JVM support, you cannot shutdown the JVM and then
   restart it.
-  Some methods rely on the "current" class/caller. Since calls coming
   directly from python code do not have a current class, these methods
   do not work. The User Manual lists all the known methods like that.
-  Mixing 64 bit Python with 32 bit Java and vice versa crashes on import jpype.
