/*****************************************************************************
   Copyright 2004 Steve Menard

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
#include "jpype.h"
#include "jp_arrayclass.h"
#include "jp_boxedtype.h"
#include "jp_method.h"
#include "pyjp.h"

JPMethod::JPMethod(JPJavaFrame& frame,
		JPClass* claz,
		const string& name,
		jobject mth,
		jmethodID mid,
		JPMethodList& moreSpecific,
		jint modifiers)
: m_Method(frame, mth)
{
	m_Class = claz;
	m_Name = name;
	m_MethodID = mid;
	m_MoreSpecificOverloads = moreSpecific;
	m_Modifiers = modifiers;
	m_ReturnType = (JPClass*) (-1);
}

JPMethod::~JPMethod()
{
}

void JPMethod::setParameters(
		JPClass *returnType,
		JPClassList parameterTypes)
{
	m_ReturnType = returnType;
	m_ParameterTypes = parameterTypes;
}

string JPMethod::toString() const
{
	return m_Name;
}

JPMatch::Type matchVars(JPJavaFrame &frame, JPMethodMatch& match, JPPyObjectVector &arg, size_t start, JPClass *vartype)
{
	JP_TRACE_IN("JPMethod::matchVars");
	JPArrayClass *arraytype = (JPArrayClass*) vartype;
	JPClass *type = arraytype->getComponentType();
	size_t len = arg.size();

	JPMatch::Type lastMatch = JPMatch::_exact;
	for (size_t i = start; i < len; i++)
	{
		JPMatch::Type quality = type->getJavaConversion(&frame, match[i], arg[i]);

		if (quality < JPMatch::_implicit)
		{
			return JPMatch::_none;
		}
		if (quality < lastMatch)
		{
			lastMatch = quality;
		}
	}

	return lastMatch;
	JP_TRACE_OUT;
}

JPMatch::Type JPMethod::matches(JPJavaFrame &frame, JPMethodMatch& match, bool callInstance,
		JPPyObjectVector& arg)
{
	ensureTypeCache();

	JP_TRACE_IN("JPMethod::matches");
	match.overload = this;
	match.offset = 0;
	match.skip = 0;
	match.isVarIndirect = false;
	match.type = JPMatch::_exact;

	size_t len = arg.size();
	size_t tlen = m_ParameterTypes.size();
	JP_TRACE("Flags", isStatic(), isInstance(), callInstance);
	JP_TRACE("arguments length", len);
	JP_TRACE("types length", tlen);
	if (callInstance && isStatic())
	{
		JP_TRACE("Skip this");
		len--;
		match.offset = 1;
	}

	if (callInstance || isInstance())
	{
		JP_TRACE("Take this");
		match.skip = 1;
	}

	if (!JPModifier::isVarArgs(m_Modifiers))
	{
		if (len != tlen)
		{
			JP_TRACE("Argument length mismatch", len, tlen);
			return match.type = JPMatch::_none;
		}
	} else
	{
		JP_TRACE("Match vargs");
		match.type = JPMatch::_none;
		if (len < tlen - 1)
		{
			return match.type;
		}

		JPClass* type = m_ParameterTypes[tlen - 1];
		// Hard, could be direct array or an array.
		if (len == tlen)
		{
			// Try direct
			size_t last = tlen - 1 - match.offset;
			match.type = type->getJavaConversion(&frame, match.argument[last], arg[last]);
			JP_TRACE("Direct vargs", match.type);
		}

		if (match.type < JPMatch::_implicit && len >= tlen)
		{
			// Must match the array type
			match.type = matchVars(frame, match, arg, tlen - 1 + match.offset, type);
			match.isVarIndirect = true;
			JP_TRACE("Indirect vargs", match.type);
		} else if (len < tlen)
		{
			match.isVarIndirect = true;
			match.type = JPMatch::_exact;
			JP_TRACE("Empty vargs");
		}
		len = tlen - 1;

		if (match.type < JPMatch::_implicit)
		{
			return match.type;
		}
	}

	JP_TRACE("Match args");
	for (size_t i = 0; i < len; i++)
	{
		size_t j = i + match.offset;
		JPClass *type = m_ParameterTypes[i];
		JP_TRACE("Compare", i, j, type->toString(), JPPyObject::getTypeName(arg[j]));
		JPMatch::Type ematch = type->getJavaConversion(&frame, match.argument[j], arg[j]);
		JP_TRACE("Result", ematch);
		if (ematch < match.type)
		{
			match.type = ematch;
		}
		if (match.type < JPMatch::_implicit)
		{
			return match.type;
		}
	}

	return match.type;
	JP_TRACE_OUT;
}

void JPMethod::packArgs(JPJavaFrame &frame, JPMethodMatch &match,
		vector<jvalue> &v, JPPyObjectVector &arg)
{
	JP_TRACE_IN("JPMethod::packArgs");
	size_t len = arg.size();
	size_t tlen = m_ParameterTypes.size();
	JP_TRACE("skip", match.skip == 1);
	JP_TRACE("offset", match.offset == 1);
	JP_TRACE("arguments length", len);
	JP_TRACE("types length", tlen);
	if (match.isVarIndirect)
	{
		JP_TRACE("Pack indirect varargs");
		len = tlen - 1;
		JPArrayClass* type = (JPArrayClass*) m_ParameterTypes[tlen - 1];
		v[tlen - 1 - match.skip] = type->convertToJavaVector(frame, arg, (jsize) tlen - 1, (jsize) arg.size());
	}

	JP_TRACE("Pack fixed total=", len - match.offset);
	for (size_t i = match.skip; i < len; i++)
	{
		JPClass* type = m_ParameterTypes[i - match.offset];
		JPConversion *conversion = match.argument[i].conversion;
		JP_TRACE("Convert", i - match.offset, i, type->getCanonicalName(), conversion);
		if (conversion == NULL)
			JP_RAISE(PyExc_RuntimeError, "Conversion is null");
		v[i - match.skip] = conversion->convert(&frame, type, arg[i]);
	}
	JP_TRACE_OUT;
}

JPPyObject JPMethod::invoke(JPJavaFrame& frame, JPMethodMatch& match, JPPyObjectVector& arg, bool instance)
{
	JP_TRACE_IN("JPMethod::invoke");
	// Check if it is caller sensitive
	if (isCallerSensitive())
		return invokeCallerSensitive(match, arg, instance);

	JPContext *context = m_Class->getContext();
	size_t alen = m_ParameterTypes.size();
	JPClass* retType = m_ReturnType;

	// Pack the arguments
	vector<jvalue> v(alen + 1);
	packArgs(frame, match, v, arg);

	// Invoke the method (arg[0] = this)
	if (JPModifier::isStatic(m_Modifiers))
	{
		JP_TRACE("invoke static", m_Name);
		jclass claz = m_Class->getJavaClass();
		return retType->invokeStatic(frame, claz, m_MethodID, &v[0]);
	} else
	{
		JPValue* selfObj = PyJPValue_getJavaSlot(arg[0]);
		jobject c;
		if (selfObj == NULL)
		{
			// This only can be hit by calling an instance method as a
			// class object.  We already know it is safe to convert.
			jvalue  v = match.argument[0].conversion->convert(&frame, m_Class, arg[0]);
			c = v.l;
		} else
		{
			c = selfObj->getJavaObject();
		}
		jclass clazz = NULL;
		if (!isAbstract() && !instance)
		{
			clazz = m_Class->getJavaClass();
			JP_TRACE("invoke nonvirtual", m_Name);
		} else
		{
			JP_TRACE("invoke virtual", m_Name);
		}
		return retType->invoke(frame, c, clazz, m_MethodID, &v[0]);
	}
	JP_TRACE_OUT;
}

JPPyObject JPMethod::invokeCallerSensitive(JPMethodMatch& match, JPPyObjectVector& arg, bool instance)
{
	JP_TRACE_IN("JPMethod::invokeCallerSensitive");
	JPContext *context = m_Class->getContext();
	size_t alen = m_ParameterTypes.size();
	JPJavaFrame frame(context, (int) (8 + alen));
	JPClass* retType = m_ReturnType;

	// Pack the arguments
	vector<jvalue> v(alen + 1);
	packArgs(frame, match, v, arg);

	//Proxy the call to
	//   public static Object callMethod(Method method, Object obj, Object[] args)
	jobject self = NULL;
	size_t len = alen;
	if (!isStatic())
	{
		JP_TRACE("Call instance");
		len--;
		JPValue *selfObj = PyJPValue_getJavaSlot(arg[0]);
		if (selfObj == NULL)
			JP_RAISE(PyExc_RuntimeError, "Null object");
		self = selfObj->getJavaObject();
	}

	// Convert arguments
	jobjectArray ja = frame.NewObjectArray((jsize) len, context->_java_lang_Object->getJavaClass(), NULL);
	for (jsize i = 0; i < (jsize) len; ++i)
	{
		JPClass *cls = m_ParameterTypes[i + match.skip - match.offset];
		if (cls->isPrimitive())
		{
			JPPrimitiveType* type = (JPPrimitiveType*) cls;
			JPMatch conv;
			JPClass *boxed = type->getBoxedClass(context);
			boxed->getJavaConversion(&frame, conv, arg[i + match.skip]);
			jvalue v = conv.conversion->convert(&frame, boxed, arg[i + match.skip]);
			frame.SetObjectArrayElement(ja, i, v.l);
		} else
		{
			frame.SetObjectArrayElement(ja, i, v[i].l);
		}
	}

	// Call the method
	jobject o = frame.callMethod(m_Method.get(), self, ja);

	JP_TRACE("ReturnType", retType->getCanonicalName());

	// Deal with the return
	if (retType->isPrimitive())
	{
		JP_TRACE("Return primitive");
		JPClass *boxed = ((JPPrimitiveType*) retType)->getBoxedClass(context);
		JPValue out = retType->getValueFromObject(JPValue(boxed, o));
		return retType->convertToPythonObject(frame, out.getValue());
	} else
	{
		JP_TRACE("Return object");
		jvalue v;
		v.l = o;
		return retType->convertToPythonObject(frame, v);
	}
	JP_TRACE_OUT;
}

JPValue JPMethod::invokeConstructor(JPJavaFrame& frame, JPMethodMatch& match, JPPyObjectVector& arg)
{
	JP_TRACE_IN("JPMethod::invokeConstructor");
	size_t alen = m_ParameterTypes.size();
	vector<jvalue> v(alen + 1);
	packArgs(frame, match, v, arg);
	JPPyCallRelease call;
	return JPValue(m_Class, frame.NewObjectA(m_Class->getJavaClass(), m_MethodID, &v[0]));
	JP_TRACE_OUT;
}

string JPMethod::matchReport(JPPyObjectVector& args)
{
	ensureTypeCache();
	JPContext *context = m_Class->getContext();
	JPJavaFrame frame(context);
	stringstream res;

	res << m_ReturnType->getCanonicalName() << " (";

	bool isFirst = true;
	for (vector<JPClass*>::iterator it = m_ParameterTypes.begin(); it != m_ParameterTypes.end(); it++)
	{
		if (isFirst && !isStatic())
		{
			isFirst = false;
			continue;
		}
		isFirst = false;
		res << (*it)->getCanonicalName();
	}

	res << ") ==> ";

	JPMethodMatch methodMatch(args.size());
	matches(frame, methodMatch, !isStatic(), args);
	switch (methodMatch.type)
	{
		case JPMatch::_none:
			res << "NONE";
			break;
		case JPMatch::_explicit:
			res << "EXPLICIT";
			break;
		case JPMatch::_implicit:
			res << "IMPLICIT";
			break;
		case JPMatch::_exact:
			res << "EXACT";
			break;
		default:
			res << "UNKNOWN";
			break;
	}

	res << endl;

	return res.str();
}

bool JPMethod::checkMoreSpecificThan(JPMethod* other) const
{
	for (JPMethodList::const_iterator it = m_MoreSpecificOverloads.begin();
			it != m_MoreSpecificOverloads.end();
			++it)
	{
		if (other == *it)
			return true;
	}
	return false;
}

void JPMethod::ensureTypeCache()
{
	if (m_ReturnType != (JPClass*) (-1))
		return;

	m_Class->getContext()->getTypeManager()->populateMethod(this, m_Method.get());
}
