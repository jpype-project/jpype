JPype documentation
===================

JPype is a Python module to provide full access to Java from within Python. It
allows Python to make use of Java specific libraries, explore and visualize
Java structures, develop and test Java libraries, make use of scientific
computing, and much more.  By enabling the use of Python for rapid prototyping
and Java for strong typed production code, JPype provides a powerful
environment for engineering and code development.

Unlike Jython, JPype does not achieve this by re-implementing Python, but
instead by interfacing both virtual machines at the native level. This
shared memory based approach achieves good computing performance, while
providing the access to the entirety of CPython and Java libraries.

Parts of the documentation
==========================

.. toctree::
   :maxdepth: 2
   :caption: Getting started

   install
   intro

.. toctree::
   :maxdepth: 2
   :caption: Python calling Java

   quickguide_py
   types_py
   collections_py
   jvm_py
   customizers_py
   proxies_py
   threading_py
   tooling_py
   pickling_py
   numpy_py
   dbapi2_py
   debugging_py
   awt_py
   gui_py
   limitations_py

.. toctree::
   :maxdepth: 2
   :caption: Java calling Python (reverse bridge)

   quickguide_java
   types_java
   collections_java
   datetime_java
   jvm_java
   threading_java
   customizers_java
   tooling_java
   limitations_java

.. toctree::
   :maxdepth: 2
   :caption: Reference

   glossary
   api
   dbapi2
   imports
   spi
   android
   CHANGELOG
   develguide


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

