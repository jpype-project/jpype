# This is the prototype for forward declarations
import typing

import jpype
import _jpype

# Convert a forward symbol from string to an actual symbol


def _lookup(symbol: typing.Optional[str]):
    if symbol is None:
        return None
    if _jpype.isPackage(symbol):
        return jpype.JPackage(symbol)
    parts = symbol.split(".")
    for i in range(len(parts)):
        p = ".".join(parts[:i + 1])
        if _jpype.isPackage(p):
            continue
        obj = jpype.JClass(p)
        if i + 1 == len(parts):
            return obj
        break
    for j in range(i + 1, len(parts)):
        obj = getattr(obj, parts[j])
    return obj


# List of forwards to be resolved
_jforward = []


def _resolve(forward, value=None):
    # Only works on forward declarations
    if not isinstance(forward, JForward):
        raise TypeError("Must be a forward declaration")

    # Find the value using Java reflection
    if value is None:
        value = _lookup(forward._symbol)

    # If it was a null symbol then just skip it as unresolvable
    if value is None:
        return

    # if the memory is compatable mutate the object

    # set the instance
    object.__setattr__(forward, "_instance", value)

    # morph the so that the slots act more like their correct behavior
    #   types we have here
    #     - java classes
    #     - java array classes
    #     - java arrays
    #     - java packages
    #     - java static fields
    #     - java throwables
    #     - java primitives
    #     - other?
    if isinstance(value, _jpype._JPackage):
        object.__setattr__(forward, '__class__', _JResolvedObject)
        # FIXME if this ended up in the import table we should replace it
        # in sys.modules[]
    elif isinstance(value, _jpype._JClass):
        object.__setattr__(forward, '__class__', _JResolvedType)
    elif isinstance(value, _jpype._JArray):
        raise TypeError("Forward declarations of arrays are not supported")
    elif isinstance(value, (_jpype._JBoolean, _jpype._JNumberLong, _jpype._JChar, _jpype._JNumberLong, _jpype._JNumberFloat)):
        raise TypeError("Forward declarations of primitives are not supported")
    elif isinstance(value, object):
        # Bypass the JForward setattr, which always raises a JVM must be started
        # exception.
        object.__setattr__(forward, '__class__', _JResolvedObject)
    else:
        raise TypeError("Forward type not supported")

    # Depending on how the base class is implemented and the amount of reserve space
    # we give it we may be able to mutate it into the actual object in C, but
    # only if we can dispose of its accessing resources (dict) and copy
    # over the new resources without overrunning the space requirements.


# Hook up the resolver for all forward declarations
@jpype.onJVMStart
def _resolveAll():
    for forward in _jforward:
        _resolve(forward)
    _jforward.clear()


###########################################################################


# This will be used for morphed types, everything must have a base tree
#  We need to make sure these are fast lookup so we don't change method resoluton cost
class _JForwardProto:
    __slots__ = ('_instance', '_symbol')

# resolved symbol


class _JResolved(_JForwardProto):
    def __getattr__(self, name):
        return getattr(self._instance, name)

    def __setattr__(self, name, value):
        return setattr(self._instance, name)

    def __str__(self):
        return str(self._instance)

    def __repr__(self):
        return repr(self._instance)

    def __eq__(self, other):
        # We swap the order of the __eq__ here to allow other to influence the
        # result further (e.g. it may be a _JResolved also)
        return other == self._instance

    def __ne__(self, other):
        # We swap the order of the __ne__ here to allow other to influence the
        # result further (e.g. it may be a _JResolved also)
        return other != self._instance


# We need to specialize the forward based on the resolved object
class _JResolvedType(_JResolved):

    def __call__(self, *args):
        return self._instance(*args)

    def __matmul__(self, value):
        return self._instance @ value

    def __getitem__(self, value):
        return self._instance[value]

    def __instancecheck__(self, other):
        return isinstance(other, self._instance)

    def __subclasscheck__(self, other):
        return issubclass(self._instance, other)


