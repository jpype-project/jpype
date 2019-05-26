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
import sys as _sys
import _jpype
from ._pykeywords import pysafe
from . import _jcustomizer

__all__ = ['JClass', 'JInterface', 'JOverride']

if _sys.version > '3':
    _unicode = str
    _long = int
else:
    _unicode = unicode
    _long = long

_JObject = None

_java_lang_throwable = None
_java_lang_RuntimeException = None
_java_ClassLoader = None
_java_lang_Class = None
_java_lang_Object = None

_JCLASSES = {}
_JP_TYPE_CLASSES = {}
_JP_OBJECT_CLASSES = {}

_jcustomizer._JCLASSES = _JCLASSES


def _initialize():
    global _java_ClassLoader

    _jpype.setResource('GetClassMethod', _JClassNew)

    # Due to bootstrapping, Object and Class must be defined first.
    global _java_lang_Object, _java_lang_Class
    _java_lang_Object = JClass("java.lang.Object")
    _java_lang_Class = JClass("java.lang.Class")

    _java_ClassLoader = JClass('java.lang.ClassLoader').getSystemClassLoader()

    global _java_lang_throwable, _java_lang_RuntimeException
    _java_lang_throwable = JClass("java.lang.Throwable")
    _java_lang_RuntimeException = JClass("java.lang.RuntimeException")

    # Preload needed classes
    java_lang_Boolean = JClass("java.lang.Boolean")
    java_lang_Long = JClass("java.lang.Long")
    java_lang_Double = JClass("java.lang.Double")
    java_lang_String = JClass("java.lang.String")

    global _JP_OBJECT_CLASSES
    _JP_OBJECT_CLASSES[bool] = java_lang_Boolean
    _JP_OBJECT_CLASSES[int] = java_lang_Long
    _JP_OBJECT_CLASSES[_long] = java_lang_Long
    _JP_OBJECT_CLASSES[float] = java_lang_Double
    _JP_OBJECT_CLASSES[str] = java_lang_String
    _JP_OBJECT_CLASSES[_unicode] = java_lang_String
    _JP_OBJECT_CLASSES[type] = _java_lang_Class
    _JP_OBJECT_CLASSES[_jpype.PyJPClass] = _java_lang_Class
    _JP_OBJECT_CLASSES[object] = _java_lang_Object


def JOverride(*args, **kwargs):
    """ Annotation to denote a method as overriding a Java method.

    This annotation applies to customizers, proxies, and extension
    to Java class. Apply it to methods to mark them as implementing
    or overriding Java methods.  Keyword arguments are passed to the 
    corresponding implementation factory.

    Args:
      sticky=bool: Applies a customizer method to all derived classes.

    """
    # Check if called bare
    if len(args) == 1 and callable(args[0]):
        object.__setattr__(args[0], "__joverride__", {})
        return args[0]
     # Otherwise apply arguments as needed

    def modifier(method):
        object.__setattr__(method, "__joverride__", kwargs)
        return method
    return modifier


