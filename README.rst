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
:Discussions: `GitHub Discussions
 <https://github.com/jpype-project/jpype/discussions>`_
:Documentation: `Python Docs`_
:License: `Apache 2 License`_
:Build status:  |TestsCI|_ |Docs|_
:Quality status:  |Codecov|_ |lgtm_python|_ |lgtm_java|_ |lgtm_cpp|_
:Version: |PypiVersion|_ |Conda|_

The work on this project began on `Sourceforge <http://sourceforge.net/projects/jpype/>`__.
LLNL-CODE- 812311


.. |alerts| image:: https://img.shields.io/lgtm/alerts/g/jpype-project/jpype.svg?logo=lgtm&logoWidth=18
.. _alerts: https://lgtm.com/projects/g/jpype-project/jpype/alerts/
.. |lgtm_python| image:: https://img.shields.io/lgtm/grade/python/g/jpype-project/jpype.svg?logo=lgtm&logoWidth=18&label=python
.. _lgtm_python: https://lgtm.com/projects/g/jpype-project/jpype/context:python
.. |lgtm_java| image:: https://img.shields.io/lgtm/grade/java/g/jpype-project/jpype.svg?logo=lgtm&logoWidth=18&label=java
.. _lgtm_java: https://lgtm.com/projects/g/jpype-project/jpype/context:java
.. |lgtm_cpp| image:: https://img.shields.io/lgtm/grade/cpp/g/jpype-project/jpype.svg?logo=lgtm&logoWidth=18&label=C++
.. _lgtm_cpp: https://lgtm.com/projects/g/jpype-project/jpype/context:cpp
.. |PypiVersion| image:: https://img.shields.io/pypi/v/Jpype1.svg
.. _PypiVersion: https://badge.fury.io/py/JPype1
.. |Conda| image:: https://img.shields.io/conda/v/conda-forge/jpype1.svg
.. _Conda: https://anaconda.org/conda-forge/jpype1
.. |TestsCI| image:: https://dev.azure.com/jpype-project/jpype/_apis/build/status/jpype-project.jpype?branchName=master
.. _TestsCI: https://dev.azure.com/jpype-project/jpype/_build/latest?definitionId=1&branchName=master
.. |Docs| image:: https://img.shields.io/readthedocs/jpype.svg
.. _Docs: http://jpype.readthedocs.org/en/latest/
.. |Codecov| image:: https://codecov.io/gh/jpype-project/jpype/branch/master/graph/badge.svg
.. _Codecov: https://codecov.io/gh/jpype-project/jpype
.. |implementation| image:: https://img.shields.io/pypi/implementation/jpype1.svg
.. |pyversions| image:: https://img.shields.io/pypi/pyversions/jpype1.svg
.. |javaversions| image:: https://img.shields.io/badge/java-8%20%7C%209%20%7C%2011-purple.svg
.. |jvm| image:: https://img.shields.io/badge/jvm-Open%20%7C%20Oracle%20%7C%20Corretto-purple.svg
.. |platform| image:: https://img.shields.io/conda/pn/conda-forge/jpype1.svg
.. |license| image:: https://img.shields.io/github/license/jpype-project/jpype.svg
.. _Apache 2 License: https://github.com/jpype-project/jpype/blob/master/LICENSE
.. _Python Docs: http://jpype.readthedocs.org/en/latest/

SPDX-License-Identifier: Apache-2.0
