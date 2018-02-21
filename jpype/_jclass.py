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
from . import _pykeywords as _keyword

_SPECIAL_CONSTRUCTOR_KEY = "This is the special constructor key"

_JAVACLASS = None
_JAVAOBJECT = None
_JAVATHROWABLE = None
_RUNTIMEEXCEPTION = None
_CUSTOMIZERS = []

def _initialize():
    global _JAVACLASS, _JAVACLASSLOADER, _JAVAOBJECT, _JAVATHROWABLE, _RUNTIMEEXCEPTION

    # Python resources for the C module
    _jpype.setResource('GetClassMethod',_getClassFor)
    _jpype.setResource('SpecialConstructorKey',_SPECIAL_CONSTRUCTOR_KEY)
    registerClassCustomizer(_JavaLangClassCustomizer())

    # Bridge classes should be constructed in order
    _JAVAOBJECT = JClass("java.lang.Object")
    _JAVACLASS = JClass("java.lang.Class")
    _JAVATHROWABLE = JClass("java.lang.Throwable")
    _RUNTIMEEXCEPTION = JClass("java.lang.RuntimeException")
    _JAVACLASSLOADER= JClass("java.lang.ClassLoader")

    # C module needs to access bridge classes
    _jpype.setResource('JavaClass', _JavaClass)
    _jpype.setResource('JavaObject', _JavaObject)


class JClassCustomizer(object):
    """ Base class for writing class customizers.
    """
    def canCustomize(self, name):
        """ Determine if this class can be customized by this customizer.

            Return true if customize should be called, false otherwise.
        """
        pass

    def customize(self, name, jc, bases, members=None, fields=None, **kwargs):
        """ Customize the class.  

            Must be able to handle keyword arguments to support changes in customizers.

            Customizers can add additional base classes, add new members, or 
            add additional static fields.  

            When adding bases to the class, the order of the bases is important for 
            looking up the methods.  Python classes should be inserted at the front
            of the list or functionality may be lost.
        """
        pass


def _jclass_applyCustomizer(customizer, name, jc, bases, members, fields):
    """ (internal) Customizes a class using a customizer.

        This will apply either the new customizer format or the previous one.
        Support for older customizers may be removed at some future point.
    """
    if isinstance(customizer, JClassCustomizer):
        customizer.customize(name, jc, bases, members=members, fields=fields)
    else:
        customizer.customize(name, jc, bases, members)


def registerClassCustomizer(customizer):
    """ Adds a new customizer for a class.  

        Customizers can be applied before or after the class if first used.
        Customizers should derive from JClassCustomizer.
    """
    _CUSTOMIZERS.append(customizer)

    # Classes that already were created may need customization. 
    for name,cls in _JavaClassDirectory.directory.items():
        if not customizer.canCustomize(name, cls.__javaclass__):
            continue

        membersOrig = cls.__dict__
        fieldsOrig = cls.__class__.__dict__

        bases = list(cls.__bases__)
        members = dict(membersOrig)
        fields = dict(fieldsOrig)
        _jclass_applyCustomizer(customizer, name, cls.__javaclass__, bases, members, fields)

        # Adjust the members and fields
        for k,v in members.items():
            if v is not membersOrig[k]:
                type.__setattr__(cls, k, v) 
        for k,v in fields.items():
            if v is not fieldsOrig[k]:
                type.__setattr__(cls.__class__, k, v) 

        # Post change of the bases
        if bases!=list(cls.__bases__):
            meta_bases = _jclass_getMeta(bases)
            cls.__bases__ = tuple(bases)
            cls.__class__.__bases__ = meta_bases
    

