# *****************************************************************************
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
#   See NOTICE file for details.
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


# %% Customizer
_CUSTOMIZERS = []


def _JExceptionHandler(pkg, name, ex):
    javaname = str(pkg) + "." + name
    exname = type(ex).__name__
    ex._expandStacktrace()
    if exname == "java.lang.ExceptionInInitializerError":
        raise ImportError("Unable to import '%s' due to initializer error" % javaname) from ex
    if exname == "java.lang.UnsupportedClassVersionError":
        raise ImportError("Unable to import '%s' due to incorrect Java version" % javaname) from ex
    if exname == "java.lang.NoClassDefFoundError":
        missing = str(ex).replace('/', '.')
        raise ImportError("Unable to import '%s' due to missing dependency '%s'" % (
            javaname, missing)) from ex
    raise ImportError("Unable to import '%s'" % javaname) from ex


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
            raise ImportError("Attempt to create Java package '%s' without jvm" % name)

        # Check for aliases
        if name in _JDOMAINS:
            jname = _JDOMAINS[name]
            if not _jpype.isPackage(jname):
                raise ImportError("Java package '%s' not found, requested by alias '%s'" % (jname, name))
            ms = _ModuleSpec(name, self)
            ms._jname = jname
            return ms

        # Check if it is a TLD
        parts = name.rpartition('.')

        # Use the parent module to simplify name mangling
        if not parts[1] and _jpype.isPackage(parts[2]):
            ms = _ModuleSpec(name, self)
            ms._jname = name
            return ms

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
            try:
                # Use forname because it give better diagnostics
                cls = _jpype._java_lang_Class.forName(name, True, _jpype.JPypeClassLoader)

                # This code only is hit if an error was not thrown
                if cls.getModifiers() & 1 == 0:
                    raise ImportError("Class `%s` is not public" % name)
                raise ImportError("Class `%s` was found but was not expected" % name)
            # Not found is acceptable
            except Exception as ex:
                raise ImportError("Failed to import '%s'" % name) from ex

        # Import the java module
        return _ModuleSpec(name, self)

    """ (internal) Loader hook for importlib. """

    def create_module(self, spec):
        if spec.parent == "":
            return _jpype._JPackage(spec._jname)
        parts = spec.name.rsplit('.', 1)
        rc = getattr(sys.modules[spec.parent], parts[1])

        # Install the handler
        rc._handler = _JExceptionHandler
        return rc

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
