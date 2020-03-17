#include "jpype.h"
#include "pyjp.h"
#include "jp_numbertype.h"

JPNumberType::JPNumberType(JPJavaFrame& frame,
		jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
}

JPNumberType::~JPNumberType()
{
}


JPMatch::Type JPNumberType::findJavaConversion(JPMatch& match)
{
	// Rules for java.lang.Object
	JP_TRACE_IN("JPNumberType::canConvertToJava");
	if (nullConversion->matches(match, this)
			|| javaNumberAnyConversion->matches(match, this)
			|| boxLongConversion->matches(match, this)
			|| boxDoubleConversion->matches(match, this)
			|| hintsConversion->matches(match, this)
			)
		return match.type;
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}
