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

static PyMethodDef classMethods[] = {
  {"getName",              &PyJPClass::getName, METH_VARARGS, ""},
  {"getBaseClass",         &PyJPClass::getBaseClass, METH_VARARGS, ""},
  {"getClassFields",       &PyJPClass::getClassFields, METH_VARARGS, ""},
  {"getClassMethods",      &PyJPClass::getClassMethods, METH_VARARGS, ""},
  {"newClassInstance",     &PyJPClass::newClassInstance, METH_VARARGS, ""},

  {"isInterface", &PyJPClass::isInterface, METH_VARARGS, ""},
  {"getBaseInterfaces", &PyJPClass::getBaseInterfaces, METH_VARARGS, ""},
  {"isSubclass", &PyJPClass::isSubclass, METH_VARARGS, ""},
  {"isPrimitive", &PyJPClass::isPrimitive, METH_VARARGS, ""},

  {"isException", &PyJPClass::isException, METH_VARARGS, ""},
  {"isArray", &PyJPClass::isArray, METH_VARARGS, ""},
  {"getSuperclass",&PyJPClass::getBaseClass, METH_NOARGS, ""},

  {"getConstructors", (PyCFunction)&PyJPClass::getConstructors, METH_NOARGS, ""},
  {"getDeclaredConstructors", (PyCFunction)&PyJPClass::getDeclaredConstructors, METH_NOARGS, ""},
  {"getDeclaredFields", (PyCFunction)&PyJPClass::getDeclaredFields, METH_NOARGS, ""},
  {"getDeclaredMethods", (PyCFunction)&PyJPClass::getDeclaredMethods, METH_NOARGS, ""},
  {"getFields", (PyCFunction)&PyJPClass::getFields, METH_NOARGS, ""},
  {"getMethods", (PyCFunction)&PyJPClass::getMethods, METH_NOARGS, ""},
  {"getModifiers", (PyCFunction)&PyJPClass::getModifiers, METH_NOARGS, ""},

  {NULL},
};

static PyTypeObject classClassType = 
{
	PyObject_HEAD_INIT(&PyType_Type)
	0,                         /*ob_size*/
	"JavaClass",              /*tp_name*/
	sizeof(PyJPClass),      /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	PyJPClass::__dealloc__,                   /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"Java Class",                  /*tp_doc */
	0,		                   /* tp_traverse */
	0,		                   /* tp_clear */
	0,		                   /* tp_richcompare */
	0,		                   /* tp_weaklistoffset */
	0,		                   /* tp_iter */
	0,		                   /* tp_iternext */
	classMethods,                   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	0,                         /* tp_init */
	0,                         /* tp_alloc */
	PyType_GenericNew          /* tp_new */

};

// Static methods
void PyJPClass::initType(PyObject* module)
{
	PyType_Ready(&classClassType);
	PyModule_AddObject(module, "_JavaClass", (PyObject*)&classClassType); 
}

PyJPClass* PyJPClass::alloc(JPClass* cls)
{
	PyJPClass* res = PyObject_New(PyJPClass, &classClassType);

	res->m_Class = cls;
	
	return res;
}

void PyJPClass::__dealloc__(PyObject* o)
{
	TRACE_IN("PyJPClass::__dealloc__");

	PyJPClass* self = (PyJPClass*)o;

	self->ob_type->tp_free(o);

	TRACE_OUT;
}

