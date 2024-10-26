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
import inspect
import enum
import sys
import weakref
import types
import typing

__all__ = ['JClass', 'JFinal', 'JInterface', 'JOverride', 'JPublic', 'JProtected',
        'JPrivate', 'JStatic', 'JThrows']


_extension_classloaders = weakref.WeakKeyDictionary()


def _get_annotations(target, globals, locals):
    # this isn't as straightforward as you might expect since our class hasn't been created yet
    if isinstance(target, _JClassTable):
        ann = target.get("__annotations__", {})
    else:
        ann = getattr(target, "__annotations__", {})
    if not ann:
        return ann
    return {
        key: value if not isinstance(value, str) else eval(value, globals, locals)
        for key, value in ann.items()
    }


class _JFieldDecl(object):

    def __init__(self, cls, modifiers):
        self.cls = cls
        self.modifiers = modifiers

    def __repr__(self):
        return "Field(%s,%s)" % (self.cls.__name__, self.name)


def _JMemberDecl(nonlocals, target, strict, modifiers, locals, globals, **kwargs):
    """Generic annotation to pass to the code generator.
    """
    if not "__jspec__" in nonlocals:
        nonlocals["__jspec__"] = set()
    jspec: set = nonlocals["__jspec__"]

    if isinstance(target, type):
        if not isinstance(target, _jpype.JClass):
            raise TypeError("Fields must be Java classes")
        prim = issubclass(target, (_jpype._JBoolean, _jpype._JNumberLong, _jpype._JChar, _jpype._JNumberFloat))
        out = []
        for p, v in kwargs.items():
            if not prim and v is not None:
                raise ValueError("Initial value must be None")
            if prim:
                v = _jpype.JObject(v, target)  # box it
            var = _JFieldDecl(target, p, v, modifiers)
            jspec.add(var)
            out.append(var)
        return out

    if isinstance(target, classmethod):
        target = target.__func__

    if isinstance(target, type(_JMemberDecl)):
        annotations = _get_annotations(target, globals, locals)
        args = inspect.getfullargspec(target).args

        # Verify the requirements for arguments are met
        # Must have a this argument first
        if strict:
            if len(args) < 1:
                raise TypeError("Methods require this argument")
            if args[0] not in ("self", "cls", "this"):
                raise TypeError("Methods first argument must be this")

            # All other arguments must be annotated as JClass types
            for i in range(1, len(args)):
                if not args[i] in annotations:
                    raise TypeError("Methods types must have specifications")
                if not isinstance(annotations[args[i]], _jpype.JClass):
                    raise TypeError("Method arguments must be Java classes")

            if target.__name__ != "__init__":
                if not isinstance(annotations.get("return", None), (_jpype.JClass, type(None))):
                    raise TypeError("Return type must be Java type")

        # Place in the Java spec list
        for p, v in kwargs.items():
            object.__setattr__(target, p, v)
        if modifiers is not None:
            jspec.add(target)
            object.__setattr__(target, '__jmodifiers__', modifiers)
        return target

    raise TypeError("Unknown Java specification '%s'" % type(target))


@enum.global_enum
class _JModifier(enum.IntFlag):

    JPublic = 1
    JPrivate = 2
    JProtected = 4
    JStatic = 8
    JFinal = 16

    def __call__(self, target, **kwargs):
        modifier = int(self)
        if hasattr(target, '__jmodifiers__'):
            target.__jmodifiers__ |= modifier
            return target
        elif isinstance(target, classmethod):
            target = target.__func__
            modifier |= JStatic

        nonlocals = inspect.stack()[1][0].f_locals
        frame = inspect.stack()[2][0]
        locals = frame.f_locals
        globals = frame.f_globals
        return _JMemberDecl(nonlocals, target, True, modifier, locals, globals, **kwargs)

    def __class_getitem__(cls, key):
        if isinstance(key, _JFieldDecl):
            key.modifiers |= cls.modifier
            return key
        return _JFieldDecl(key, cls.modifier)


JPublic = _JModifier.JPublic
JPrivate = _JModifier.JPrivate
JProtected = _JModifier.JProtected
JStatic = _JModifier.JStatic
JFinal = _JModifier.JFinal


def JThrows(*args):
    for arg in args:
        if not isinstance(arg, _jpype.JException):
            raise TypeError("JThrows requires Java exception arguments")

    def deferred(target):
        throws = getattr(target, '__jthrows__', tuple())
        # decorators are processed lifo
        # preserve the order in which they were added
        object.__setattr__(target, '__jthrows__', (*args, *throws))
        return target
    return deferred


def JOverride(*target, sticky=False, rename=None, **kwargs):
    """Annotation to denote a method as overriding a Java method.

    This annotation applies to customizers, proxies, and extensions
    to Java classes. Apply it to methods to mark them as implementing
    or overriding Java methods.  Keyword arguments are passed to the
    corresponding implementation factory.

    Args:
      sticky=bool: Applies a customizer method to all derived classes.

    """
    nonlocals = inspect.stack()[1][0].f_locals
    frame = inspect.stack()[2][0]
    locals = frame.f_locals
    globals = frame.f_globals
    if len(target) == 0:
        overrides = {}
        if kwargs:
            overrides.update(kwargs)
        if sticky:
            overrides["sticky"] = True
        if rename is not None:
            overrides["rename"] = rename

        def deferred(method):
            return _JMemberDecl(nonlocals, method, False, None, locals, globals, __joverride__=overrides)
        return deferred
    if len(target) == 1:
        return _JMemberDecl(nonlocals, *target, False, None, locals, globals, __joverride__={})
    raise TypeError("JOverride can only have one argument")


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

    def __class_getitem__(cls, index):
        # enables JClass[1] to get a Class[]
        return JClass("java.lang.Class")[index]


