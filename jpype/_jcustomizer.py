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
import _jpype

__all__ = ['JImplementationFor', 'JConversion']

# Member types that are copied from the prototype
_jcopymembers = (str, property, staticmethod, classmethod)


def JConversion(cls, exact=None, instanceof=None, attribute=None, excludes=None):
    """ Decorator to define a method as a converted a Java type.

    Whenever a method resolution is called the JPype internal rules
    are applied, but this may be insufficient.  If only a
    single method requires modification then a class customizer can
    be applied.  But if many interfaces require the same conversion
    than a user conversion may be a better option.

    To add a user conversion define a method which take the requested
    Java type as the first argument, the target object to be converted
    as the second argument and returns a Java object or Java proxy that
    matches the required type.  If the type is not a Java type then
    a TypeError will be raised.  This method is only evaluated
    after the match has been determine prior to calling.

    Care should be used when defining a user conversion. If example
    if one has an interface that requires a specific class and you
    want it to take a Python string, then a user conversion can
    do that.  On the other hand, if you define a generic converter
    of any Python object to a Java string, then every interface
    will attempt to call the conversion method whenever a Java string
    is being matched, which can cause many methods to potentially
    become ambiguous.

    Conversion are not inherited. If the same converter needs to
    apply to multiple types, then multiple decorators can
    be applied to the same method.

    Args:
      cls(str, JClass): The class that will be produced by this
        conversion.
      exact(type): This conversion applies only to objects that have
        a type exactly equal to the argument.
      instanceof(type or protocol): This conversion applies to 
        any object that passes isinstance(obj, type).
      attribute(str): This conversion applies to any object that has
        passes hasattr(obj, arg). (deprecated)
      excludes(type): Prevents a conversion for a specified type.
        Can be used to prevent a specific type from being converted.
        For example, to prevent maps or strings from passing 
        a check for Sequence.  Exclusions are applied before all 
        other user specificied conversions.
    """
    hints = getClassHints(cls)
    if excludes is not None:
        hints._excludeConversion(excludes)

    def customizer(func=None):
        if exact is not None:
            hints._addTypeConversion(exact, func, True)
        if instanceof is not None:
            hints._addTypeConversion(instanceof, func, False)
        if attribute is not None:
            hints._addAttributeConversion(attribute, func)
        return func
    return customizer


def JImplementationFor(clsname, base=False):
    """ Decorator to define an implementation for a class.

    Applies to a class which will serve as a prototype as for the Java class
    wrapper.  If it is registered as a base class, then the class must
    derive from JObject.  Otherwise, the methods are copied from
    the prototype to the Java class wrapper.

    The method ``__jclass_init__(cls)`` will be called with the constructed
    class as the argument.  This call is used to set methods for all classes
    that derive from the specified class.  Use ``jclass._customize()`` to
    alter the class methods.

    Using the prototype class as a base class is used mainly to support
    classes which must be derived from a Python type by design.  Use
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
    if not isinstance(clsname, str):
        raise TypeError("ImplementationFor requires a java classname string")

    def customizer(cls):
        hints = getClassHints(clsname)
        if base:
            hints.registerClassBase(cls)
        else:
            hints.registerClassImplementation(clsname, cls)
        return cls
    return customizer


def _applyStickyMethods(cls, sticky):
    for method in sticky:
        attr = getattr(method, '__joverride__')
        rename = attr.get('rename', None)
        name = method.__name__
        if rename:
            orig = type.__getattribute__(cls, name)
            cls._customize(rename, orig)
        cls._customize(name, method)


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
        if callable(v) or isinstance(v, _jcopymembers):
            # Apply JOverride annotation
            attr = getattr(v, '__joverride__', None)
            if attr is not None:
                if attr.get('sticky', False):
                    sticky.append(v)
                    continue
                # Apply rename
                rename = attr.get('rename', "_" + p)
                if p in members and isinstance(members[p], (_jpype._JField, _jpype._JMethod)):
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
                         lambda p, v: cls._customize(p, v))

    # Merge sticky into existing __jclass_init__
    if len(sticky) > 0:
        method = proto.__dict__.get('__jclass_init__', None)

        def init(cls):
            if method:
                method(cls)
            _applyStickyMethods(cls, sticky)
        cls._customize('__jclass_init__', init)

    # Apply a customizer to all derived classes
    if '__jclass_init__' in proto.__dict__:
        method = proto.__dict__['__jclass_init__']
        _applyAll(cls, method)


class JClassHints(_jpype._JClassHints):
    """ ClassHints holds class customizers and conversions.

    These items can be defined before the JVM is created.
    """

    def __init__(self):
        self.bases = []
        self.implementations = []
        self.instantiated = False

    def registerClassBase(self, base):
        """ (internal) Add an implementation for a class

        Use @JImplementationFor(cls, base=True) to access this.

        """
        self.bases.append(base)

        # Changing the base class in python can break things,
        # so we will tag this as an error for now.
        if self.instantiated:
            raise TypeError(
                "Base classes must be added before class is created")

    def registerClassImplementation(self, classname, proto):
        """ (internal) Add an implementation for a class

        Use @JImplementationFor(cls) to access this.
        """
        self.implementations.append(proto)

        # If we have already created a class, apply it retroactively.
        if self.instantiated:
            _applyCustomizerPost(_jpype.JClass(classname), proto)

    def applyCustomizers(self, name, bases, members):
        """ (internal) Called by JClass and JArray to customize a newly created class."""
        # Apply base classes
        for b in self.bases:
            bases.insert(0, b)

        module = name.rsplit('.', 1)
        if len(module) == 2:
            members['__module__'] = module[0]

        # Apply implementations
        sticky = []
        for proto in self.implementations:
            _applyCustomizerImpl(members, proto, sticky,
                                 lambda p, v: members.__setitem__(p, v))

        if len(sticky) > 0:
            method = members.get('__jclass_init__', None)

            def init(cls):
                if method is not None:
                    method(cls)
                _applyStickyMethods(cls, sticky)
            members['__jclass_init__'] = init

    def applyInitializer(self, cls):
        """ (internal) Called after the class is created to apply any customizations
        required by inherited parents.
        """
        self.instantiated = True
        if hasattr(cls, '__jclass_init__'):
            init = []
            for base in cls.__mro__:
                if '__jclass_init__' in base.__dict__:
                    init.insert(0, base.__dict__['__jclass_init__'])
            for func in init:
                func(cls)


def getClassHints(name):
    if isinstance(name, _jpype._JClass):
        name = name.__name__
    hints = _jpype._hints.get(name, None)
    if not hints:
        hints = JClassHints()
        _jpype._hints[name] = hints
    return hints


_jpype._hints = {}
getClassHints("java.lang.IndexOutOfBoundsException").registerClassBase(
    IndexError)
getClassHints("java.lang.NullPointerException").registerClassBase(
    ValueError)
