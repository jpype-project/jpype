/*****************************************************************************
   Copyright 2004-2008 Steve M�nard

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
PyObject* JPypeJavaNio::convertToDirectBuffer(PyObject* self, PyObject* args)
{  
	TRACE_IN("convertStringToBuffer"); 

	// Use special method defined on the TypeConverter interface ...
	PyObject* src;

	JPyArg::parseTuple(args, "O", &src);

	PyObject* res = NULL;
	if (JPyString::checkStrict(src))
	{
		// converts to byte buffer ...
		JPTypeName tname = JPTypeName::fromType(JPTypeName::_byte);
		JPType* type = JPTypeManager::getType(tname);
		HostRef srcRef(src);

		TRACE1("Converting");
		HostRef* ref = type->convertToDirectBuffer(&srcRef);
		JPEnv::registerRef(ref, &srcRef);

		TRACE1("detaching result");
		res = detachRef(ref);
	}

	if (res != NULL)
	{
		return res;
	}

	RAISE(JPypeException, "Do not know how to convert to Direct Buffer");

	return NULL;
	TRACE_OUT;
}

