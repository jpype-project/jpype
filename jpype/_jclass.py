#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
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
#*****************************************************************************
import _jpype
from ._pykeywords import KEYWORDS



_CLASSES = {}

_SPECIAL_CONSTRUCTOR_KEY = "This is the special constructor key"

_JAVACLASS = None
_JAVAOBJECT = None
_JAVATHROWABLE = None
_COMPARABLE = None
_RUNTIMEEXCEPTION = None

_CUSTOMIZERS = []

_COMPARABLE_METHODS = {
        "__cmp__": lambda self, o: self.compareTo(o)
        }

def _initialize():
    global _COMPARABLE, _JAVACLASS, _JAVAOBJECT, _JAVATHROWABLE, _RUNTIMEEXCEPTION
    registerClassCustomizer(_JavaLangClassCustomizer())

    _JAVAOBJECT = JClass("java.lang.Object")
    _JAVACLASS = JClass("java.lang.Class")
    _JAVATHROWABLE = JClass("java.lang.Throwable")
    _RUNTIMEEXCEPTION = JClass("java.lang.RuntimeException")
    _jpype.setResource('JavaClass', _JavaClass)
    _jpype.setResource('JavaObject', _JavaObject)
    _jpype.setResource('GetClassMethod',_getClassFor)
    _jpype.setResource('SpecialConstructorKey',_SPECIAL_CONSTRUCTOR_KEY)


def registerClassCustomizer(c):
    _CUSTOMIZERS.append(c)

class JClassCustomizer(object):
    def canCustomize(self, name):
        """ Deterimine if this class can be customized by this customizer.

        Return true if customize should be called, false otherwise."""
        pass

    def customize(self, name, jc, bases, members=None, fields=None, **kwargs):
        """ Customize the class.  

        Should be able to handle keyword arguments to support changes in customizers.
        """
        pass
    

def JClass(name):
    jc = _jpype.findClass(name)
    if jc is None:
        raise _RUNTIMEEXCEPTION.PYEXC("Class %s not found" % name)

    return _getClassFor(jc)


def _getClassFor(javaClass):
    name = javaClass.getName()
    if name in _CLASSES:
        return _CLASSES[name]

    pyJavaClass = _JavaClass(javaClass)
    _CLASSES[name] = pyJavaClass
    return pyJavaClass


def _javaNew(self, *args):
    return object.__new__(self)


def _javaExceptionNew(self, *args):
    return Exception.__new__(self)


def _isJavaCtor(args):
    if len(args)==1 and isinstance(args[0], tuple) \
            and args[0][0] is _SPECIAL_CONSTRUCTOR_KEY:
        return True
    return False
 

def _javaInit(self, *args):
    object.__init__(self)

    if len(args) == 1 and isinstance(args[0], tuple) \
       and args[0][0] is _SPECIAL_CONSTRUCTOR_KEY:
        self.__javaobject__ = args[0][1]
    else:
        self.__javaobject__ = self.__class__.__javaclass__.newClassInstance(
            *args)


def _javaGetAttr(self, name):
    try:
        r = object.__getattribute__(self, name)
    except AttributeError as ex:
        if name in dir(self.__class__.__metaclass__):
            r = object.__getattribute__(self.__class__, name)
        else:
            raise ex

    if isinstance(r, _jpype._JavaMethod):
        return _jpype._JavaBoundMethod(r, self)
    return r

def _javaSetAttr(self, attr, value):
    if attr.startswith('_') \
           or callable(value) \
           or isinstance(getattr(self.__class__, attr), property):
        object.__setattr__(self, attr, value)
    else:
        raise AttributeError("%s does not have field %s"%(self.__name__, attr), self)

def _mro_override_topsort(cls):
    # here we run a topological sort to get a linear ordering of the inheritance graph.
    parents = set().union(*[x.__mro__ for x in cls.__bases__])
    numsubs = dict()
    for cls1 in parents:
        numsubs[cls1] = len([cls2 for cls2 in parents if cls1 != cls2 and issubclass(cls2,cls1)])
    mergedmro = [cls]
    while numsubs:
        for k1,v1 in numsubs.items():
            if v1 != 0: continue
            mergedmro.append(k1)
            for k2,v2 in numsubs.items():
                if issubclass(k1,k2):
                    numsubs[k2] = v2-1
            del numsubs[k1]
            break
    return mergedmro

class _MetaClassForMroOverride(type):
    def mro(cls):
        return _mro_override_topsort(cls)


