/*****************************************************************************
   Copyright 2004-2008 Steve Menard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
*****************************************************************************/   
#include <jpype_python.h>

#define UNWRAP(ref) ((PyObject*)ref->data())

#define GETDESC(ref) string((char*)JPyCObject::getDesc(UNWRAP(ref)))
#define WRAP(ref, t) new HostRef( JPyCObject::fromVoidAndDesc(ref, (void*)t, NULL) )
#define IS_REF(ref, t) JPyCObject::check((PyObject*)ref) && GETDESC(ref) == t

void* PythonHostEnvironment::acquireRef(void* d)
{
	Py_XINCREF((PyObject*)d);
	return d;
}

void PythonHostEnvironment::releaseRef(void* d)
{
	Py_XDECREF((PyObject*)d);
}

bool PythonHostEnvironment::isRefNull(void* d)
{
	return d == NULL;
}

string PythonHostEnvironment::describeRef(HostRef* ref)
{
	stringstream out;
	
	return out.str();
	
}

void* PythonHostEnvironment::gotoExternal()
{  
	PyThreadState *_save; 
	_save = PyEval_SaveThread();
	return (void*)_save;
}

void PythonHostEnvironment::returnExternal(void* state)
{
	PyThreadState *_save = (PyThreadState *)state;
	PyEval_RestoreThread(_save);
}

void PythonHostEnvironment::setRuntimeException(const char* msg)
{
	JPyErr::setString(PyExc_RuntimeError, msg);
}

void PythonHostEnvironment::setAttributeError(const char* msg)
{
	JPyErr::setString(PyExc_AttributeError, msg);
}

void PythonHostEnvironment::setTypeError(const char* msg)
{
	JPyErr::setString(PyExc_TypeError, msg);
}

void PythonHostEnvironment::raise(const char* msg)
{
	RAISE(JPypeException, msg);
}

HostRef* PythonHostEnvironment::getNone()
{
	return new HostRef(Py_None);
}

bool PythonHostEnvironment::isNone(HostRef* ref)
{
	return UNWRAP(ref) == Py_None;
}

bool PythonHostEnvironment::isBoolean(HostRef* ref)
{
	return JPyBoolean::isTrue(UNWRAP(ref)) || JPyBoolean::isFalse(UNWRAP(ref));
}

jboolean PythonHostEnvironment::booleanAsBoolean(HostRef* ref)
{
	if (JPyBoolean::isTrue(UNWRAP(ref)))
	{
		return true;
	}
	return false;
}

HostRef* PythonHostEnvironment::getTrue()
{
	return new HostRef(JPyBoolean::getTrue(), false);
}

HostRef* PythonHostEnvironment::getFalse()
{
	return new HostRef(JPyBoolean::getFalse(), false);
}

bool PythonHostEnvironment::isSequence(HostRef* ref)
{
	return JPySequence::check(UNWRAP(ref)) && ! JPyString::check(UNWRAP(ref));
}

HostRef* PythonHostEnvironment::newMutableSequence(jsize sz)
{
	return new HostRef(JPySequence::newList(sz), false);
}

HostRef* PythonHostEnvironment::newImmutableSequence(jsize sz)
{
	return new HostRef(JPySequence::newTuple(sz), false);
}

jsize PythonHostEnvironment::getSequenceLength(HostRef* ref)
{
	return (jsize)JPyObject::length(UNWRAP(ref));
}

HostRef* PythonHostEnvironment::getSequenceItem(HostRef* ref, jsize pos)
{
	return new HostRef(JPySequence::getItem(UNWRAP(ref), pos), false);
}

void PythonHostEnvironment::setSequenceItem(HostRef* seq, jsize pos, HostRef* val)
{
	JPySequence::setItem(UNWRAP(seq), pos, UNWRAP(val));
}

bool PythonHostEnvironment::isInt(HostRef* res)
{
	return JPyInt::check(UNWRAP(res));
}

HostRef* PythonHostEnvironment::newInt(jint v)
{
	return new HostRef(JPyInt::fromLong(v), false);
}

