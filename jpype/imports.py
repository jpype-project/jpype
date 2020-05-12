# -*- coding: utf-8 -*-
# *****************************************************************************
#   Copyright 2017 Karl Einar Nelson
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# *****************************************************************************

"""
JPype Imports Module
--------------------

Once imported this module will place the standard Top Level Domains (TLD) into
the Python scope. These TLDs are ``java``, ``com``, ``org``, ``gov``, ``mil``,
``net`` and ``edu``. Java symbols from these domains can be imported using the
standard Python syntax.

Import customizers are supported in Python 3.6 or greater.

Forms supported:
   - **import <java_pkg> [ as <name> ]**
   - **import <java_pkg>.<java_class> [ as <name> ]**
   - **from <java_pkg> import <java_class>[,<java_class>*]**
   - **from <java_pkg> import <java_class> [ as <name> ]**
   - **from <java_pkg>.<java_class> import <java_static> [ as <name> ]**
   - **from <java_pkg>.<java_class> import <java_inner> [ as <name> ]**

For further information please read the :doc:`imports` guide.

Requires:
    Python 3.6 or later

Example:

.. code-block:: python

   import jpype
   import jpype.imports
   jpype.startJVM()

   # Import java packages as modules
   from java.lang import String

"""
import sys
import _jpype
from importlib.machinery import ModuleSpec as _ModuleSpec
from . import _pykeywords

__all__ = ["registerImportCustomizer", "registerDomain", "JImportCustomizer"]

# %% Utility


def _keywordUnwrap(name):
    if not name.endswith('_'):
        return name
    if name[:-1] in _pykeywords._KEYWORDS:
        return name[:-1]
    return name


def _keywordWrap(name):
    if name in _pykeywords._KEYWORDS:
        return name + "_"
    return name


_java_lang_Class = None
_java_lang_NoClassDefFoundError = None
_java_lang_ClassNotFoundException = None
_java_lang_UnsupportedClassVersionError = None
_java_lang_ExceptionInInitializerError = None


def _getJavaClass(javaname):
    """ This produces diagnostics on failing to find a Java class """
    global _java_lang_Class
    global _java_lang_NoClassDefFoundError
    global _java_lang_ClassNotFoundException
    global _java_lang_UnsupportedClassVersionError
    global _java_lang_ExceptionInInitializerError
    if not _java_lang_Class:
        _java_lang_Class = _jpype.JClass("java.lang.Class")
        _java_lang_ClassNotFoundException = _jpype.JClass(
            "java.lang.ClassNotFoundException")
        _java_lang_NoClassDefFoundError = _jpype.JClass(
            "java.lang.NoClassDefFoundError")
        _java_lang_UnsupportedClassVersionError = _jpype.JClass(
            "java.lang.UnsupportedClassVersionError")
        _java_lang_ExceptionInInitializerError = _jclass.JClass(
            "java.lang.ExceptionInInitializerError")

    err = None
    try:
        # Use forname because it give better diagnostics
        cls = _java_lang_Class.forName(javaname)
        return _jpype.JClass(cls)

    # Not found is acceptable
    except _java_lang_ClassNotFoundException:
        p = javaname.rpartition('.')
        err = "'%s' not found in '%s'" % (p[2], p[0])

    # Missing dependency
    except _java_lang_NoClassDefFoundError as ex:
        missing = str(ex).replace('/', '.')
        err = "Unable to import '%s' due to missing dependency '%s'" % (
            javaname, missing)
        ex1 = ex

    # Wrong Java version
    except _java_lang_UnsupportedClassVersionError as ex:
        err = "Unable to import '%s' due to incorrect Java version" % (
            javaname)
        ex1 = ex

    except _java_lang_ExceptionInInitializerError as ex:
        err = "Unable to import '%s' due to initializer error" % (
            javaname)
        ex1 = ex
        ex1._expandStacktrace()

    # Otherwise!?
    except Exception as ex:
        err = "Unable to import '%s' due to unexpected exception, '%s'" % (
            javaname, ex)
        ex1 = ex
    raise ImportError(err) from ex1


# %% Customizer
_CUSTOMIZERS = []


def registerImportCustomizer(customizer):
    """ Import customizers can be used to import python packages
    into java modules automatically.
    """
    _CUSTOMIZERS.append(customizer)

# Support hook for placing other things into the java tree


class JImportCustomizer(object):
    """ Base class for Import customizer.

    Import customizers should implement canCustomize and getSpec.

    Example:

    .. code-block:: python

       # Site packages for each java package are stored under $DEVEL/<java_pkg>/py
       class SiteCustomizer(jpype.imports.JImportCustomizer):
           def canCustomize(self, name):
               if name.startswith('org.mysite') and name.endswith('.py'):
                   return True
               return False
           def getSpec(self, name):
               pname = name[:-3]
               devel = os.environ.get('DEVEL')
               path = os.path.join(devel, pname,'py','__init__.py')
               return importlib.util.spec_from_file_location(name, path)
   """

    def canCustomize(self, name):
        """ Determine if this path is to be treated differently

        Return:
            True if an alternative spec is required.
        """
        return False

    def getSpec(self, name):
        """ Get the module spec for this module.
        """
        raise NotImplementedError


# %% Finder

def unwrap(name):
    # Deal with Python keywords in the Java path
    if not '_' in name:
        return name
    return ".".join([_keywordUnwrap(i) for i in name.split('.')])


class _JImportLoader:
    """ (internal) Finder hook for importlib. """

    def find_spec(self, name, path, target=None):
        # If jvm is not started then we just check against the TLDs
        if not _jpype.isStarted():
            base = name.partition('.')[0]
            if not base in _JDOMAINS:
                return None
            raise ImportError("Attempt to create java modules without jvm")

        # Check if it is a TLD
        parts = name.rpartition('.')
        if not parts[1] and _jpype.isPackage(parts[2]):
            return _ModuleSpec(name, self)

        if not parts[1] and not _jpype.isPackage(parts[0]):
            return None

        base = sys.modules.get(parts[0], None)
        if not base or not isinstance(base, _jpype._JPackage):
            return None

        # Support for external modules in java tree
        name = unwrap(name)
        for customizer in _CUSTOMIZERS:
            if customizer.canCustomize(name):
                return customizer.getSpec(name)

        # Using isPackage eliminates need for registering tlds
        if not hasattr(base, parts[2]):
            # If the base is a Java package and it wasn't found in the
            # package using getAttr, then we need to emit an error
            # so we produce a meaningful diagnositic.
            _getJavaClass(name)

        # Import the java module
        return _ModuleSpec(name, self)

    """ (internal) Loader hook for importlib. """

    def create_module(self, spec):
        if spec.parent == "":
            return _jpype._JPackage(spec.name)
        parts = spec.name.rsplit('.', 1)
        return getattr(sys.modules[spec.parent], parts[1])

    def exec_module(self, fullname):
        pass


# Install hooks into python importlib
sys.meta_path.append(_JImportLoader())

# %% Domains
_JDOMAINS = {}


def registerDomain(mod, alias=None):
    """ Add a Java domain to Python as a dynamic module.

    This can be used to bind a Java path to a Python path.

    Args:
        mod(str): Is the Python module to bind to Java.
        alias(str, optional): Is the name of the Java path if different
          than the Python name.
    """
    if not alias:
        alias = mod
    _JDOMAINS[mod] = alias


# Preregister common top level domains
registerDomain('com')
registerDomain('gov')
registerDomain('java')
registerDomain('org')
registerDomain('mil')
registerDomain('edu')
registerDomain('net')
