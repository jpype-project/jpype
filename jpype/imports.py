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

# Optional jpype module to support:
#   import <java_pkg> [ as <name> ]
#   import <java_pkg>.<java_class> [ as <name> ]
#   from <java_pkg> import <java_class>[,<java_class>*]
#   from <java_pkg> import <java_class> [ as <name> ]
#   from <java_pkg>.<java_class> import <java_static> [ as <name> ]
#   from <java_pkg>.<java_class> import <java_inner> [ as <name> ]
#
#   jpype.imports.registerDomain(moduleName, alias=<java_pkg>)
#   jpype.imports.registerImportCustomizer(JImportCustomizer)
# 
# Requires Python 3.6 or later
# Usage:
#   import jpype
#   import jpype.imports
#   <start or attach jvm>
#   # Import java packages as modules
#   from java.lang import String

try:
    from importlib.machinery import ModuleSpec as _ModuleSpec
    from types import ModuleType as _ModuleType
except Exception:
    raise ImportError("jpype.imports Not supported for Python 2")
import sys as _sys
import keyword as _keyword
from ._jclass import JClass as _JClass
from ._jclass import _JavaClass as _JavaClass
from ._core import registerJVMInitializer as _jinit

__all__ = ["registerImportCustomizer", "registerDomain", "JImportCustomizer"]
_initialized = False
_exportTypes = ()
_modifier = None

# %% Utility
def _keywordUnwrap(name):
    if not name.endswith('_'):
        return name
    if _keyword.iskeyword(name[:-1]):
        return name[:-1]
    return name

def _keywordWrap(name):
    if name in _keyword.kwlist:
        return name + "_"
    return name

def _getJavaClass(javaname):
    try:
        return _JClass(javaname)
    except Exception:
        return None

def _copyProperties(out, mc):
    for v in dir(mc):
        # Skip private members
        if v.startswith('_'):
            continue

        # Copy properties
        attr = getattr(mc, v)
        if isinstance(attr, property):
            out[v] = attr

def _getStaticMethods(cls):
    global _modifier
    static = {}
    for u in cls.__javaclass__.getMethods():
        if not _modifier.isStatic(u.getModifiers()):
            continue
        name = _keywordWrap(u.getName())
        static[name] = getattr(cls, name)
    return static

def _copyStaticMethods(out, cls):
    for u, v in _getStaticMethods(cls).items():
        out[u] = v

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
      | # Site packages for each java package are stored under $DEVEL/<java_pkg>/py
      | class SiteCustomizer(jpype.imports.JImportCustomizer):
      |     def canCustomize(self, name):
      |         if name.startswith('org.mysite') and name.endswith('.py'):
      |             return True
      |         return False
      |     def getSpec(self, name):
      |         pname = name[:-3]
      |         devel = os.environ.get('DEVEL')
      |         path = os.path.join(devel, pname,'py','__init__.py')
      |         return importlib.util.spec_from_file_location(name, path)
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
        raise Exception

# %% Import
class _JImport(object):
    """ (internal) Base class for import java modules """
    # Module requirements
    __doc__ = None
    __loader__ = None
    __path__ = "<java>"
    __package__ = "java"

    def __init__(self, name):
        pass

    def __getattr__(self, name):
        if name.startswith('_'):
            return object.__getattribute__(self, name)

        name = _keywordUnwrap(name)

        # Inner class support
        jname = object.__getattribute__(self, '__javaname__')
        try:
            object.__getattribute__(self, '__javaclass__')
            jname = "$".join([jname, name])
        except AttributeError:
            jname = ".".join([jname, name])

        # Get the class (if it exists)
        jtype = _getJavaClass(jname)
        if jtype:
            # Cache it for later
            object.__setattr__(self, name, jtype)
            return jtype

        # If the java class does not exist, throw a ClassNotFound exception
        raise Exception("Unable to find java class " + jname)

    def __setattr__(self, name, value):
        if name.startswith('__'):
            raise AttributeError("Module does not allow setting of %s" % name)
        if hasattr(value, '__javaclass__'):
            return object.__setattr__(self, name, getattr(value, '__javaclass__'))
        if isinstance(value, (_JImport, _ModuleType)):
            return object.__setattr__(self, name, value)
        raise AttributeError("JImport may not set attribute %s" % name)