jint PythonHostEnvironment::intAsInt(HostRef* res)
{
	return JPyInt::asLong(UNWRAP(res));
}

bool PythonHostEnvironment::isLong(HostRef* ref)
{
	return JPyLong::check(UNWRAP(ref));
}

HostRef* PythonHostEnvironment::newLong(jlong v)
{
	TRACE_IN("PythonHostEnvironment::newLong");
	return new HostRef(JPyLong::fromLongLong(v), false);
	TRACE_OUT;
}

jlong PythonHostEnvironment::longAsLong(HostRef* ref)
{
	return JPyLong::asLongLong(UNWRAP(ref));
}

bool PythonHostEnvironment::isFloat(HostRef* ref)
{
	return JPyFloat::check(UNWRAP(ref));
}

HostRef* PythonHostEnvironment::newFloat(jdouble v)
{
	return new HostRef(JPyFloat::fromDouble(v), false);
}

jdouble PythonHostEnvironment::floatAsDouble(HostRef* ref)
{
	return JPyFloat::asDouble(UNWRAP(ref));
}

bool PythonHostEnvironment::isMethod(HostRef* ref)
{
	return IS_REF(ref, "JPMethod");
}

HostRef* PythonHostEnvironment::newMethod(JPMethod* m)
{
	return WRAP(m, "JPMethod");
}

JPMethod* PythonHostEnvironment::asMethod(HostRef* ref)
{
	return (JPMethod*)JPyCObject::asVoidPtr(UNWRAP(ref));
}

bool PythonHostEnvironment::isObject(HostRef* ref)
{
	PyObject* obj = UNWRAP(ref);

	if (JPyObject::isInstance(obj, m_JavaLangObject))
	{
		return true;
	}

	return false;
}

JPObject* PythonHostEnvironment::asObject(HostRef* m)
{
	PyObject* obj = (PyObject*)m->data();
	if (JPyCObject::check(obj))
	{
		return (JPObject*)JPyCObject::asVoidPtr(obj);
	}
	PyObject* javaObject = JPyObject::getAttrString(obj, "__javaobject__");

	JPObject* res = (JPObject*)JPyCObject::asVoidPtr(javaObject);
	
	Py_DECREF(javaObject);

	return res;
}

HostRef* PythonHostEnvironment::newObject(JPObject* obj)
{
	TRACE_IN("PythonHostEnvironment::newObject");
	TRACE2("classname", obj->getClass()->getName().getSimpleName());

	JPClass* jc = obj->getClass();
	JPTypeName name = jc->getName();

	PyObject* pyClass = getJavaShadowClass(jc);

	PyObject* args = JPySequence::newTuple(2);
	PyObject* arg2 = JPySequence::newTuple(1);
	JPySequence::setItem(arg2, 0, args);
	Py_DECREF(args);
	PyObject* joHolder = JPyCObject::fromVoidAndDesc((void*)obj, (void*)"JPObject", &deleteJPObjectDestructor);
	JPySequence::setItem(args, 0, m_SpecialConstructorKey);
	JPySequence::setItem(args, 1, joHolder);
	Py_DECREF(joHolder);

	PyObject* res = JPyObject::call(pyClass, arg2, NULL);
	Py_DECREF(arg2);

	return new HostRef(res, false);
	TRACE_OUT;

}

bool PythonHostEnvironment::isClass(HostRef* ref)
{
	PyObject* self = UNWRAP(ref);

	if (! JPyType::check(self)) 
	{
		// If its not a type ... it can;t be a java type
		return false;
	}

	return JPyType::isSubclass(self, m_JavaLangObject);
}

HostRef* PythonHostEnvironment::newClass(JPClass* m)
{
	PyJPClass* co = PyJPClass::alloc(m);

	PyObject* args = JPySequence::newTuple(1);
	JPySequence::setItem(args, 0, (PyObject*)co);
	Py_DECREF(co);

	PyObject* pyClass = JPyObject::call(m_GetClassMethod, args, NULL);

	return new HostRef(pyClass, false);
}