class _JavaClassDirectory(object):
    """ Master directory for dynamically created java classes.

        Classes can be accessed by name or by Class<>.  
    """
    # A dictionary of Python/Java bridge classes
    directory={}

    def __init__(self, *args):
        prefix=""
        if len(args)==2:
            prefix=object.__getattribute__(args[0], "prefix")+args[1]+"."
        object.__setattr__(self, "prefix", prefix)

    def __getitem__(self, key):
        if isinstance(key, _jpype._JavaClass):
            pyJavaClass=_JavaClassDirectory.directory.get(key.getName(), None)
            if pyJavaClass is None:
                # Create a class wrapper for this type
                pyJavaClass=_JavaClass(key)
            return pyJavaClass

        if isinstance(key, str):
            if key.startswith('__'):
                return object.__getattribute__(self, key)

            # Check the cache
            prefix=object.__getattribute__(self, 'prefix')
            name=prefix+key
            pyJavaClass=_JavaClassDirectory.directory.get(name, None)
            if pyJavaClass is not None:
                return pyJavaClass

            # Allocate if possible
            #if not _jpype.isStarted():
            #    raise RuntimeError("JVM not started yet, cannot load class %s"%key)
            #if not _jpype.isThreadAttachedToJVM():
            #    _jpype.attachThreadToJVM()
            javaClass = _jpype.findClass(name)
            if javaClass is None:
                return None
            return _JavaClass(javaClass)
        
        raise RuntimeError("Directory cannot index using %s"%type(key))

    def __setattr__(self, key, value):
        raise RuntimeError("Cannot set attributes in a directory")
 
    def __getattribute__(self, key):
        value = self[key]
        if value is None:
            return _JavaClassDirectory(self, key)
        return value

    def __setstate__(self, value):
        raise RuntimeError("not implemented")

classes=_JavaClassDirectory()

def JClass(name):
    """ Create a python bridge class from a fully specified java class name

        Raises a runtime exception if the class cannot be found.
    """
    pyJavaClass=classes[name]
    if pyJavaClass is None:
        raise _RUNTIMEEXCEPTION.PYEXC("Class %s not found" % name)
    return pyJavaClass

# Hook for C module
def _getClassFor(javaClass):
    return classes[javaClass]

def _jclass_getClass(name):
    """ (internal) Get the java Class<> from a fully specified java class name
    """
    return _JAVACLASS.forName(name, True, _JAVACLASSLOADER.getSystemClassLoader())


# Are these used?
#def _javaNew(self, *args):
#    return object.__new__(self)
#
#
#def _javaExceptionNew(self, *args):
#    return Exception.__new__(self)
#

def _isJavaCtor(args):
    """ (internal) Function for testing constructors.
         
        Python objects can either be created from within python which 
        calls java to create a new object or from an existing java object.
        This tests which way the __init__ method was used.
    """
    if len(args)==1 and isinstance(args[0], tuple) \
            and args[0][0] is _SPECIAL_CONSTRUCTOR_KEY:
        return True
    return False
 

class _JavaMetaClass(type):
    """ Base class for all Java Metaclasses. 

        Meta classes hold the static fields that need to be accessed from the Class.
        Required to set up the inheritance graph.
    """
    def mro(cls):
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

#  JPype has several class types (assuming Foo is a python/java bridge class)
#     Foo$$Static - python meta class for Foo holding
#       properties for static fields and static methods
#
#     Foo - Python class which produces a Foo object() 
#       and access to static fields and static methods
#       in addition as a class type it holds all the fields and methods 
#       inherits from _JavaClass
#     Foo.__javaclass__ - private jpype capsule holding C resources
#     Foo.__class__ - python class type for class (will be Foo$$Static)
#     Foo.class_ - java.lang.Class<Foo>
#
#     Foo() - instance of Foo which wraps a java object
#       inherits from _JavaObject
#     Foo().__class__ - python class type for object (will be Foo)
#     Foo().getClass() - java.lang.Class<Foo> 
#

def _jclass_getMeta(bases):
    """ (internal) Construct a list of meta classes from a base class list.
    """
    meta_bases = []
    for i in bases:
        if i is object:
            meta_bases.append(_JavaClass)
        else:
            try:
                meta_bases.append(i.__metaclass__)
            except AttributeError:
                pass
    return tuple(meta_bases)


