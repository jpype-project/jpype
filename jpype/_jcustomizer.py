#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# *****************************************************************************
import _jpype
import sys as _sys

_JP_BASES = {}
_JP_IMPLEMENTATIONS = {}
_JP_CUSTOMIZERS = []

if _sys.version_info > (3,):
    _unicode = str
else:
    _unicode = unicode

__all__ = ['JImplementationFor']

_JObject = None


def registerClassBase(name, cls):
    """ (internal) Add an implementation for a class

    Use @JImplementationFor(cls, True) to access this.
    """
    if not issubclass(cls,  _JObject):
        raise TypeError("Classbases must derive from JObject")

    if name in _JP_BASES:
        _JP_BASES[name].append(cls)
    else:
        _JP_BASES[name] = [cls]


def registerClassImplementation(classname, cls):
    """ (internal) Add an implementation for a class

    Use @JImplementationFor(cls) to access this.
    """
    if classname in _JP_IMPLEMENTATIONS:
        _JP_IMPLEMENTATIONS[classname].append(cls)
    else:
        _JP_IMPLEMENTATIONS[classname] = [cls]


def registerClassCustomizer(c):
    """ (internal) Add a customizer for a java class wrapper.

    Deprecated customizer implementation.
    """
    _JP_CUSTOMIZERS.append(c)


def JImplementationFor(clsname, base=False):
    """ Decorator to define an implementation for a class.

    Applies to a class which will serve as a prototype as for the java class
    wrapper.  If it is registered as a base class, then the class must 
    derive from JObject.  Otherwise, the methods are copied from 
    the prototype to java class wrapper.

    The method __jclass_init__(cls) will be called with the constructed 
    class as the argument.  This call be used to set methods for all classes 
    that derive from the specified class.  Use type.__setattr__() to 
    alter the class methods.

    Args:
      clsname (str): name of java class.
      base (bool, optional): if True this will be a base class. 
        Default is False.

    """
    if not isinstance(clsname, (str, _unicode)):
        raise TypeError("SuperFor requires a java classname string")

    def customizer(cls):
        if base:
            registerClassBase(clsname, cls)
        else:
            registerClassImplementation(clsname, cls)
        return cls
    return customizer


# FIXME customizers currently only apply to classes after the customizer is
# loaded. this creates issues for bootstrapping especially with classes like
# basic arrays which may be overloaded to support operations like __add__.


# FIXME remove this once all customizers are converted
class JClassCustomizer(object):
    def canCustomize(self, name, jc):
        """ Determine if this class can be customized by this customizer.

        Classes should be customized on the basis of an exact match to 
        the class name or if the java class is derived from some base class.

        Args:
          name (str): is the name of the java class being created.
          jc (_jpype.PyJPClass): is the java class wrapper. 

        Returns:
          bool: true if customize should be called, false otherwise.
        """
        pass

    def customize(self, name, jc, bases, members):
        """ Customize the class.

        Should be able to handle keyword arguments to support changes in customizers.

        Args:
          name (str): is the name of the java class being created.

        """
        pass


def _applyStickyMethods(cls, sticky):
    for method in sticky:
        attr = getattr(method, '__joverride__')
        rename = attr.get('rename', None)
        if rename:
            type.__setattr__(
                cls, rename, type.__getattribute__(cls, method.__name__))
        type.__setattr__(cls, method.__name__, method)


def _applyCustomizers(name, jc, bases, members):
    """ Called by JClass and JArray to customize a newly created class."""
    # Apply base classes
    if name in _JP_BASES:
        for b in _JP_BASES[name]:
            bases.insert(0, b)

    # Apply implementations
    if name in _JP_IMPLEMENTATIONS:
        sticky = []
        for proto in _JP_IMPLEMENTATIONS[name]:
            for p, v in proto.__dict__.items():
                if isinstance(v, (str, property)):
                    members[p] = v
                elif callable(v):
                    rename = "_"+p

                    # Apply JOverride annotation
                    attr = getattr(v, '__joverride__', None)
                    if attr is not None:
                        if attr.get('sticky', False):
                            sticky.append(v)
                        rename = attr.get('rename', rename)

                    # Apply rename
                    if p in members:
                        members[rename] = members[p]

                    members[p] = v
                elif p == "__new__":
                    members[p] = v

        if len(sticky) > 0:
            def init(cls):
                _applyStickyMethods(cls, sticky)
            members['__jclass_init__'] = init

    # Apply customizers
    for i in _JP_CUSTOMIZERS:
        if i.canCustomize(name, jc):
            i.customize(name, jc, bases, members)


def _applyInitializer(cls):
    if hasattr(cls, '__jclass_init__'):
        init = []
        for base in cls.__mro__:
            if '__jclass_init__' in base.__dict__:
                init.insert(0, base.__dict__['__jclass_init__'])
        for func in init:
            func(cls)
