Installation
============

JPype is available either as a pre-compiled binary for Anaconda, or 
may be build from source though several methods.


Binary Install
--------------

JPype can be installed as pre-compiled binary if you are using the `Anaconda
<https://anaconda.org>`_ Python stack. Binaries are available for Linux, OSX,
ad windows are available on conda-forge.

1. Ensure you have installed Anaconda/Miniconda. Instructions can be found
   `here <http://conda.pydata.org/docs/install/quick.html>`__.  
2. Install from
   the conda-forge software channel::

    conda install -c conda-forge jpype1


Source Install
--------------

Installing from source requires

Python
  JPype works with CPython 2.6 or later including the 3 series. Both the
  runtime and the development package are required.

Java
  Either the Sun/Oracle JDK/JRE Variant or OpenJDK. Python 2.6+ (including
  Python 3+).  JPype source distribution includes a copy of the Java JNI header
  and precompiled Java code, so the development kit is not required from the
  source distribution. JPype has been used with Java versions from 
  Java 1.6 to Java 11.

C++
  A C++ compiler which matches the ABI used to build CPython.

Ant and JDK
  *(Optional)* JPype contains sections of Java code. These sections are
  precompiled in the source distribution but must be build with installing from
  git directly.

Once these requirements have been met one can use pip to build either from the
source distribution or directly from the repo.  Specific requirements from
different achitectures are shown below_.   


Build using pip
~~~~~~~~~~~~~~~

JPype may be build and installed with one easy step using pip.

To install the latest JPype using the source distribute use: ::

  pip install JPype1

To install from the current github master use: ::

  pip install git+https://github.com/jpype-project/jpype.git

More details on installing from git can be found at `Pip install
<https://pip.pypa.io/en/stable/reference/pip_install/#git>`__.  The git version
does not include a prebuilt jar and thus both ant and JDK are required.


Build and install manually
~~~~~~~~~~~~~~~~~~~~~~~~~~

JPype can be built entirely from source. Just follow these three
easy steps.

**1. Get the JPype source**

Either from 
`github <https://github.com/jpype-project/jpype>`__ or
from `PyPi <http://pypi.python.org/pypi/JPype1>`__. 

**2. Build the source with desired options**

Compile JPype using the included ``setup.py`` script with: ::

  python setup.py build

The setup script recognizes several arguments.

--ant                Define the location of ant on your system using 
                     ``--ant=path``.  This option is useful when building 
                     when ant is not in the path.
--enable-build-jar   Force setup to recreate the jar from scratch. 
--enable-tracting    Build a verison of JPype with full logging to the 
                     console. This can be used to diagnose trick JNI
                     issues.
--disable-numpy      Do not compile with numpy extenstions.

After building, JPype can be tested using the test bench. The test
bench requires ant and JDK to build.

**3. Install JPype with:** ::

    python setup.py install


If it fails...
~~~~~~~~~~~~~~

This happens mostly due to the setup not being able to find your ``JAVA_HOME``.
In case this happens, please do two things:

1. You can continue the installation by finding the ``JAVA_HOME`` on your own (
   the place where the headers etc. are) and explicitly setting it for the
   installation: ::

     JAVA_HOME=/usr/lib/java/jdk1.8.0/ python setup.py install

2. Please create an Issue `on
   github <https://github.com/jpype-project/jpype/issues?state=open>`__ and
   post all the information you have.


.. _below:

Specific requirements
~~~~~~~~~~~~~~~~~~~~~

JPype is know to work on Linx, OSX, Windows, and Cygwin.  To make it easier to
those who have not build CPython modules before here are some helpful tips for
different machines.

Debian/Ubuntu
:::::::::::::

Debian/Ubuntu users will have to install ``g++``, ``python-dev``, and ``ant`` (optional)
Use:

::

    sudo apt-get install g++ python-dev python3-dev ant

Windows
:::::::

Windows users need a CPython installation and C++ compilers specificly for 
CPython:

1. Install some version of Python (2.7 or higher), e.g., `Anaconda
   <https://www.continuum.io/downloads>`_ is a good choice for users not yet
   familiar with the language
2. For Python 2 series, Install `Windows C++ Compiler
   <http://landinghub.visualstudio.com/visual-cpp-build-tools>`_
3. For Python 3 series, Install `Microsoft Visual Studio 2010 Redistributable Package (x64)
   <https://www.microsoft.com/en-us/download/details.aspx?id=14632>`_ and
   `Microsoft Build Tools 2015 Update 3
   <https://visualstudio.microsoft.com/vs/older-downloads/>`_
4. (optional) Install `Apache Ant (tested using 1.9.13)
   <https://ant.apache.org/bindownload.cgi>`_

Netbeans ant can be used in place of Apache Ant.  Netbeans ant is located in
``${netbeans}/extide/ant/bin/ant.bat``.  

Due to differences in the C++ API, only the version specified will work to
build CPython modules.  The Build Tools 2015 is a pain to find. Microsoft
really wants people to download the latest version.  To get to it from the
above URL, click on "Redistributables and Build Tools", then select Microsoft
Build Tools 2015 Update 3.

When building for windows you must use the Visual Studio developer command
prompt.


Known Bugs/Limitations
----------------------

-  Java classes outside of a package (in the ``<default>``) cannot be
   imported.
-  Because of lack of JVM support, you cannot shutdown the JVM and then
   restart it.
-  Structural issues prevent managing objects from more than one JVM
   at a time.
-  Some methods rely on the "current" class/caller. Since calls coming
   directly from python code do not have a current class, these methods
   do not work. The :doc:`userguide` lists all the known methods like that.
-  Mixing 64 bit Python with 32 bit Java and vice versa crashes on import jpype.