JPClass* PythonHostEnvironment::asClass(HostRef* ref)
{
	PyObject* self = UNWRAP(ref);
	PyObject* claz = JPyObject::getAttrString(self, "__javaclass__");
	PyJPClass* res = (PyJPClass*)claz;
	Py_DECREF(claz);

	return res->m_Class;
}

bool PythonHostEnvironment::isArrayClass(HostRef* ref)
{
	PyObject* self = UNWRAP(ref);

	if (! JPyType::check(self)) 
	{
		// If its not a type ... it can;t be a java type
		return false;
	}

	return JPyType::isSubclass(self, m_JavaArrayClass);
}

HostRef* PythonHostEnvironment::newArrayClass(JPArrayClass* m)
{
	PyObject* args = JPySequence::newTuple(1);

	PyObject* cname = JPyString::fromString(m->getName().getSimpleName().c_str());
	JPySequence::setItem(args, 0, cname);
	Py_DECREF(cname);

	PyObject* pyClass = JPyObject::call(m_GetArrayClassMethod, args, NULL);

	return new HostRef(pyClass, false);
}

JPArrayClass* PythonHostEnvironment::asArrayClass(HostRef* ref)
{
	PyObject* self = UNWRAP(ref);
	PyObject* claz = JPyObject::getAttrString(self, "__javaclass__");
	JPArrayClass* res = (JPArrayClass*)JPyCObject::asVoidPtr(claz);
	Py_DECREF(claz);

	return res;
}

bool PythonHostEnvironment::isArray(HostRef* ref)
{
	PyObject* obj = UNWRAP(ref);

	if (JPyObject::isInstance(obj, m_JavaArrayClass))
	{
		return true;
	}

	return false;
}

HostRef* PythonHostEnvironment::newArray(JPArray* m)
{
	JPArrayClass* jc = m->getClass();
	JPTypeName name = jc->getName();

	PyObject* args = JPySequence::newTuple(1);
	PyObject* cname = JPyString::fromString(name.getSimpleName().c_str());
	JPySequence::setItem(args, 0, cname);
	Py_DECREF(cname);

	PyObject* pyClass = JPyObject::call(m_GetArrayClassMethod, args, NULL);
	Py_DECREF(args);
	
	PyObject* joHolder = JPyCObject::fromVoidAndDesc((void*)m, (void*)"JPArray", &deleteJPArrayDestructor);
	args = JPySequence::newTuple(2);
	JPySequence::setItem(args, 0, m_SpecialConstructorKey);
	JPySequence::setItem(args, 1, joHolder);
	Py_DECREF(joHolder);

	PyObject* res = JPyObject::call(pyClass, args, NULL);
	Py_DECREF(args);

	return new HostRef(res, false);
}

JPArray* PythonHostEnvironment::asArray(HostRef* ref)
{
	PyObject* obj = UNWRAP(ref);
	PyObject* javaObject = JPyObject::getAttrString(obj, "__javaobject__");	

	JPArray* res = (JPArray*)JPyCObject::asVoidPtr(javaObject);
	
	Py_DECREF(javaObject);

	return res;
}

bool PythonHostEnvironment::isWrapper(HostRef* ref)
{
	return JPyObject::isInstance(UNWRAP(ref), m_WrapperClass);
}

bool PythonHostEnvironment::isProxy(HostRef* ref)
{
	return JPyObject::isInstance(UNWRAP(ref), m_ProxyClass);
}

JPProxy* PythonHostEnvironment::asProxy(HostRef* ref)
{
	JPCleaner cleaner;
	PyObject* proxy = UNWRAP(ref);
	PyObject* jproxy = JPyObject::getAttrString(proxy, "_proxy");
	cleaner.add(new HostRef(jproxy, false));

	JPProxy* res = (JPProxy*)JPyCObject::asVoidPtr(jproxy);
	return res;
}