class _JResolvedObject(_JResolved):
    pass


def _jvmrequired(*args):
    raise RuntimeError("JVM must be started")

# This is an unresolved symbol
#  Must have every possible slot type.


# Public view of a forward declaration
class JForward(_JForwardProto):
    # Note that we can have arbitrary attributes on this class,
    # hence we don't specify __slots__ here.

    def __new__(cls, symbol: typing.Optional[str]):
        if jpype.isJVMStarted():
            return _lookup(symbol)
        return _JForwardProto.__new__(cls)

    def __init__(self, symbol):
        object.__setattr__(self, '_symbol', symbol)
        _jforward.append(self)

    def __getattr__(self, name):
        forward = self._symbol

        if forward is None:
            # Handle this JForward being the root.
            value = JForward(name)
        elif name.startswith('['):
            value = JForward(forward + name)
        else:
            value = JForward(forward + "." + name)
        # Cache the result for later, giving us faster lookup next time.
        object.__setattr__(self, name, value)
        return value

    def __getitem__(self, value):
        if isinstance(value, slice):
            return self.__getattr__("[]")
        if isinstance(value, tuple):
            depth = 0
            for item in value:
                if isinstance(item, slice):
                    depth += 1
                else:
                    raise RuntimeError("Cannot create array without a JVM")
            return self.__getattr__("[]" * depth)
        raise RuntimeError("Cannot create array without a JVM")

    __setattr__ = _jvmrequired
    __call__ = _jvmrequired
    __matmul__ = _jvmrequired
    __instancecheck__ = _jvmrequired
    __subclasscheck__ = _jvmrequired



########################################################


def main():
    root = JForward(None)
    java = root.java
    Object = java.lang.Object
    String = java.lang.String
    Array = java.lang.String[:]
    Array2 = java.lang.String[:, :]
    Integer = java.lang.Integer

    try:
        String()
        print("fail")
    except RuntimeError:
        pass

    try:
        String @ object()
        print("fail")
    except RuntimeError:
        pass

    try:
        String @ object()
        print("fail")
    except RuntimeError:
        pass


    # String = java.lang.String2  # this does not fail here, but will when the JVM is started
    C = java.lang.String.CASE_INSENSITIVE_ORDER


    def pre(v: java.lang.String, p=java.lang.String.CASE_INSENSITIVE_ORDER) -> java.lang.String:
        return p

    #
    jpype.startJVM()


    def post(v: java.lang.String, p=java.lang.String.CASE_INSENSITIVE_ORDER) -> java.lang.String:
        return p


    # Create instance
    print(type(String("foo")) == type(jpype.java.lang.String("foo")))
    print(String == jpype.java.lang.String)  # works
    print(String is java.lang.String)  # works
    print(not (String != jpype.java.lang.String))  # works
    print(isinstance(String("foo"), String))  # works
    print(isinstance(String("foo"), jpype.java.lang.String))
    print(isinstance(jpype.java.lang.String("foo"), String))
    print(not isinstance(1, String))  # works
    print(issubclass(String, Object))  # works

    print(Integer(2) > java.lang.Integer(1))  # works
    print(java.lang.Integer(2) > Integer(1))  # works
    print(not (Integer(2) < java.lang.Integer(1)))  # works

    # Late use of static fields
    print(C == jpype.java.lang.String.CASE_INSENSITIVE_ORDER)  # works
    print(jpype.java.lang.String.CASE_INSENSITIVE_ORDER == C)  # fails

    # Annotations and defaults
    print(pre(1) == post(1))  # works
    print(post(1) == post(1))  # works
    print(pre.__annotations__['return'] == post.__annotations__['return'])  # works
    print(pre.__annotations__['v'] == post.__annotations__['v'])  # works

    print(type(Array(10)) == jpype.java.lang.String[:])  # works
    print(type(Array2(10)) == jpype.java.lang.String[:, :])  # works


if __name__ == "__main__":
    main()
