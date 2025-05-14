Installation
============

JPype is available either as a pre-compiled binary for Anaconda, or may be
built from source though various methods.


Binary Install
--------------

JPype can be installed as pre-compiled binary if you are using the `Anaconda
<https://anaconda.org>`_ Python stack. Binaries are available for Linux, OSX,
and Windows on conda-forge.

1. Ensure you have installed Anaconda/Miniconda. Instructions can be found
   `here <http://conda.pydata.org/docs/install/quick.html>`__.
2. Install from
   the conda-forge software channel::

    conda install -c conda-forge jpype1


Source Install
--------------

Installing from source requires:

Python
  JPype works CPython 3.5 or later. Both the runtime and the development
  package are required.

Java
  Either the Sun/Oracle JDK/JRE Variant or OpenJDK.

  JPype source distribution includes a copy of the Java JNI header
  and precompiled Java code, thus the Java Development Kit (JDK) is not required.
  JPype has been tested with Java versions from Java 1.8 to Java 13.

C++
  A C++ compiler which matches the ABI used to build CPython.

JDK
  *(Optional)* JPype contains sections of Java code. These sections are
  precompiled in the source distribution, but must be built when installing 
  directly from the git repository.

Once these requirements have been met, one can use pip to build from either the
source distribution or directly from the repository.  Specific requirements from
different achitectures are listed below_.


Build using pip
~~~~~~~~~~~~~~~

JPype may be built and installed with one step using pip.

To install the latest JPype, use: ::

  pip install JPype1

This will install JPype either from source or binary distribution, depending on
your operating system and pip version.

To install from the current github master use: ::

  pip install git+https://github.com/jpype-project/jpype.git

More details on installing from git can be found at `Pip install
<https://pip.pypa.io/en/stable/reference/pip_install/#git>`__.  The git version
does not include a prebuilt jar the JDK is required.


Build and install manually
~~~~~~~~~~~~~~~~~~~~~~~~~~

JPype can be built entirely from source.

**1. Get the JPype source**

The JPype source may be acquired from either 
`github <https://github.com/jpype-project/jpype>`__ or
from `PyPi <http://pypi.python.org/pypi/JPype1>`__. 

**2. Build the source with desired options**

Compile JPype using the `build <https://pypi.org/project/build/>` module (this will produce a wheel): ::

  python -m build /path/to/source

A number of additional argument may be provided.

--enable-build-jar   Force setup to recreate the jar from scratch. 
--enable-tracing     Build a verison of JPype with full logging to the 
                     console. This can be used to diagnose tricky JNI
                     issues.

For example::

    python -m build /path/to/source -C--global-option=build_ext -C--global-option="--enable-tracing"

After building, JPype can be tested using the test bench. The test
bench requires JDK to build.

**3. Install the built wheel with:** ::

    pip install /path/to/wheel

**4. Test JPype with (optional):** ::

    python -m pytest



If it fails...
~~~~~~~~~~~~~~

Most failures happen when setup.py is unable to find the JDK home directory
which shouble be set in the enviroment variable ``JAVA_HOME``.  If this
happens, preform the following steps:

1. Identify the location of your systems JDK installation and explicitly passing
   it to setup.py. ::

     JAVA_HOME=/usr/lib/java/jdk1.8.0/ python -m build .

2. If that setup.py still fails please create an Issue `on
   github <https://github.com/jpype-project/jpype/issues?state=open>`__ and
   post the relevant logs.


.. _below:

Platform Specific requirements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

JPype is known to work on Linux, OSX, and Windows.  To make it easier to those
who have not built CPython modules before here are some helpful tips for
different machines.

Debian/Ubuntu
:::::::::::::

Debian/Ubuntu users will have to install ``g++`` and ``python-dev``.
Use:

    sudo apt-get install g++ python3-dev

Windows
:::::::

CPython modules must be built with the same C++ compiler used to build Python.
The tools listed below work for Python 3.5 to 3.8.  Check with `Python dev guide
<https://devguide.python.org/setup/>`_ for the latest instructions.

1. Install your desired version of Python (3.5 or higher), e.g., `Miniconda
   <https://docs.conda.io/en/latest/miniconda.html#windows-installers>`_ is a good choice for users not yet
   familiar with the language
2. For Python 3 series, Install either 2017 or 2019 Visual Studio.
   `Microsoft Visual Studio 2019 Community Edition
   <https://visualstudio.microsoft.com/downloads/>`_ is known to work.

From the Python developer page: 

   When installing Visual Studio 2019, select the Python development workload and
   the optional Python native development tools component to obtain all of the
   necessary build tools. If you do not already have git installed, you can find
   git for Windows on the Individual components tab of the installer.

When building for windows you must use the Visual Studio developer command
prompt.


Path requirements
-----------------

On certain systems such as Windows 2016 Server, the JDK will not load properly
despite JPype properly locating the JVM library.  The work around for this 
issue is add the JRE bin directory to the system PATH.  Apparently, the 
shared library requires dependencies which are located in the bin directory.
If a JPype fails to load despite having the correct JAVA_HOME and 
system architecture, it may be this issue.


Known Bugs/Limitations
----------------------

-  Java classes outside of a package (in the ``<default>``) cannot be
   imported.
-  Because of lack of JVM support, you cannot shutdown the JVM and then
   restart it.  Nor can you start more than one copy of the JVM.
-  Mixing 64 bit Python with 32 bit Java and vice versa crashes on import 
   of the jpype module.