class _JavaObject(object):
    """ Base class for all Java Objects. 

        Use isinstance(obj, jpype.JavaObject) to test for a object.
    """
    pass

#  JPype has several class types (assuming Foo is a java class)
#     Foo$$Static - python meta class for Foo holding
#       properties for static fields and static methods
#
#     Foo - Python class which produces a Foo object() 
#       and access to static fields and stataic methods
#       in addition as a class type it holds all the fields and methods 
#       inherites from _JavaClass
#     Foo.__javaclass__ - private jpype capsule holding C resources
#     Foo.__class__ - python class type for class (will be Foo$$Static)
#     Foo.class_ - java.lang.Class<Foo>
#
#     Foo() - instance of Foo which wraps a java object
#       inherits from _JavaObject
#     Foo().__class__ - ptthon class type for object (will be Foo)
#     Foo().getClass() - java.lang.Class<Foo> 
#


class _JavaClass(type):
    """ Base class for all Java Class types. 

        Use isinstance(obj, jpype.JavaClass) to test for a class.
    """
    def __new__(cls, jc):
        global _JAVACLASS
        bases = []
        name = jc.getName()

        static_fields = {}
        members = {
                "__javaclass__": jc,
                "__init__": _javaInit,
                "__str__": lambda self: self.toString(),
                "__hash__": lambda self: self.hashCode(),
                "__eq__": lambda self, o: self.equals(o),
                "__ne__": lambda self, o: not self.equals(o),
                "__getattribute__": _javaGetAttr,
                "__setattr__": _javaSetAttr,
                }

        if name == 'java.lang.Object':
            bases.append(_JavaObject)
        elif jc.isPrimitive():
            bases.append(object)
        elif not jc.isInterface():
            bjc = jc.getBaseClass()
            bases.append(_getClassFor(bjc))

        if _JAVATHROWABLE is not None and jc.isSubclass("java.lang.Throwable"):
            from . import _jexception
            members["PYEXC"] = _jexception._makePythonException(name, bjc)

        itf = jc.getBaseInterfaces()
        for ic in itf:
            bases.append(_getClassFor(ic))

        if len(bases) == 0:
            bases.append(_JavaObject)

        # add the fields
        fields = jc.getClassFields()
        for i in fields:
            fname = i.getName()
            if fname in KEYWORDS:
                fname += "_"

            if i.isStatic():
                g = lambda self, fld=i: fld.getStaticAttribute()
                s = None
                if not i.isFinal():
                    s = lambda self, v, fld=i: fld.setStaticAttribute(v)
                static_fields[fname] = property(g, s)
            else:
                g = lambda self, fld=i: fld.getInstanceAttribute(
                    self.__javaobject__)
                s = None
                if not i.isFinal():
                    s = lambda self, v, fld=i: fld.setInstanceAttribute(
                        self.__javaobject__, v)
                members[fname] = property(g, s)

        # methods
        methods = jc.getClassMethods()  # Return tuple of tuple (name, method).
        for jm in methods:
            mname = jm.getName()
            if mname in KEYWORDS:
                mname += "_"

            members[mname] = jm

        static_fields['mro'] = _mro_override_topsort
        static_fields['class_']= property(lambda self: _JAVACLASS.forName(name,True, JClass('java.lang.ClassLoader').getSystemClassLoader()), None)

        for i in _CUSTOMIZERS:
            if i.canCustomize(name, jc):
                if isinstance(i, JClassCustomizer):
                    i.customize(name, jc, bases, members=members, fields=static_fields)
                else:
                    i.customize(name, jc, bases, members)

        # Prepare the meta-metaclass
        meta_bases = []
        for i in bases:
            if i is object or i is _JavaObject:
                meta_bases.append(cls)
            else:
                meta_bases.append(i.__metaclass__)

        metaclass = type.__new__(_MetaClassForMroOverride, name + "$$Static", tuple(meta_bases),
                                 static_fields)
        members['__metaclass__'] = metaclass
        result = type.__new__(metaclass, name, tuple(bases), members)

        return result

# Patch for forName
def _jclass_forName(self, *args):
    if len(args)==1 and isinstance(args[0],str):
        return self._forName(args[0], True, JClass('java.lang.ClassLoader').getSystemClassLoader())
    else:
        return self._forName(*args)

class _JavaLangClassCustomizer(JClassCustomizer):
    def canCustomize(self, name, jc):
        return name == 'java.lang.Class'

    def customize(self, name, jc, bases, members, fields, **kwargs):
        members['_forName']=members['forName']
        del members['forName']
        fields['forName']= _jclass_forName