PyObject* PyJPClass::getName(PyObject* o, PyObject* arg)
{
	try {
		PyJPClass* self = (PyJPClass*)o;

		JPTypeName name = self->m_Class->getName();

		PyObject* res = JPyString::fromString(name.getSimpleName().c_str());

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getBaseClass(PyObject* o, PyObject* arg)
{
	try {
		PyJPClass* self = (PyJPClass*)o;

		JPClass* base = self->m_Class->getSuperClass();
		if (base == NULL)
		{
			Py_INCREF(Py_None);
			return Py_None;
		}

		PyObject* res  = (PyObject*)PyJPClass::alloc(base);

		return res;

	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getBaseInterfaces(PyObject* o, PyObject* arg)
{
	try {
		PyJPClass* self = (PyJPClass*)o;

		vector<JPClass*> baseItf = self->m_Class->getInterfaces();

		PyObject* result = JPySequence::newTuple((int)baseItf.size());
		for (unsigned int i = 0; i < baseItf.size(); i++)
		{
			JPClass* base = baseItf[i];
			PyObject* obj = (PyObject*)PyJPClass::alloc(base);
			JPySequence::setItem(result, i, obj);
		}

		return result;

	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getClassFields(PyObject* o, PyObject* arg)
{
	try {
		PyJPClass* self = (PyJPClass*)o;

		map<string, JPField*> staticFields = self->m_Class->getStaticFields();
		map<string, JPField*> instFields = self->m_Class->getInstanceFields();

		PyObject* res = JPySequence::newTuple((int)(staticFields.size()+instFields.size()));

		int i = 0;
		for (map<string, JPField*>::iterator curStatic = staticFields.begin(); curStatic != staticFields.end(); curStatic ++)
		{
			PyObject* f = (PyObject*)PyJPField::alloc(curStatic->second);

			JPySequence::setItem(res, i, f);
			i++;
			Py_DECREF(f);
		}

		for (map<string, JPField*>::iterator curInst = instFields.begin(); curInst != instFields.end(); curInst ++)
		{
			PyObject* f = (PyObject*)PyJPField::alloc(curInst->second);

			JPySequence::setItem(res, i, f);
			i++;
			Py_DECREF(f);
		}


		return res;

	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getClassMethods(PyObject* o, PyObject* arg)
{
	try {
		PyJPClass* self = (PyJPClass*)o;

		vector<JPMethod*> methods = self->m_Class->getMethods();

		PyObject* res = JPySequence::newTuple((int)methods.size());

		int i = 0;
		for (vector<JPMethod*>::iterator curMethod = methods.begin(); curMethod != methods.end(); curMethod ++)
		{

			JPMethod* mth= *curMethod;
			PyJPMethod* methObj = PyJPMethod::alloc(mth);
			
			JPySequence::setItem(res, i, (PyObject*)methObj);
			i++;
			Py_DECREF(methObj);
		}

		return res;

	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::newClassInstance(PyObject* o, PyObject* arg)
{
	try {
		PyJPClass* self = (PyJPClass*)o;
		JPCleaner cleaner;

		//cout << "Creating a new " << self->m_Class->getName().getSimpleName() << endl;
		//JPyHelper::dumpSequenceRefs(arg, "Start");

		vector<HostRef*> args;
		Py_ssize_t len = JPyObject::length(arg);
		for (Py_ssize_t i = 0; i < len; i++)
		{
			PyObject* obj = JPySequence::getItem(arg, i);
			HostRef* ref = new HostRef((void*)obj);
			cleaner.add(ref);
			args.push_back(ref);
			Py_DECREF(obj);
		}

		JPObject* resObject = self->m_Class->newInstance(args);
		PyObject* res = JPyCObject::fromVoidAndDesc((void*)resObject, (void*)"JPObject", &PythonHostEnvironment::deleteJPObjectDestructor);

		//JPyHelper::dumpSequenceRefs(arg, "End");
		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::isInterface(PyObject* o, PyObject* arg)
{
	try {
		PyJPClass* self = (PyJPClass*)o;

		if (self->m_Class->isInterface())
		{
			return JPyBoolean::getTrue();
		}
		return JPyBoolean::getFalse();
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::isSubclass(PyObject* o, PyObject* arg)
{
	try {
		PyJPClass* self = (PyJPClass*)o;
		char* other;

		JPyArg::parseTuple(arg, "s", &other);
		JPTypeName name = JPTypeName::fromSimple(other);
		JPClass* otherClass = JPTypeManager::findClass(name);

		if (self->m_Class->isSubclass(otherClass))
		{
			return JPyBoolean::getTrue();
		}
		return JPyBoolean::getFalse();
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::isException(PyObject* o, PyObject* args)
{
	try 
	{
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;

		bool res = JPJni::isThrowable(self->m_Class->getClass());
		if (res)
		{
			return JPyBoolean::getTrue();
		}
		return JPyBoolean::getFalse();

	}
	PY_STANDARD_CATCH;
	return NULL;
}

bool PyJPClass::check(PyObject* o)
{
	return o->ob_type == &classClassType;
}

PyObject* PyJPClass::getDeclaredMethods(PyObject* o)
{
	try {
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getDeclaredMethods(self->m_Class->getClass());

		PyObject* res = JPySequence::newTuple((int)mth.size());
		JPTypeName methodClassName = JPTypeName::fromSimple("java.lang.reflect.Method");
		JPClass* methodClass = JPTypeManager::findClass(methodClassName);
		for (unsigned int i = 0; i < mth.size(); i++)
		{
			jvalue v;
			v.l = mth[i];
			HostRef* ref = methodClass->asHostObject(v);
			cleaner.add(ref);
			JPySequence::setItem(res, i, (PyObject*)ref->data());
		}

		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getConstructors(PyObject* o)
{
	try {
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getConstructors(self->m_Class->getClass());

		PyObject* res = JPySequence::newTuple((int)mth.size());
		JPTypeName methodClassName = JPTypeName::fromSimple("java.lang.reflect.Method");
		JPClass* methodClass = JPTypeManager::findClass(methodClassName);
		for (unsigned int i = 0; i < mth.size(); i++)
		{
			jvalue v;
			v.l = mth[i];
			HostRef* ref = methodClass->asHostObject(v);
			cleaner.add(ref);
			JPySequence::setItem(res, i, (PyObject*)ref->data());
		}

		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getDeclaredConstructors(PyObject* o)
{
	try {
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getDeclaredConstructors(self->m_Class->getClass());

		PyObject* res = JPySequence::newTuple((int)mth.size());
		JPTypeName methodClassName = JPTypeName::fromSimple("java.lang.reflect.Method");
		JPClass* methodClass = JPTypeManager::findClass(methodClassName);
		for (unsigned int i = 0; i < mth.size(); i++)
		{
			jvalue v;
			v.l = mth[i];
			HostRef* ref = methodClass->asHostObject(v);
			cleaner.add(ref);
			JPySequence::setItem(res, i, (PyObject*)ref->data());
		}

		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getDeclaredFields(PyObject* o)
{
	try {
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getDeclaredFields(self->m_Class->getClass());

		PyObject* res = JPySequence::newTuple((int)mth.size());
		JPTypeName fieldClassName = JPTypeName::fromSimple("java.lang.reflect.Field");
		JPClass* fieldClass = JPTypeManager::findClass(fieldClassName);
		for (unsigned int i = 0; i < mth.size(); i++)
		{
			jvalue v;
			v.l = mth[i];
			HostRef* ref = fieldClass->asHostObject(v);
			cleaner.add(ref);
			JPySequence::setItem(res, i, (PyObject*)ref->data());
		}

		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getFields(PyObject* o)
{
	try {
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getFields(self->m_Class->getClass());

		PyObject* res = JPySequence::newTuple((int)mth.size());
		JPTypeName fieldClassName = JPTypeName::fromSimple("java.lang.reflect.Field");
		JPClass* fieldClass = JPTypeManager::findClass(fieldClassName);
		for (unsigned int i = 0; i < mth.size(); i++)
		{
			jvalue v;
			v.l = mth[i];
			HostRef* ref = fieldClass->asHostObject(v);
			cleaner.add(ref);
			JPySequence::setItem(res, i, (PyObject*)ref->data());
		}

		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getModifiers(PyObject* o)
{
	try {
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;
		long mod = JPJni::getClassModifiers(self->m_Class->getClass());

		PyObject* res = JPyLong::fromLongLong(mod);

		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getMethods(PyObject* o)
{
	try {
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getMethods(self->m_Class->getClass());

		PyObject* res = JPySequence::newTuple((int)mth.size());
		JPTypeName methodClassName = JPTypeName::fromSimple("java.lang.reflect.Method");
		JPClass* methodClass = JPTypeManager::findClass(methodClassName);
		for (unsigned int i = 0; i < mth.size(); i++)
		{
			jvalue v;
			v.l = mth[i];
			HostRef* ref = methodClass->asHostObject(v);
			cleaner.add(ref);
			JPySequence::setItem(res, i, (PyObject*)ref->data());
		}

		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;
	
}

PyObject* PyJPClass::isPrimitive(PyObject* o, PyObject* args)
{
	try {
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;

		JPTypeName name = self->m_Class->getName();
		if (name.isObjectType())
		{
			return JPyBoolean::getFalse();
		}
		return JPyBoolean::getTrue();
	}
	PY_STANDARD_CATCH;
	return NULL;
	
}

PyObject* PyJPClass::isArray(PyObject* o, PyObject* args)
{
	try {
		JPCleaner cleaner;
		PyJPClass* self = (PyJPClass*)o;

		JPTypeName name = self->m_Class->getName();
		char c = name.getNativeName()[0];
		if (c == '[')
		{
			return JPyBoolean::getTrue();
		}
		return JPyBoolean::getFalse();
	}
	PY_STANDARD_CATCH;
	return NULL;
	
}