class _JavaClass(type):
    """ Base class for all Java Class types. 

        Use isinstance(obj, jpype.JavaClass) to test for a class.
    """
    def __str__(cls):
        return "<Java bridge %s>"%cls.__javaclass__.getName()

    def __new__(cls, jc):
        global _JAVACLASS, classes
        bases = []
        name = jc.getName()

        static_fields = {}
        members = { "__javaclass__": jc,
                "__getattribute__": _JavaObject.__getattribute__,
                "__setattr__": _JavaObject.__setattr__,
                }

        # Set the base object type
        if name == 'java.lang.Object':
            bases.append(_JavaObject)
        elif jc.isPrimitive():
            bases.append(object)
        elif not jc.isInterface():
            bjc = jc.getBaseClass()
            bases.append(classes[bjc])

        if _JAVATHROWABLE is not None and jc.isSubclass("java.lang.Throwable"):
            from . import _jexception
            members["PYEXC"] = _jexception._makePythonException(name, bjc)

        # Add interfaces
        itf = jc.getBaseInterfaces()
        for ic in itf:
            bases.append(classes[ic])

        # Fallback option (for interfaces without parents)
        if len(bases) == 0:
            bases.append(_JavaObject)

        # Add the fields
        fields = jc.getClassFields()
        for i in fields:
            # Handle name mangling
            fname = i.getName()
            if _keyword.iskeyword(fname):
                fname += "_"

            # Create a property for the field
            setter = None
            if i.isStatic():
                getter = lambda self, fld=i: fld.getStaticAttribute()
                if not i.isFinal():
                    setter = lambda self, v, fld=i: fld.setStaticAttribute(v)
                static_fields[fname] = property(getter, setter)
            else:
                getter = lambda self, fld=i: fld.getInstanceAttribute(
                    self.__javaobject__)
                if not i.isFinal():
                    setter = lambda self, v, fld=i: fld.setInstanceAttribute(
                        self.__javaobject__, v)
                members[fname] = property(getter, setter)

        # Add methods
        methods = jc.getClassMethods()  # Return tuple of tuple (name, method).
        for jm in methods:
            # Handle name mangling
            mname = jm.getName()
            if _keyword.iskeyword(mname):
                mname += "_"

            members[mname] = jm

        # class_ is the java Class<>
        static_fields['mro']= _JavaMetaClass.mro
        static_fields['class_']= property(lambda self: _jclass_getClass(name), None)

        # Apply any customizers
        for i in _CUSTOMIZERS:
            if i.canCustomize(name, jc):
                _jclass_applyCustomizer(i, name, jc, bases, members, static_fields)

        # Prepare the meta-metaclass
        meta_bases = _jclass_getMeta(bases)

        # Create the dynamic classes
        metaclass = type.__new__(_JavaMetaClass, "meta." + name, meta_bases,
                                 static_fields)
        members['__metaclass__'] = metaclass
        #FIXME this needs to have the "classes." prefix so we can look it up, but something is using __name__
        # resulting in strange behavior. Seems like a problem in native.
        result = type.__new__(metaclass, "classes."+name, tuple(bases), members)

        # Add to master directory
        _JavaClassDirectory.directory[name]=result
        return result


class _JavaObject(object):
    """ Base class for all Java Objects. 

        Use isinstance(obj, jpype.JavaObject) to test for a object.
        These default methods may be overriden by a customizer.
    """
    __metaclass__=_JavaClass

    def __init__(self, *args):
        object.__init__(self)

        if _isJavaCtor(args):
            self.__javaobject__ = args[0][1]
        else:
            self.__javaobject__ = self.__class__.__javaclass__.newClassInstance(
                *args)

    def __str__(self):
        return self.toString()

    def __hash__(self):
        return self.hashCode()

    def __eq__(self, other):
        return self.equals(other)

    def __ne__(self, other):
        return not self.equals(other)

    def __getattribute__(self, name):
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

    def __setattr__(self, attr, value):
        if attr.startswith('_') \
               or callable(value) \
               or isinstance(getattr(self.__class__, attr), property):
            object.__setattr__(self, attr, value)
        else:
            raise AttributeError("%s does not have field %s"%(self.__name__, attr), self)


# Patch for forName
def _jclass_forName(self, *args):
    if len(args)==1 and isinstance(args[0],str):
        return self._forName(args[0], True, _JAVACLASSLOADER.getSystemClassLoader())
    else:
        return self._forName(*args)

class _JavaLangClassCustomizer(JClassCustomizer):
    def canCustomize(self, name, jc):
        return name == 'java.lang.Class'

    def customize(self, name, jc, bases, members, fields, **kwargs):
        members['_forName']=members['forName']
        del members['forName']
        fields['forName']= _jclass_forName
