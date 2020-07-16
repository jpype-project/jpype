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
from ._pykeywords import pysafe
from . import _jcustomizer

__all__ = ['JClass', 'JInterface', 'JOverride']


def JOverride(*args, **kwargs):
    """Annotation to denote a method as overriding a Java method.

    This annotation applies to customizers, proxies, and extensions
    to Java classes. Apply it to methods to mark them as implementing
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


class JClassMeta(type):
    def __instancecheck__(self, other):
        return type(other) == _jpype._JClass


class JClass(_jpype._JClass, metaclass=JClassMeta):
    """Meta class for all Java class instances.

    When called as an object, JClass will contruct a new Java class wrapper.

    All Python wrappers for Java classes derive from this type.
    To test if a Python class is a Java wrapper use
    ``isinstance(obj, jpype.JClass)``.

    Args:
      className (str): name of a Java type.

    Keyword Args:
      loader (java.lang.ClassLoader): specifies a class loader to use
        when creating a class.
      initialize (bool): If true the class will be loaded and initialized.
        Otherwise, static members will be uninitialized.

    Returns:
      JavaClass: a new wrapper for a Java class

    Raises:
      TypeError: if the component class is invalid or could not be found.
    """
    def __new__(cls, jc, loader=None, initialize=True):
        if loader and isinstance(jc, str):
            jc = _jpype._java_lang_Class.forName(jc, initialize, loader)

        # Handle generics
        if isinstance(jc, str) and jc.endswith(">"):
            i = jc.find("<")
            params = jc[i + 1:-1]
            ret = _jpype._getClass(jc[:i])
            acceptParams = len(ret.class_.getTypeParameters())
            if acceptParams == 0:
                raise TypeError(
                    "Java class '%s' does not take parameters" % (ret.__name__))
            if len(params) > 0:
                params = params.split(',')
            if len(params) > 0 and len(params) != len(ret.class_.getTypeParameters()):
                raise TypeError(
                    "Java generic class '%s' length mismatch" % (ret.__name__))
            return ret

        # Pass to class factory to create the type
        return _jpype._getClass(jc)


class JInterface(_jpype._JObject, internal=True):
    """A meta class for all Java Interfaces.

    ``JInterface`` is serves as the base class for any Java class that is
    a pure interface without implementation. It is not possible to create
    a instance of a Java interface.

    Example:

    .. code-block:: python

       if isinstance(java.util.function.Function, jpype.JInterface):
          print("is interface")

    """
    pass


def _jclassPre(name, bases, members):
    # Correct keyword conflicts with Python
    m = list(members.items())
    for k, v in m:
        k2 = pysafe(k)
        if k2 != k:
            del members[k]
            members[k2] = v

    # Apply customizers
    hints = _jcustomizer.getClassHints(name)
    hints.applyCustomizers(name, bases, members)
    return (name, tuple(bases), members)


def _jclassPost(res, *args):
    # Post customizers
    hints = _jcustomizer.getClassHints(res.__name__)
    res._hints = hints
    hints.applyInitializer(res)

    # Attach public inner classes we find
    #   Due to bootstrapping, we must wait until java.lang.Class is defined
    #   before we can access the class structures.
    if _jpype._java_lang_Class:
        for cls in res.class_.getDeclaredClasses():
            if cls.getModifiers() & 1 == 0:
                continue
            wrapper = _jpype.JClass(cls)
            res._customize(str(cls.getSimpleName()), wrapper)


def _jclassDoc(cls):
    """Generator for JClass.__doc__ property

    Parameters:
       cls (JClass): class to document.

    Returns:
      The doc string for the class.
    """
    out = []
    if not hasattr(cls, "__javadoc__"):
        jde = JClass("org.jpype.javadoc.JavadocExtractor")()
        jd = jde.getDocumentation(cls)
        if jd is not None:
            setattr(cls, "__javadoc__", jd)
            if jd.description is not None:
                out.append(str(jd.description))
            if jd.ctors is not None:
                out.append(str(jd.ctors))
            return "".join(out)
    setattr(cls, "__javadoc__", None)
    from textwrap import TextWrapper
    jclass = cls.class_
    out.append("Java class '%s'" % (jclass.getName()))
    out.append("")

    sup = jclass.getSuperclass()
    if sup:
        out.append("    Extends:")
        out.append("        %s" % sup.getName())
        out.append("")

    intfs = jclass.getInterfaces()
    if intfs:
        out.append("    Interfaces:")
        words = ", ".join([str(i.getCanonicalName()) for i in intfs])
        wrapper = TextWrapper(initial_indent=' ' * 8,
                              subsequent_indent=' ' * 8)
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


# Install module hooks
_jpype.JClass = JClass
_jpype.JInterface = JInterface
_jpype._jclassDoc = _jclassDoc
_jpype._jclassPre = _jclassPre
_jpype._jclassPost = _jclassPost
