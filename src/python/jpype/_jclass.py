#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#	   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#   
#*****************************************************************************
import _jpype, _jexception, _jcollection
from _pykeywords import KEYWORDS
import sets


_CLASSES = {}

_SPECIAL_CONSTRUCTOR_KEY = "This is the special constructor key"

_JAVAOBJECT = None
_JAVATHROWABLE = None
_COMPARABLE = None
_RUNTIMEEXCEPTION = None

_CUSTOMIZERS = []

_COMPARABLE_METHODS = { 
	"__cmp__" : lambda self, o : self.compareTo(o)
}

def _initialize() :
	global _COMPARABLE, _JAVAOBJECT, _JAVATHROWABLE, _RUNTIMEEXCEPTION
	
	import _jpackage
	_JAVAOBJECT = JClass("java.lang.Object")
	_JAVATHROWABLE = JClass("java.lang.Throwable")
	_RUNTIMEEXCEPTION = JClass("java.lang.RuntimeException")
	_jpype.setJavaLangObjectClass(_JAVAOBJECT)
	_jpype.setGetClassMethod(_getClassFor)
	_jpype.setSpecialConstructorKey(_SPECIAL_CONSTRUCTOR_KEY)

def registerClassCustomizer(c) :
	_CUSTOMIZERS.append(c)
	
def JClass(name) :
	jc = _jpype.findClass(name)
	if jc is None :
		raise _RUNTIMEEXCEPTION.PYEXC("Class %s not found" % name)
		
	return _getClassFor(jc)

def _getClassFor(javaClass) :
	name = javaClass.getName()
	if _CLASSES.has_key(name) :
		return _CLASSES[name]
		
	pyJavaClass = _JavaClass(javaClass)
	_CLASSES[name] = pyJavaClass
	return pyJavaClass
	
def _javaNew(self, *args) :
	return object.__new__(self)
	
def _javaExceptionNew(self, *args) :
	return Exception.__new__(self)

def _javaInit(self, *args) :
	object.__init__(self)
	
	if len(args) == 1 and isinstance(args[0], tuple) and args[0][0] is _SPECIAL_CONSTRUCTOR_KEY :
		self.__javaobject__ = args[0][1]
	else:
		self.__javaobject__ = self.__class__.__javaclass__.newClassInstance(*args)
	
def _javaGetAttr(self, name) :
	try :
		r = object.__getattribute__(self, name)
	except AttributeError, ex :
		if name in dir(self.__class__.__metaclass__) :
			r = object.__getattribute__(self.__class__, name)
		else:
			raise ex
		
	if isinstance(r, _jpype._JavaMethod) :
		return _jpype._JavaBoundMethod(r, self) 
	return r

class _JavaClass(type) :  
	def __new__(mcs, jc) :
		bases = []
		name = jc.getName()
		
		static_fields = {}
		constants = []
		members = {
			"__javaclass__" : jc,
			"__init__" : _javaInit,
			"__str__" : lambda self : self.toString(),
			"__hash__" : lambda self : self.hashCode(),
			"__eq__" : lambda self, o : self.equals(o),
			"__ne__" : lambda self, o : not self.equals(o),
			"__getattribute__" : _javaGetAttr,
		}
		
		if name == 'java.lang.Object' or jc.isPrimitive():
			bases.append(object)
		elif not jc.isInterface() :
			bjc = jc.getBaseClass(jc)
			bases.append( _getClassFor(bjc) )
		
		if _JAVATHROWABLE is not None and jc.isSubclass("java.lang.Throwable") :
			members["PYEXC"] = _jexception._makePythonException(name, bjc)

		itf = jc.getBaseInterfaces()
		for ic in itf :
			bases.append( _getClassFor(ic) )
		
		if len(bases) == 0 :
			bases.append(_JAVAOBJECT)
		
		# add the fields	
		fields = jc.getClassFields()
		for i in fields :
			fname = i.getName()
			if fname in KEYWORDS :
				fname = fname + "_"
				
			if i.isStatic() :
				g = lambda self, fld=i : fld.getStaticAttribute()
				s = None
				if not i.isFinal() :
					s = lambda self, v, fld=i : fld.setStaticAttribute(v)
				static_fields[fname] = property(g, s)
			else:
				g = lambda self, fld=i : fld.getInstanceAttribute(self.__javaobject__)
				s = None
				if not i.isFinal() :
					s = lambda self, v, fld=i : fld.setInstanceAttribute(self.__javaobject__, v)
				members[fname] = property(g, s)
		
		# methods
		methods = jc.getClassMethods() # return tuple of tuple (name, method)
		for jm in methods :
			mname = jm.getName()
			if mname in KEYWORDS :
				mname = mname + "_"
								
			members[mname] = jm

		for i in _CUSTOMIZERS :
			if i.canCustomize(name, jc) :
				i.customize(name, jc, bases, members)
		
		# remove multiple bases that would cause a MRO problem
		toRemove = sets.Set()
		for c in bases :
			for d in bases :
				if c == d :
					continue
				if issubclass(c, d) :
					toRemove.add(d)
					
		for i in toRemove :
			bases.remove(i)

		# Prepare the meta-metaclass
		meta_bases = []
		for i in bases :
			if i is object :
				meta_bases.append(mcs)
			else:
				meta_bases.append(i.__metaclass__)
				
		metaclass = type.__new__(type, name+"$$Static", tuple(meta_bases), static_fields)
		members['__metaclass__'] = metaclass
		result =  type.__new__(metaclass, name, tuple(bases), members)
		
		return result
				
