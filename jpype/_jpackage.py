# *****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# *****************************************************************************

import re
import _jpype
from . import _jclass

__all__ = ['JPackage']


class JPackage(object):
    """ Gateway for automatic importation of Java classes.

    This class allows structured access to Java packages and classes.
    This functionality has been replaced by ``jpype.imports``, but is still
    useful in some cases.

    Only the root of the package tree need be declared with the ``JPackage``
    constructor. Sub-packages will be created on demand.

    For example, to import the w3c DOM package:

    .. code-block:: python

      Document = JPackage('org').w3c.dom.Document

    Under some situations such as a missing jar the resulting object
    will be a JPackage object rather than the expected java class. This
    results in rather challanging debugging messages. Thus the
    ``jpype.imports`` module is preferred. To prevent these types of errors
    a package can be declares as ``strict`` which prevents expanding
    package names that do not comply with Java package name conventions.

    Args:
      path (str): Path into the Java class tree.
      strict (bool, optional): Requires Java paths to conform to the Java
        package naming convention. If a path does not conform and a class 
        with the required name is not found, the AttributeError is raise 
        to indicate that the class was not found.

    Example:

      .. code-block:: python

        # Alias into a library
        google = JPackage("com.google")

        # Access members in the library
        result = google.common.IntMath.pow(x,m)

    """

    def __init__(self, name, strict=False, pattern=None):
        self.__name = name
        self.__pattern = pattern
        if strict:
            self.__pattern = re.compile('[_a-z][_a-z0-9]')

    def __getattribute__(self, n):
        try:
            return object.__getattribute__(self, n)
        except AttributeError as ex:
            ex1 = ex
            pass

        if n.startswith("__"):
            raise ex1
        # not found ...

        # perhaps it is a class?
        subname = "{0}.{1}".format(self.__name, n)
        if not _jpype.isStarted():
            if n.startswith('_'):
                raise ex1
            import warnings
            warnings.warn(
                "JVM not started yet, can not inspect JPackage contents %s")
            return n

        # See if it is a Java class
        try:
            cc = _jclass.JClass(subname)
            self.__setattr__(n, cc, True)
            return cc
        except:
            pass

        # Check to see if this conforms to the required package name
        # convention, it not then we should not create a new package
        if self.__pattern and self.__pattern.match(n) == None:
            raise AttributeError(
                "Java package %s does not contain a class %s" % (self.__name, n))

        # Add package to the path
        cc = JPackage(subname, pattern=self.__pattern)
        self.__setattr__(n, cc, True)
        return cc

    def __setattr__(self, n, v, intern=False):
        if not n[:len('_JPackage')] == '_JPackage' \
           and not intern:  # NOTE this shadows name mangling
            raise RuntimeError("Cannot set attributes in a package {0}"
                               .format(n))
        object.__setattr__(self, n, v)

    def __str__(self):
        return "<Java package {0}>".format(self.__name)

    def __call__(self, *arg, **kwarg):
        raise TypeError("Package {0} is not Callable".format(self._name))
