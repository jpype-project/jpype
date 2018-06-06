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

PyObject* JPypeJavaClass::findClass(PyObject* obj, PyObject* args)
{
	TRACE_IN("JPypeModule::findClass");
	try {
		ASSERT_JVM_RUNNING("JPypeJavaClass::findClass");
		JPJavaFrame frame;
		char* cname;
		JPyArg::parseTuple(args, "s", &cname);
		TRACE1(cname);

		JPTypeName name = JPTypeName::fromSimple(cname);

		JPClass* claz = JPTypeManager::findClass(name);
		if (claz == NULL)
		{
			Py_RETURN_NONE;
		}

		PyObject* res = (PyObject*)PyJPClass::alloc(claz);

		return res;
	}
	PY_STANDARD_CATCH;  

	PyErr_Clear();

	Py_RETURN_NONE;

	TRACE_OUT;
}


