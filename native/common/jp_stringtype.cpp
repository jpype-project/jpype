/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include "jpype.h"
#include "pyjp.h"
#include "jp_stringtype.h"

JPStringType::JPStringType(JPJavaFrame& frame,
		jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
}

JPStringType::~JPStringType()
= default;

JPPyObject JPStringType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	JP_TRACE_IN("JPStringType::asHostObject");
	JPContext *context = frame.getContext();

	if (!cast)
	{
		// This loses type
		if (val.l == nullptr)
		{
			return JPPyObject::getNone();
		}

		if (context->getConvertStrings())
		{
			string str = frame.toStringUTF8((jstring) (val.l));
			return JPPyObject::call(PyUnicode_FromStringAndSize(str.c_str(), str.length()));
		}
	}

	return JPClass::convertToPythonObject(frame, val, cast);
	JP_TRACE_OUT; // GCOV_EXCL_LINE
}

JPMatch::Type JPStringType::findJavaConversion(JPMatch& match)
{
	JP_TRACE_IN("JPStringType::findJavaConversion");
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| stringConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	return match.type = JPMatch::_none;
	JP_TRACE_OUT; // GCOV_EXCL_LINE
}

void JPStringType::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	JPContext* context = frame.getContext();
	objectConversion->getInfo(this, info);
	stringConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	if (context->getConvertStrings())
		PyList_Append(info.ret, (PyObject*) & PyUnicode_Type); // GCOVR_EXCL_LINE
	else
		PyList_Append(info.ret, (PyObject*) getHost());
}

JPValue JPStringType::newInstance(JPJavaFrame& frame, JPPyObjectVector& args)
{
	JP_TRACE_IN("JPStringType::newInstance");
	if (args.size() == 1 && JPPyString::check(args[0]))
	{
		// JNI has a short cut for constructing java.lang.String
		JP_TRACE("Direct");
		string str = JPPyString::asStringUTF8(args[0]);
		return JPValue(this, frame.fromStringUTF8(str));
	}
	return JPClass::newInstance(frame, args);
	JP_TRACE_OUT; // GCOV_EXCL_LINE
}
