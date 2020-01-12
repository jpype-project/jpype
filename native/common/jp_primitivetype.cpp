/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#include <jpype.h>

JPPrimitiveType::JPPrimitiveType(const string& name)
: JPClass(name, 0x411)
{
}

JPPrimitiveType::~JPPrimitiveType()
{
}

bool JPPrimitiveType::isPrimitive() const
{
	return true;
}

bool JPPrimitiveType::isAssignableFrom(JPClass* o)
{
	return this == o;
}

JPValue JPPrimitiveType::newInstance(JPJavaFrame& frame, JPPyObjectVector& args)
{
	if (args.size() != 1)
		JP_RAISE(PyExc_TypeError, "primitives take 1 positional argument");

	// Test the conversion
	JPMatch match;
	getJavaConversion(NULL, match, args[0]);

	// If there is no conversion report a failure
	if (match.type == JPMatch::_none)
		JP_RAISE(PyExc_TypeError, "Unable to create an instance.");

	// Otherwise give back a PyJPValue
	return JPValue(this, match.conversion->convert(&frame, this, args[0]));
}
