.. image:: doc/logo.png
   :scale: 50 %
   :alt: JPype logo
   :align: center

JPype
=====
   
|implementation|  |pyversions|  |javaversions|  |jvm|  |platform|  |license|

JPype is a Python module to provide full access to Java from 
within Python. It allows Python to make use of Java only libraries,
exploring and visualization of Java structures, development and testing
of Java libraries, scientific computing, and much more.  By gaining 
the best of both worlds using Python for rapid prototyping and Java
for strong typed production code, JPype provides a powerful environment
for engineering and code development.  

This is achieved not through re-implementing Python, as
Jython has done, but rather through interfacing at the native
level in both virtual machines. This shared memory based 
approach achieves decent computing performance, while providing the
access to the entirety of CPython and Java libraries.

:Code: `GitHub
 <https://github.com/jpype-project/jpype>`_
:Issue tracker: `GitHub Issues
 <https://github.com/jpype-project/jpype/issues>`_
:Documentation: `Python Docs`_
:License: `Apache 2 License`_
:Build status:  |travisCI|_ |appveyorCI|_ |Docs|_
:Test status:  |testsCI|_ 
:Version: |pypiVersion|_ |conda|_
     
    .. |travisCI| image:: https://img.shields.io/travis/jpype-project/jpype.svg?label=linux
    .. _travisCI: https://travis-ci.org/jpype-project/jpype
    
    .. |appveyorCI| image:: https://img.shields.io/appveyor/ci/jpype-project/jpype.svg?label=windows
    .. _appveyorCI: https://ci.appveyor.com/project/jpype-project/jpype
    
    .. |testsCI| image:: https://img.shields.io/appveyor/tests/jpype-project/jpype.svg
    .. _testsCI: https://ci.appveyor.com/project/jpype-project/jpype
    
    .. |pypiVersion| image:: https://img.shields.io/pypi/v/Jpype1.svg
    .. _pypiVersion: https://badge.fury.io/py/JPype1
    
    .. |conda| image:: https://img.shields.io/conda/v/conda-forge/jpype1.svg
    .. _conda: https://anaconda.org/conda-forge/jpype1

    .. |Docs| image:: https://img.shields.io/readthedocs/jpype.svg
    .. _Docs: http://jpype.readthedocs.org/en/latest/

   
.. |implementation| image:: https://img.shields.io/pypi/implementation/jpype1.svg
.. |pyversions| image:: https://img.shields.io/pypi/pyversions/jpype1.svg
.. |javaversions| image:: https://img.shields.io/badge/java-8%20%7C%209%20%7C%2011-purple.svg
.. |jvm| image:: https://img.shields.io/badge/jvm-Open%20%7C%20Oracle%20%7C%20Corretto-purple.svg
.. |platform| image:: https://img.shields.io/conda/pn/conda-forge/jpype1.svg
.. |license| image:: https://img.shields.io/github/license/jpype-project/jpype.svg
.. _Apache 2 License: https://github.com/jpype-project/jpype/blob/master/LICENSE
.. _Python Docs: http://jpype.readthedocs.org/en/latest/

The work on this project began on `Sourceforge <http://sourceforge.net/projects/jpype/>`__.
