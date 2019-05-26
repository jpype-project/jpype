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

if _sys.version_info > (3,):
    _unicode = str
else:
    _unicode = unicode

__all__ = ['JImplementationFor']

# Forward declarations
_JObject = None
_JCLASSES = None


def registerClassBase(name, cls):
    """ (internal) Add an implementation for a class

    Use @JImplementationFor(cls, base=True) to access this.

    """
    if not issubclass(cls,  _JObject):
        raise TypeError("Classbases must derive from JObject")

    if name in _JP_BASES:
        _JP_BASES[name].append(cls)
    else:
        _JP_BASES[name] = [cls]

    # Changing the base class in python can break things,
    # so we will tag this as an error for now.
    if name in _JCLASSES:
        raise RuntimeError(
            "Base classes must be added before class is created")


def registerClassImplementation(classname, proto):
    """ (internal) Add an implementation for a class

    Use @JImplementationFor(cls) to access this.
    """
    if classname in _JP_IMPLEMENTATIONS:
        _JP_IMPLEMENTATIONS[classname].append(proto)
    else:
        _JP_IMPLEMENTATIONS[classname] = [proto]

    # If we have already created a class, apply it retroactively.
    if classname in _JCLASSES:
        _applyCustomizerPost(_JCLASSES[classname], proto)


def JImplementationFor(clsname, base=False):
    """ Decorator to define an implementation for a class.

    Applies to a class which will serve as a prototype as for the java class
    wrapper.  If it is registered as a base class, then the class must 
    derive from JObject.  Otherwise, the methods are copied from 
    the prototype to java class wrapper.

    The method ``__jclass_init__(cls)`` will be called with the constructed 
    class as the argument.  This call be used to set methods for all classes 
    that derive from the specified class.  Use ``type.__setattr__()`` to 
    alter the class methods.

    Using the prototype class as a base class is used mainly to support 
    classes which must be derived from a python type by design.  Use
    of a base class will produce a RuntimeError if the class has already
    been created.  

    For non-base class customizers, the customizer will be applied 
    retroactively if the class is already created.  Conflicts are
    resolved by the last customizer applied.

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


def _applyStickyMethods(cls, sticky):
    for method in sticky:
        attr = getattr(method, '__joverride__')
        rename = attr.get('rename', None)
        if rename:
            type.__setattr__(
                cls, rename, type.__getattribute__(cls, method.__name__))
        type.__setattr__(cls, method.__name__, method)


def _applyCustomizerImpl(members, proto, sticky, setter):
    """ (internal) Apply a customizer to a class.

    This "borrows" methods from a prototype class.
    Current behavior is:
     - Copy any string or property.
     - Copy any callable applying @JOverride
       if applicable with conflict renaming.
     - Copy __new__ method.

    """
    for p, v in proto.__dict__.items():
        if callable(v) or isinstance(v, (str, property, staticmethod, classmethod)):
            rename = "_"+p

            # Apply JOverride annotation
            attr = getattr(v, '__joverride__', None)
            if attr is not None:
                if attr.get('sticky', False):
                    sticky.append(v)
                rename = attr.get('rename', rename)

            # Apply rename
            if p in members and isinstance(members[p], (_jpype.PyJPField, _jpype.PyJPMethod)):
                setter(rename, members[p])

            setter(p, v)


def _applyAll(cls, method):
    applied = set()
    todo = [cls]
    while todo:
        c = todo.pop(0)
        if c in applied:
            continue
        todo.extend(c.__subclasses__())
        applied.add(c)
        method(c)


def _applyCustomizerPost(cls, proto):
    """ (internal) Customize a class after it has been created """
    sticky = []
    _applyCustomizerImpl(cls.__dict__, proto, sticky,
                         lambda p, v: type.__setattr__(cls, p, v))

    # Apply a customizer to all derived classes
    if '__jclass_init__' in proto.__dict__:
        method = proto.__dict__['__jclass_init__']
        _applyAll(cls, method)

    # Merge sticky into existing __jclass_init__
    if len(sticky) > 0:
        method = proto.__dict__.get('__jclass_init__', None)

        def init(cls):
            if method:
                method(cls)
            _applyStickyMethods(cls, sticky)
        type.__setattr__(cls, '__jclass_init__', init)
        _applyAll(cls, init)


def _applyCustomizers(name, jc, bases, members):
    """ (internal) Called by JClass and JArray to customize a newly created class."""
    # Apply base classes
    if name in _JP_BASES:
        for b in _JP_BASES[name]:
            bases.insert(0, b)

    # Apply implementations
    if name in _JP_IMPLEMENTATIONS:
        sticky = []
        for proto in _JP_IMPLEMENTATIONS[name]:
            _applyCustomizerImpl(members, proto, sticky,
                                 lambda p, v: members.__setitem__(p, v))

        if len(sticky) > 0:
            def init(cls):
                _applyStickyMethods(cls, sticky)
            members['__jclass_init__'] = init


def _applyInitializer(cls):
    """ (internal) Called after the class is created to apply any customizations
    required by inherited parents. 
    """
    if hasattr(cls, '__jclass_init__'):
        init = []
        for base in cls.__mro__:
            if '__jclass_init__' in base.__dict__:
                init.insert(0, base.__dict__['__jclass_init__'])
        for func in init:
            func(cls)