class JInterface(_jpype._JObject, internal=True):  # type: ignore[call-arg]
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
            # Remove unmappable functions
            if k2:
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


class _JClassTable(dict):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        frame = inspect.stack()[1][0]
        self.locals = frame.f_locals
        self.globals = frame.f_globals
        self.field_annotations = {}
        self.annotations = tuple()

    def __setitem__(self, key, value):
        if key == "__jannotations__":
            if isinstance(value, JClass):
                self.annotations = (_jpype.JAnnotation(value),)
            elif isinstance(value, _jpype.JAnnotation):
                self.annotations = (value,)
            else:
                self.annotations = value
        if not hasattr(value, "__jmodifiers__"):
            if isinstance(value, tuple):
                if value and isinstance(value[0], _jpype.JAnnotation):
                    self.field_annotations[key] = value
                    value = value[0]._default
            elif isinstance(value, _jpype.JAnnotation):
                self.field_annotations[key] = (value,)
                value = value._default
            elif isinstance(value, JClass) and value.class_.isAnnotation():
                self.field_annotations[key] = (_jpype.JAnnotation(value),)
                value = None
            dict.__setitem__(self, key, value)

    @property
    def loader(self):
        return self.globals['__loader__']

    @property
    def module(self):
        return self['__module__']


def _get_classloader(members: _JClassTable):
    if members.module in sys.modules:
        if members.loader is sys.modules[members.module].__loader__:
            # we don't have the module object at this point
            # so we check for reference equality of the loader
            # any module loaded "normally" (via import)
            # will use the "builtin" extension classloader
            # and will live for the life of the program
            # just like every other JClass.
            return None
    classloader = _extension_classloaders.get(members.loader)
    if classloader is None:
        classloader = _jpype.JClass('org.jpype.extension.Factory').getNewExtensionClassLoader()
        _extension_classloaders[members.loader] = classloader
        finalizer = weakref.finalize(members.loader, type(classloader).cleanup, classloader)
        finalizer.atexit = False
    return classloader


def _throw_java_exception(cls: JClass, msg: str):
    # Unfortunately our Kevlar is either worn out or I have
    # it on inside out. We need to create the requested exception
    # and give it an empty stack trace. If we don't, then the JVM
    # will crash in initStackTraceElements.
    ex = cls(msg)
    ex.setStackTrace(JClass("java.lang.StackTraceElement")[0])
    return ex


def _add_annotations(member, java_annotations):
    # this is gross but I couldn't think of a better way
    # importing normally would cause a circular import
    from jpype import JParameterAnnotation
    args = JClass("java.util.ArrayList")()
    for annotation in java_annotations:
        if isinstance(annotation, JParameterAnnotation):
            param = member.getParameter(annotation._name)
            _add_annotations(param, annotation._annotations)
        else:
            args.add(annotation._decl)
    member.setAnnotations(args)


def _prepare_methods(cls, members: _JClassTable):
    jspec = members['__jspec__']

    functions = []
    for i in jspec:
        if isinstance(i, type(_JExtension)):
            exceptions = getattr(i, '__jthrows__', None)
            mspec = inspect.getfullargspec(i)
            annotations = _get_annotations(i, members.globals, members.locals)
            if i.__name__ == "__init__":
                names = mspec.args[1:]
                args = [annotations[j] for j in names]
                fun = cls.addCtor(args, names, exceptions, i.__jmodifiers__) # type: ignore[attr-defined]
                functions.append(i)
            else:
                names = mspec.args[1:]
                args = [annotations[j] for j in names]
                ret = annotations.get("return", None)
                fun = cls.addMethod(i.__name__, ret, args, names, exceptions, i.__jmodifiers__) # type: ignore[attr-defined]
                functions.append(i)
            java_annotations = getattr(i, "__jannotations__", None)
            if java_annotations:
                _add_annotations(fun, java_annotations)
            try:
                members.pop(i.__name__)
            except KeyError:
                pass
        else:
            raise TypeError("Unknown member %s" % type(i))

    return functions


def _prepare_fields(cls, members: _JClassTable):
    def compute_modifiers(mods: tuple[type[_JModifier]]):
        res = 0
        for mod in mods:
            res |= mod
        return res

    for k, v in _get_annotations(members, members.globals, members.locals).items():
        if hasattr(v, "__metadata__"):
            modifiers = compute_modifiers(v.__metadata__)
            field = cls.addField(v.__origin__, k, members.pop(k, None), modifiers)
            java_annotations = members.field_annotations.get(k)
            if java_annotations:
                _add_annotations(field, java_annotations)


def _JExtension(_, bases, members: _JClassTable):
    if "__jspec__" not in members:
        raise TypeError("Java classes cannot be extended in Python")

    Factory = _jpype.JClass('org.jpype.extension.Factory')
    ldr = _get_classloader(members)
    cls = Factory.newClass(members["__qualname__"], bases, ldr)

    java_annotations = members.annotations
    if java_annotations:
        _add_annotations(cls, java_annotations)

    functions = _prepare_methods(cls, members)
    _prepare_fields(cls, members)

    overrides = []
    res = Factory.loadClass(cls)
    for i, method in enumerate(cls.getMethods()):
        overrides.append((method.retId, method.parametersId, functions[i]))

    return (res, overrides)


# Install module hooks
_jpype.JClass = JClass
_jpype.JInterface = JInterface
_jpype._jclassDoc = _jclassDoc
_jpype._jclassPre = _jclassPre
_jpype._jclassPost = _jclassPost
_jpype._JExtension = _JExtension
_jpype._JClassTable = _JClassTable
_jpype._throw_java_exception = _throw_java_exception