HostRef* PythonHostEnvironment::getCallableFrom(HostRef* ref, string& name)
{
	JPCleaner cleaner;

	PyObject* pname = JPyString::fromString(name.c_str());
	cleaner.add(new HostRef(pname, false));
	PyObject* mname = JPyString::fromString("getCallable");
	cleaner.add(new HostRef(mname, false));

	PyObject* call = PyObject_CallMethodObjArgs(UNWRAP(ref), mname, pname, NULL);

	JPyErr::check();
	return new HostRef(call, false);

}


bool PythonHostEnvironment::isString(HostRef* ref)
{
	return JPyString::check(UNWRAP(ref));
}

jsize PythonHostEnvironment::getStringLength(HostRef* ref)
{
	return (jsize)JPyObject::length(UNWRAP(ref));
}

string PythonHostEnvironment::stringAsString(HostRef* ref)
{
	return JPyString::asString(UNWRAP(ref));
}

JCharString PythonHostEnvironment::stringAsJCharString(HostRef* ref)
{
	return JPyString::asJCharString(UNWRAP(ref));
}

HostRef* PythonHostEnvironment::newStringFromUnicode(const jchar* v, unsigned int l)
{
	TRACE_IN("PythonHostEnvironment::newStringFromUnicode");
	return new HostRef(JPyString::fromUnicode(v, l), false);
	TRACE_OUT;
}

HostRef* PythonHostEnvironment::newStringFromASCII(const char* v, unsigned int l)
{
	return new HostRef(JPyString::fromString(v), false);
}

void* PythonHostEnvironment::prepareCallbackBegin()
{
	PyGILState_STATE state = PyGILState_Ensure();;
	return (void*)new PyGILState_STATE(state);
}

void PythonHostEnvironment::prepareCallbackFinish(void* state)
{
	PyGILState_STATE* state2 = (PyGILState_STATE*)state;
	PyGILState_Release(*state2);
	delete state2;
}

HostRef* PythonHostEnvironment::callObject(HostRef* c, vector<HostRef*>& args)
{
	JPCleaner cleaner;
	PyObject* pargs = JPySequence::newTuple((int)args.size());
	cleaner.add(new HostRef(pargs, false));

	for (unsigned int i = 0; i < args.size(); i++)
	{
		JPySequence::setItem(pargs, i, UNWRAP(args[i]));
	}

	PyObject* res = JPyObject::call(UNWRAP(c), pargs, NULL);
	return new HostRef(res, false);
}

void PythonHostEnvironment::printError()
{
	PyErr_Print();
	PyErr_Clear();
}

bool PythonHostEnvironment::mapContains(HostRef* mapping, HostRef* key)
{
	return JPyDict::contains(UNWRAP(mapping), UNWRAP(key));
}

HostRef* PythonHostEnvironment::getMapItem(HostRef* mapping, HostRef* key)
{
	return new HostRef(JPyDict::getItem(UNWRAP(mapping), UNWRAP(key)), false);
}

bool PythonHostEnvironment::objectHasAttribute(HostRef* obj, HostRef* key)
{
	return JPyObject::hasAttr(UNWRAP(obj), UNWRAP(key));
}

HostRef* PythonHostEnvironment::getObjectAttribute(HostRef* obj, HostRef* key)
{
	return new HostRef(JPyObject::getAttr(UNWRAP(obj), UNWRAP(key)), false);
}

bool PythonHostEnvironment::isJavaException(HostException* ex)
{
	PythonException* pe = (PythonException*)ex;

	return JPyObject::isSubclass(pe->m_ExceptionClass, m_JavaExceptionClass);
}

HostRef* PythonHostEnvironment::getJavaException(HostException* ex)
{
	PythonException* pe = (PythonException*)ex;
	PyObject* obj = pe->getJavaException();

	PyObject* javaObject = JPyObject::getAttrString(obj, "__javaobject__");

	return new HostRef(javaObject, false);
}

void PythonHostEnvironment::clearError()
{
	PyErr_Clear();
}

