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

_JObject = None

# FIXME reconnect customizers
#_jcustomizer._JCLASSES = _JCLASSES


def JOverride(*args, **kwargs):
    """Annotation to denote a method as overriding a Java method.

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


class JClass(_jpype.PyJPClassMeta):
    """Meta class for all java class instances.

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
    def __new__(cls, *args, loader=None, initialize=True):
        if len(args) == 1:
            jc = args[0]
            if loader and isinstance(jc, str):
                jc = _jpype._java_lang_Class.forName(arg, initialize, loader)
            elif isinstance(jc, str):
                jc = _jpype.PyJPClass(*args)
            if jc is None:
                raise _jpype._java_lang_RuntimeException(
                    "Java class '%s' not found" % name)
            return _jpype._getClass(jc)
        return super(JClass, cls).__new__(cls, *args)

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

        return out

    def __repr__(self):
        return "<java class '%s'>" % (self.__name__)

    @property
    def class_(self):
        return _jpype.JObject(self.__javaclass__)


class JInterface(metaclass=_jpype.PyJPClassMeta):
    """Virtual Base class for all Java Interfaces.

    ``JInterface`` is serves as the base class for any java class that is
    a pure interface without implementation. It is not possible to create
    a instance of a Java interface. 

    Example:

    .. code-block:: python

       if issubclass(java.util.function.Function, jpype.JInterface):
          print("is interface")

        Use ``isinstance(obj, jpype.JavaInterface)`` to test for a interface.
    """
    pass


def _JClassFactory(jc):
    """ This hook creates Python class wrappers for each Java class.

    The wrapper is a Python class containing:
      - private member ``__javaclass__`` pointing to a java.lang.Class
        instance.
      - member and static methods
      - member and static fields
      - Python wrappers for inner classes
      - anything added by the customizer
    """
    from . import _jarray

    # Set up bases
    name = jc.__javaname__
    bases = list(jc._bases)

    # Set up members
    members = {
        "__javaclass__": jc,
        "__name__": name,
    }
    for field in jc._fields:
        members[pysafe(field.__name__)] = field
    for method in jc._methods:
        members[pysafe(method.__name__)] = method

    # Apply customizers
    _jcustomizer._applyCustomizers(name, jc, bases, members)
    res = _jpype.JClass(name, tuple(bases), members)

    # We need to register wrapper before any further actions
    jc._setHost(res)

    # Post customizers
    _jcustomizer._applyInitializer(res)

    # Attach public inner classes we find
    #   Due to bootstrapping, we must wait until java.lang.Class is defined
    #   before we can access the class structures.
    if _jpype._java_lang_Class:
        for cls in res.class_.getDeclaredClasses():
            if cls.getModifiers() & 1 == 0:
                continue
            wrapper = _jpype.JClass(cls)
            type.__setattr__(res, str(cls.getSimpleName()), wrapper)

    return res


def _jclassDoc(cls):
    """Generator for JClass.__doc__ property

    Parameters:
       cls (JClass): class to document.

    Returns:
      The doc string for the class.
    """
    from textwrap import TextWrapper
    jclass = cls.class_
    out = []
    out.append("Java class '%s'" % (jclass.getName()))
    out.append("")

    sup = jclass.getSuperclass()
    if sup:
        out.append("  Extends:")
        out.append("    %s" % sup.getName())
        out.append("")

    intfs = jclass.getInterfaces()
    if intfs:
        out.append("  Interfaces:")
        words = ", ".join([str(i.getCanonicalName()) for i in intfs])
        wrapper = TextWrapper(initial_indent='        ',
                              subsequent_indent='        ')
        out.extend(wrapper.wrap(words))
        out.append("")

    ctors = jclass.getDeclaredConstructors()
    if ctors:
        exceptions = []
        name = jclass.getSimpleName()
        ctordecl = []
        for ctor in ctors:
            modifiers = ctor.getModifiers()
            if not modifiers & 1:
                continue
            params = ", ".join([str(i.getCanonicalName())
                                for i in ctor.getParameterTypes()])
            ctordecl.append("    * %s(%s)" % (name, params))
            exceptions.extend(ctor.getExceptionTypes())
        if ctordecl:
            out.append("  Constructors:")
            out.extend(ctordecl)
            out.append("")
        if exceptions:
            out.append("  Raises:")
            for exc in set(exceptions):
                out.append("    %s: from java" % exc.getCanonicalName())
            out.append("")

    fields = jclass.getDeclaredFields()
    if fields:
        fielddesc = []
        for field in fields:
            modifiers = field.getModifiers()
            if not modifiers & 1:
                continue
            fieldInfo = []
            if modifiers & 16:
                fieldInfo.append("final")
            if modifiers & 8:
                fieldInfo.append("static")
            if field.isEnumConstant():
                fieldInfo.append("enum constant")
            else:
                fieldInfo.append("field")
            fielddesc.append("    %s (%s): %s" % (field.getName(),
                                                  field.getType().getName(),
                                                  " ".join(fieldInfo)))
        if fielddesc:
            out.append("  Attributes:")
            out.extend(fielddesc)
            out.append("")

    return "\n".join(out)

# Install hooks
_jpype.JClass = JClass
_jpype.JInterface = JInterface
_jpype._JClassFactory = _JClassFactory
_jpype._jclassDoc = _jclassDoc
