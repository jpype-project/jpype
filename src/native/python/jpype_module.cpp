/*****************************************************************************
   Copyright 2004 Steve M�nard

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
PyObject* JPypeModule::startup(PyObject* obj, PyObject* args)  
{  
	TRACE_IN("startup");
	try {
		PyObject* vmOpt;
		PyObject* vmPath;
		char ignoreUnrecognized = true;

		JPyArg::parseTuple(args, "OO!b|", &vmPath, &PyTuple_Type, &vmOpt, &ignoreUnrecognized);

		if (! (JPyString::check(vmPath)))
		{
			RAISE(JPypeException, "First paramter must be a string or unicode");
		}

		string cVmPath = JPyString::asString(vmPath);

		StringVector args;

		for (int i = 0; i < JPyObject::length(vmOpt); i++)
		{
			PyObject* obj = JPySequence::getItem(vmOpt, i);

			if (JPyString::check(obj))
			{
				// TODO support unicode
				string v = JPyString::asString(obj);	

				args.push_back(v);
			}
			else if (JPySequence::check(obj))
			{
				//String name = arg[0];
				//Callable value = arg[1];

				// TODO complete this for the hooks ....
			}
			else {
				RAISE(JPypeException, "VM Arguments must be string or tuple");
			}
		}

		JPEnv::loadJVM(cVmPath, ignoreUnrecognized, args);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
	TRACE_OUT;
}

PyObject* JPypeModule::attach(PyObject* obj, PyObject* args)  
{  
	TRACE_IN("attach");
	try {
		PyObject* vmPath;

		JPyArg::parseTuple(args, "O", &vmPath);

		if (! (JPyString::check(vmPath)))
		{
			RAISE(JPypeException, "First paramter must be a string or unicode");
		}

		string cVmPath = JPyString::asString(vmPath);
		JPEnv::attachJVM(cVmPath);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
	TRACE_OUT;
}

PyObject* JPypeModule::dumpJVMStats(PyObject* obj)   
{
 	cerr << "JVM activity report     :" << endl;
	//cerr << "\tmethod calls         : " << methodCalls << endl;
	//cerr << "\tstatic method calls  : " << staticMethodCalls << endl;
	//cerr << "\tconstructor calls    : " << constructorCalls << endl;
	//cerr << "\tproxy callbacks      : " << JProxy::getCallbackCount() << endl;
	//cerr << "\tfield gets           : " << fieldGets << endl;
	//cerr << "\tfield sets           : " << fieldSets << endl;
	cerr << "\tclasses loaded       : " << JPTypeManager::getLoadedClasses() << endl;

	Py_INCREF(Py_None);	
	return Py_None;
}

PyObject* JPypeModule::shutdown(PyObject* obj)
{
	TRACE_IN("shutdown");
	try {
		dumpJVMStats(obj);

		JPEnv::getJava()->checkInitialized();

		JPTypeManager::flushCache();

		if (JPEnv::getJava()->DestroyJavaVM() )
		{
			RAISE(JPypeException, "Unable to destroy JVM");
		}

		JPEnv::getJava()->shutdown();
		cerr << "JVM has been shutdown" << endl;
		
		Py_INCREF(Py_None);		
		return Py_None;
	}
	PY_STANDARD_CATCH;

	return NULL;
	TRACE_OUT;
}

PyObject* JPypeModule::synchronized(PyObject* obj, PyObject* args)
{
	JPCleaner cleaner;
	TRACE_IN("synchronized");
	try {
		PyObject* o;
		
		JPyArg::parseTuple(args, "O!", &PyCObject_Type, &o);
		string desc = (char*)JPyCObject::getDesc(o);

		jobject obj;
		if (desc == "JPObject")
		{
			JPObject* jpo = (JPObject*)JPyCObject::asVoidPtr(o);
			obj = jpo->getObject();
			cleaner.addLocal(obj);
		}
		else if (desc == "JPClass")
		{
			JPClass* jpo = (JPClass*)JPyCObject::asVoidPtr(o);
			obj = jpo->getClass();
			cleaner.addLocal(obj);
		}
		else if (desc == "JPArray")
		{
			JPArray* jpo = (JPArray*)JPyCObject::asVoidPtr(o);
			obj = jpo->getObject();
			cleaner.addLocal(obj);
		}
		else if (desc == "JPArrayClass")
		{
			JPArrayClass* jpo = (JPArrayClass*)JPyCObject::asVoidPtr(o);
			obj = jpo->getClass();
			cleaner.addLocal(obj);
		}
		else if (hostEnv->isWrapper(o) && hostEnv->getWrapperTypeName(o).isObjectType())
		{
			obj = hostEnv->getWrapperValue(o).l;
			cleaner.addLocal(obj);
		}
		// TODO proxy		
		else 
		{
			RAISE(JPypeException, "method only accepts object values.");
		}

		PyJPMonitor* c = PyJPMonitor::alloc(new JPMonitor(obj));

		return (PyObject*)c;
	}
	PY_STANDARD_CATCH;

	PyErr_Clear();

	Py_INCREF(Py_None);
	return Py_None;

	TRACE_OUT;
}

PyObject* JPypeModule::isStarted(PyObject* obj)
{
	if (JPEnv::isInitialized())
	{
		return JPyBoolean::getTrue();
	}
	return JPyBoolean::getFalse();
}

PyObject* JPypeModule::attachThread(PyObject* obj)
{
	try {
		JPEnv::attachCurrentThread();
		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH;

	return NULL;
}

PyObject* JPypeModule::detachThread(PyObject* obj)
{
	try {
		JPEnv::getJava()->DetachCurrentThread();
		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH;

	return NULL;
}

PyObject* JPypeModule::isThreadAttached(PyObject* obj)
{
	try {
		if (JPEnv::isThreadAttached())
	{
		return JPyBoolean::getTrue();
	}
	return JPyBoolean::getFalse();
	}
	PY_STANDARD_CATCH;

	return NULL;

}

PyObject* JPypeModule::raiseJava(PyObject* , PyObject* args)
{
	try 
	{
		//PyObject* arg;
		//JPyArg::parseTuple(args, "O", &arg);
		//JPObject* obj;
		//JPCleaner cleaner;
		//
		//if (JPyCObject::check(arg) && string((char*)JPyCObject::getDesc(arg)) == "JPObject")
		//{
		//	obj = (JPObject*)JPyCObject::asVoidPtr(arg);
		//}
		//else
		//{
		//	JPyErr::setString(PyExc_TypeError, "You can only throw a subclass of java.lang.Throwable");
		//	return NULL;
		//}
		//
		//// check if claz is an instance of Throwable
		//JPClass* claz = obj->getClass();
		//jclass jc = claz->getClass();
		//cleaner.add(jc);
		//if (! JPJni::isThrowable(jc))
		//{
		//	JPyErr::setString(PyExc_TypeError, "You can only throw a subclass of java.lang.Throwable");
		//	return NULL;
		//}
		//
		//jobject jobj = obj->getObject();
		//cleaner.add(jobj);
		//JPEnv::getJava()->Throw((jthrowable)jobj);

		//PyJavaException::errorOccurred();
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* JPypeModule::attachThreadAsDaemon(PyObject* obj)
{
	try {
		JPEnv::attachCurrentThreadAsDaemon();

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH;

	return NULL;
}


PyObject* JPypeModule::startReferenceQueue(PyObject* obj, PyObject* args)
{
	try {
		int i;
		JPyArg::parseTuple(args, "i", &i);

		JPJni::startJPypeReferenceQueue(i == 1);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH;

	return NULL;
}

PyObject* JPypeModule::stopReferenceQueue(PyObject* obj)
{
	try {
		JPJni::stopJPypeReferenceQueue();

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH;

	return NULL;
}


PyObject* JPypeModule::setConvertStringObjects(PyObject* obj, PyObject* args)
{
	try {
		PyObject* flag;
		JPyArg::parseTuple(args, "O", &flag);

		if (JPyBoolean::isTrue(flag))
		{
			JPEnv::getJava()->setConvertStringObjects(true);
		}
		else
		{
			JPEnv::getJava()->setConvertStringObjects(false);
		}

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH;

	return NULL;
}