class JClass(type):
    """ Meta class for all java class instances.

    JClass when called as an object will contruct a new java Class wrapper. 

    All python wrappers for java classes derived from this type.
    To test if a python class is a java wrapper use
    ``isinstance(obj, jpype.JClass)``.

    Args:
      className (str): name of a java type.

    Keyword Args:
      loader (java.lang.ClassLoader): specifies a class loader to use
        when creating a class.
      initialize (bool): Passed to class loader when loading a class
        using the class loader.

    Returns:
      JavaClass: a new wrapper for a Java class

    Raises:
      TypeError: if the component class is invalid or could not be found.
    """
    @property
    def class_(self):
        return _JObject(self.__javaclass__)

    def __new__(cls, *args, **kwargs):
        if len(args) == 1:
            return _JClassNew(args[0], **kwargs)
        return super(JClass, cls).__new__(cls, *args, **kwargs)

    def __init__(self, *args, **kwargs):
        if len(args) == 1:
            return
        super(JClass, self.__class__).__init__(self, *args)

    def __getattribute__(self, name):
        if name.startswith('_'):
            return type.__getattribute__(self, name)
        attr = type.__getattribute__(self, name)
        if isinstance(attr, _jpype.PyJPMethod):
            return attr
        if isinstance(attr, property):
            raise AttributeError("Field is not static")
        return attr

    def __setattr__(self, name, value):
        if name.startswith('_'):
            return type.__setattr__(self, name, value)

        if not hasattr(self, name):
            raise AttributeError("Static field '%s' not found on Java '%s' class" %
                                 (name, self.__name__))

        try:
            attr = typeLookup(self, name)
            if hasattr(attr, '__set__'):
                return attr.__set__(self, value)
        except AttributeError:
            pass
        raise AttributeError("Static field '%s' is not settable on Java '%s' class" %
                             (name, self.__name__))

    def mro(cls):
        # Bases is ordered by (user, extend, interfaces)
        # Thus we maintain order of parents whereever possible
        # Breadth first inclusion of the parents
        parents = list(cls.__bases__)
        out = [cls]

        # Until we run out of parents
        while parents:

            # Test if the first is a leaf of the tree
            front = parents.pop(0)
            for p in parents:
                if issubclass(p, front):
                    parents.append(front)
                    front = None
                    break

            if not front:
                # It is not a leaf, we select another candidate
                continue

            # It is a leaf, so add it to the parents
            out.append(front)

            # Place the immediate parents of newest in the head of the list
            prev = parents
            parents = list(front.__bases__)

            # Include the remaining that we still need to consider
            parents.extend([b for b in prev if not b in parents])

        # JavaObjects are not interfaces, so we need to remove the JavaInterface inheritance
        if _JObject in out and JInterface in out:
            out.remove(JInterface)

        return out


def _JClassNew(arg, loader=None, initialize=True):
    if loader and isinstance(arg, str):
        arg = _java_lang_Class.forName(arg, initialize, loader)

    if isinstance(arg, _jpype.PyJPClass):
        javaClass = arg
    else:
        javaClass = _jpype.PyJPClass(arg)

    if javaClass is None:
        raise _java_lang_RuntimeException("Java class '%s' not found" % name)

    # Lookup the class name
    name = javaClass.getCanonicalName()

    # See if we have an existing class in the cache
    if name in _JCLASSES:
        return _JCLASSES[name]
    return _JClassFactory(name, javaClass)


class JInterface(object):
    """ Base class for all Java Interfaces. 

    ``JInterface`` is serves as the base class for any java class that is 
    a pure interface without implementation. It is not possible to create 
    a instance of a java interface. The ``mro`` is hacked such that
    ``JInterface`` does not appear in the tree of objects implement an 
    interface.

    Example:

    .. code-block:: python

       if issubclass(java.util.function.Function, jpype.JInterface):
          print("is interface")


        Use ``isinstance(obj, jpype.JavaInterface)`` to test for a interface.
    """
    @property
    def class_(self):
        return _JObject(self.__javaclass__)

    def __new__(cls, *args, **kwargs):
        return super(JInterface, cls).__new__(cls)

    def __init__(self, *args, **kwargs):
        if len(args) == 1 and isinstance(args[0], _jpype.PyJPValue):
            object.__setattr__(self, '__javavalue__', args[0])
        elif not hasattr(self, '__javavalue__'):
            raise JClass("java.lang.InstantiationException")(
                "`%s` is an interface." % str(self.class_.getName()))
        super(JObject, self).__init__()

    def __str__(self):
        return self.toString()

    def __hash__(self):
        return self.hashCode()

    def __eq__(self):
        return self.equals(o)

    def __ne__(self):
        return not self.equals(o)