bool PythonHostEnvironment::isWrapper(PyObject* obj)
{
	return JPyObject::isInstance(obj, m_WrapperClass);
}

JPTypeName PythonHostEnvironment::getWrapperTypeName(PyObject* obj)
{
	PyObject* pyTName = JPyObject::getAttrString(obj, "typeName");

	string tname = JPyString::asString(pyTName);
	Py_DECREF(pyTName);

	return JPTypeName::fromSimple(tname.c_str());
}

jvalue PythonHostEnvironment::getWrapperValue(PyObject* obj)
{
	JPTypeName name = getWrapperTypeName(obj);
	PyObject* value = JPyObject::getAttrString(obj, "_value");
	jvalue* v = (jvalue*)JPyCObject::asVoidPtr(value);
	Py_DECREF(value);

	if (name.isObjectType())
	{
		jvalue res;
		res.l = JPEnv::getJava()->NewGlobalRef(v->l);
		return res;
	}
	return *v;
}

JPTypeName PythonHostEnvironment::getWrapperTypeName(HostRef* obj)
{
	return getWrapperTypeName(UNWRAP(obj));
}

jvalue PythonHostEnvironment::getWrapperValue(HostRef* obj)
{
	return getWrapperValue(UNWRAP(obj));
}

PyObject* PythonHostEnvironment::getJavaShadowClass(JPClass* jc)
{
	PyJPClass* cls = PyJPClass::alloc(jc);
	PyObject* args = JPySequence::newTuple(1);
	JPySequence::setItem(args, 0, (PyObject*)cls);
	Py_DECREF(cls);

	PyObject* res = JPyObject::call(m_GetClassMethod, args, NULL);
	Py_DECREF(args);

	return res;

}

bool PythonHostEnvironment::isByteString(HostRef* ref)
{
	PyObject* obj = UNWRAP(ref);
	return JPyString::checkStrict(obj);
}

bool PythonHostEnvironment::isUnicodeString(HostRef* ref)
{
	return JPyString::checkUnicode(UNWRAP(ref));
}

void PythonHostEnvironment::getRawByteString(HostRef* obj, char** outBuffer, long& outSize)
{
	PyObject* objRef = UNWRAP(obj);
	Py_ssize_t tempSize = 0;
	JPyString::AsStringAndSize(objRef, outBuffer, &tempSize);
	outSize = (long)tempSize;
}

void PythonHostEnvironment::getRawUnicodeString(HostRef* obj, jchar** outBuffer, long& outSize)
{
	PyObject* objRef = UNWRAP(obj);
	outSize = (long)JPyObject::length(objRef);
	*outBuffer = (jchar*)JPyString::AsUnicode(objRef);
}

size_t PythonHostEnvironment::getUnicodeSize()
{
	return sizeof(Py_UNICODE);
}

HostRef* PythonHostEnvironment::newStringWrapper(jstring jstr)
{
	TRACE_IN("PythonHostEnvironment::newStringWrapper");
	jvalue* v = new jvalue;
	v->l = jstr;
	PyObject* value = JPyCObject::fromVoidAndDesc((void*)v, (void*)"object jvalue", deleteObjectJValueDestructor);

	PyObject* args = JPySequence::newTuple(1);
	JPySequence::setItem(args, 0, Py_None);

	PyObject* res = JPyObject::call(m_StringWrapperClass, args, Py_None);
	Py_DECREF(args);

	JPyObject::setAttrString(res, "_value", value);
	Py_DECREF(value);

	HostRef* resRef = new HostRef(res);
	Py_DECREF(res);

	return resRef;
	TRACE_OUT;
}

void PythonHostEnvironment::printReferenceInfo(HostRef* obj)
{
	PyObject* pobj = UNWRAP(obj);
	cout << "Object info report" << endl;
	cout << "    obj type " << pobj->ob_type->tp_name << endl;
	cout << "    Ref count " << (long)pobj->ob_refcnt << endl;
}

void PythonHostEnvironment::deleteJPObjectDestructor(void* data, void* desc)
{
	delete (JPObject*)data;
}
