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
	if (nullConversion->matches(this, match)
			|| javaNumberAnyConversion->matches(this, match)
			|| boxLongConversion->matches(this, match)
			|| boxDoubleConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPNumberType::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer(m_Context);
	javaNumberAnyConversion->getInfo(this, info);
	boxLongConversion->getInfo(this, info);
	boxDoubleConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}