def _JClassFactory(name, jc):
    from . import _jarray

    # Set up bases
    bases = []
    bjc = jc.getSuperClass()
    if name == 'java.lang.Object':
        bases.append(_JObject)
    elif jc.isArray():
        bases.append(_jarray.JArray)
    elif jc.isPrimitive():
        bases.append(object)
    elif bjc is not None:
        bases.append(JClass(bjc))
    elif bjc is None:
        bases.append(JInterface)
    itf = jc.getInterfaces()
    for ic in itf:
        bases.append(JClass(ic))

    # Set up members
    members = {
        "__javaclass__": jc,
        "__name__": name,
    }
    fields = jc.getClassFields()
    for i in fields:
        fname = pysafe(i.getName())
        members[fname] = i
    for jm in jc.getClassMethods():
        members[pysafe(jm.getName())] = jm

    # Apply customizers
    _jcustomizer._applyCustomizers(name, jc, bases, members)
    res = JClass(name, tuple(bases), members)
    _JCLASSES[name] = res

    # Post customizers
    _jcustomizer._applyInitializer(res)

    # Attach public inner classes we find
    #   Due to bootstrapping, we must wait until java.lang.Class is defined
    #   before we can access the class structures.
    if _java_lang_Class:
        for cls in res.class_.getDeclaredClasses():
            if cls.getModifiers() & 1 == 0:
                continue
            cls2 = JClass(cls)
            type.__setattr__(res, str(cls.getSimpleName()), cls2)

    return res

# **********************************************************


def _toJavaClass(tp):
    """ (internal) Converts a class type in python into a internal java class.

    Used mainly to support JArray.

    The type argument will operate on:
     - (str) lookup by class name or fail if not found.
     - (JClass) just returns the java type.
     - (type) uses a lookup table to find the class.
    """
    # if it a string
    if isinstance(tp, (str, _unicode)):
        return JClass(tp).__javaclass__

    if isinstance(tp, _jpype.PyJPClass):
        return tp

    if not isinstance(tp, type):
        raise TypeError(
            "Argument must be a class, java class, or string representing a java class")

    # See if it a class type
    try:
        return tp.__javaclass__
    except AttributeError:
        pass

    try:
        return _JP_TYPE_CLASSES[tp].__javaclass__
    except KeyError:
        pass

    raise TypeError("Unable to find class for '%s'" % tp.__name__)


def _getDefaultJavaObject(obj):
    global _JP_OBJECT_CLASSES
    try:
        return _JP_OBJECT_CLASSES[type(obj)]
    except KeyError:
        pass

    try:
        return obj._java_boxed_class
    except AttributeError:
        pass

    if isinstance(obj, _jpype.PyJPClass):
        return _java_lang_Class

    # We need to check this first as values have both
    # __javavalue__ and __javaclass__
    if hasattr(obj, '__javavalue__'):
        return JClass(obj.__javaclass__)

    if hasattr(obj, '__javaclass__'):
        return _java_lang_Class

    if obj == None:
        return _java_lang_Object

    raise TypeError(
        "Unable to determine the default type of `{0}`".format(obj.__class__))

# Patch for forName


@_jcustomizer.JImplementationFor('java.lang.Class')
class _JavaLangClass(object):
    @classmethod
    def forName(cls, *args):
        if len(args) == 1 and isinstance(args[0], str):
            return cls._forName(args[0], True, _java_ClassLoader)
        else:
            return cls._forName(*args)


def typeLookup(tp, name):
    """ Fetch a descriptor from the inheritance tree.

    This uses a cache to avoid additional cost when accessing items deep in 
    the tree multiple times.
    """
    # TODO this cache may have interactions with retroactive
    # customizers. We should likely clear the cache if
    # the class is customized.
    try:
        cache = tp.__dict__['_cache']
    except:
        cache = {}
        type.__setattr__(tp, '_cache', cache)
    if name in cache:
        return cache[name]

    # Drill down the tree searching all parents for a field
    for cls in tp.__mro__:
        if name in cls.__dict__:
            obj = cls.__dict__[name]
            cache[name] = obj
            return obj

    cache[name] = None
    return None