# In order to get properties to be attached to the _JImport class,
# we must create a dynamic class between
def _JImportFactory(spec, javaname, cls=_JImport):
    """ (internal) Factory for creating java modules dynamically.

    This is needed to create a new type node to hold static methods.
    """
    def init(self, name):
        # Call the base class
        cls.__init__(self, name)

    def getall(self):
        global _exportTypes
        d1 = self.__dict__.items()
        d2 = self.__class__.__dict__.items()
        local = [name for name, attr in d1 if not name.startswith('_')
                and isinstance(attr, _exportTypes)]
        glob = [name for name, attr in d2 if not name.startswith('_')
                and isinstance(attr, _exportTypes)]
        local.extend(glob)
        return local

    # Set up a new class for this type
    bases = [cls]
    members = {
        "__init__": init,
        "__javaname__": javaname,
        "__name__": spec.name,
        "__all__": property(getall),
        "__spec__": spec,
    }

    # Is this module also a class, if so insert class info
    jclass = _getJavaClass(javaname)
    if jclass:
        # Mark this as a class (will cause children to be inner classes)
        members['__javaclass__'] = jclass

        # Exposed static members as part of the module
        _copyProperties(members, jclass.__metaclass__)
        _copyStaticMethods(members, jclass)

    return type("module." + spec.name, tuple(bases), members)

def _JModule(spec, javaname):
    """ (internal) Front end for creating a java module dynamically """
    cls = _JImportFactory(spec, javaname)
    out = cls(spec.name)
    return out

# %% Finder
class _JImportLoader:
    """ (internal) Finder hook for importlib. """
    def find_spec(self, name, path, target):
        parts = name.split('.', 1)
        if not parts[0] in _JDOMAINS:
            return None

        # Support for external modules in java tree
        for customizer in _CUSTOMIZERS:
            if customizer.canCustomize(name):
                return customizer.getSpec(name)

        # Import the java module
        return _ModuleSpec(name, self)

    """ (internal) Loader hook for importlib. """
    def create_module(self, spec):
        global _initialized
        if not _initialized:
            raise ImportError("Attempt to create java modules without jvm")

        # Handle creating the java name based on the path
        parts = spec.name.split('.')
        if len(parts) == 1:
            return _JModule(spec, _JDOMAINS[spec.name])

        # Use the parent module to simplify name mangling
        base = _sys.modules[".".join(parts[:-1])]

        # Support of inner classes
        if not isinstance(base,_JImport):
            return getattr(base, parts[-1])
        jbasename = object.__getattribute__(base, '__javaname__')
        try:
            object.__getattribute(base, '__javaclass__')
            javaname = "$".join([jbasename, _keywordUnwrap(parts[-1])])
        except AttributeError:
            javaname = ".".join([jbasename, _keywordUnwrap(parts[-1])])

        return _JModule(spec, javaname)

    def exec_module(self, fullname):
        pass

# Install hooks into python importlib
_sys.meta_path.append(_JImportLoader())

# %% Domains
_JDOMAINS = {}

def registerDomain(mod, alias=None):
    """ Add a java domain to python as a dynamic module.

    Args:
        mod is the name of the dynamic module
        alias is the name of the java path. (optional)
    """
    if not alias:
        alias = mod
    _JDOMAINS[mod] = alias

# Preregister common top level domains
registerDomain('com')
registerDomain('gov')
registerDomain('java')
registerDomain('org')

# %% Initialize
def _initialize():
    global _exportTypes
    global _initialized 
    global _modifier
    _initialized = True
    _JMethod = type(_JClass('java.lang.Class').forName)
    _modifier = _JClass('java.lang.reflect.Modifier')
    _exportTypes = (property, _JavaClass, _JImport, _JMethod)

_jinit(_initialize)

