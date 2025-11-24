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
#include "jp_arrayclass.h"
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
= default;

void JPMethod::setParameters(
		JPClass *returnType,
		JPClassList&& parameterTypes)
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
	auto *arraytype = dynamic_cast<JPArrayClass*>( vartype);
	JPClass *type = arraytype->getComponentType();
	size_t len = arg.size();

	JPMatch::Type lastMatch = JPMatch::_exact;
	for (size_t i = start; i < len; i++)
	{
		JPMatch::Type quality = type->findJavaConversion(match[i]);

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
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

JPMatch::Type JPMethod::matches(JPJavaFrame &frame, JPMethodMatch& methodMatch, bool callInstance,
		JPPyObjectVector& arg)
{
	ensureTypeCache();

	JP_TRACE_IN("JPMethod::matches");
	methodMatch.m_Overload = this;
	methodMatch.m_Offset = 0;
	methodMatch.m_Skip = 0;
	methodMatch.m_IsVarIndirect = false;
	methodMatch.m_Type = JPMatch::_exact;

	size_t len = arg.size();
	size_t tlen = m_ParameterTypes.size();
	JP_TRACE("Flags", isStatic(), isInstance(), callInstance);
	JP_TRACE("arguments length", len);
	JP_TRACE("types length", tlen);
	if (callInstance && isStatic())
	{
		JP_TRACE("Skip this");
		len--;
		methodMatch.m_Offset = 1;
	}

	if (callInstance || isInstance())
	{
		JP_TRACE("Take this");
		methodMatch.m_Skip = 1;
	}

	if (!JPModifier::isVarArgs(m_Modifiers))
	{
		// Bypass if length does not match
		if (len != tlen)
		{
			JP_TRACE("Argument length mismatch", len, tlen);
			return methodMatch.m_Type = JPMatch::_none;
		}
	} else
	{
		JP_TRACE("Match vargs");
		methodMatch.m_Type = JPMatch::_none;
		if (len < tlen - 1)
		{
			return methodMatch.m_Type;
		}

		JPClass* type = m_ParameterTypes[tlen - 1];
		// Hard, could be direct array or an array.
		if (len == tlen)
		{
			// Try direct
			size_t last = tlen - 1 - methodMatch.m_Offset;
			methodMatch.m_Type = type->findJavaConversion(methodMatch.m_Arguments[last]);
			JP_TRACE("Direct vargs", methodMatch.m_Type);
		}

		if (methodMatch.m_Type < JPMatch::_implicit && len >= tlen)
		{
			// Must match the array type
			methodMatch.m_Type = matchVars(frame, methodMatch, arg, tlen - 1 + methodMatch.m_Offset, type);
			methodMatch.m_IsVarIndirect = true;
			JP_TRACE("Indirect vargs", methodMatch.m_Type);
		} else if (len < tlen)
		{
			methodMatch.m_IsVarIndirect = true;
			methodMatch.m_Type = JPMatch::_exact;
			JP_TRACE("Empty vargs");
		}
		len = tlen - 1;

		if (methodMatch.m_Type < JPMatch::_implicit)
		{
			return methodMatch.m_Type;
		}
	}

	JP_TRACE("Match args");
	for (size_t i = 0; i < len; i++)
	{
		size_t j = i + methodMatch.m_Offset;
		JPClass *type = m_ParameterTypes[i];
		JPMatch::Type ematch = type->findJavaConversion(methodMatch.m_Arguments[j]);
		JP_TRACE("Result", ematch);
		if (ematch < methodMatch.m_Type)
		{
			methodMatch.m_Type = ematch;
		}
		if (methodMatch.m_Type < JPMatch::_implicit)
		{
			return methodMatch.m_Type;
		}
	}

	return methodMatch.m_Type;
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPMethod::packArgs(JPJavaFrame &frame, JPMethodMatch &match,
		vector<jvalue> &v, JPPyObjectVector &arg)
{
	JP_TRACE_IN("JPMethod::packArgs");
	size_t len = arg.size();
	size_t tlen = m_ParameterTypes.size();
	JP_TRACE("skip", match.m_Skip == 1);
	JP_TRACE("offset", match.m_Offset == 1);
	JP_TRACE("arguments length", len);
	JP_TRACE("types length", tlen);
	if (match.m_IsVarIndirect)
	{
		JP_TRACE("Pack indirect varargs");
		len = tlen - 1;
		auto* type = dynamic_cast<JPArrayClass*>( m_ParameterTypes[tlen - 1]);
		v[tlen - 1 - match.m_Skip] = type->convertToJavaVector(frame, arg, (jsize) tlen - 1, (jsize) arg.size());
	}

	JP_TRACE("Pack fixed total=", len - match.m_Offset);
	for (size_t i = match.m_Skip; i < len; i++)
	{
		v[i - match.m_Skip] = match.m_Arguments[i].convert();
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

JPPyObject JPMethod::invoke(JPJavaFrame& frame, JPMethodMatch& match, JPPyObjectVector& arg, bool instance)
{
	JP_TRACE_IN("JPMethod::invoke");
	// Check if it is caller sensitive
	if (isCallerSensitive())
		return invokeCallerSensitive(match, arg, instance);

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
		if (selfObj == nullptr)
		{
			// This only can be hit by calling an instance method as a
			// class object.  We already know it is safe to convert.
			auto val = match.m_Arguments[0].convert();
			c = val.l;
		} else
		{
			c = selfObj->getJavaObject();
		}
		jclass clazz = nullptr;
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
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

JPPyObject JPMethod::invokeCallerSensitive(JPMethodMatch& match, JPPyObjectVector& arg, bool instance)
{
	JP_TRACE_IN("JPMethod::invokeCallerSensitive");
	size_t alen = m_ParameterTypes.size();
	JPJavaFrame frame = JPJavaFrame::outer((int) (8 + alen));
	JPContext *context = frame.getContext();
	JPClass* retType = m_ReturnType;

	// Pack the arguments
	vector<jvalue> v(alen + 1);
	packArgs(frame, match, v, arg);

	//Proxy the call to
	//   public static Object callMethod(Method method, Object obj, Object[] args)
	jobject self = nullptr;
	size_t len = alen;
	if (!isStatic())
	{
		JP_TRACE("Call instance");
		len--;
		JPValue *selfObj = PyJPValue_getJavaSlot(arg[0]);
		if (selfObj == nullptr)
			JP_RAISE(PyExc_RuntimeError, "Null object"); // GCOVR_EXCL_LINE
		self = selfObj->getJavaObject();
	}

	// Convert arguments
	jobjectArray ja = frame.NewObjectArray((jsize) len, context->_java_lang_Object->getJavaClass(), nullptr);
	for (jsize i = 0; i < (jsize) len; ++i)
	{
		JPClass *cls = m_ParameterTypes[i + match.m_Skip - match.m_Offset];
		if (cls->isPrimitive())
		{
			auto* type = dynamic_cast<JPPrimitiveType*>( cls);
			PyObject *u = arg[i + match.m_Skip];
			JPMatch conv(&frame, u);
			JPClass *boxed = type->getBoxedClass(frame);
			boxed->findJavaConversion(conv);
			jvalue v = conv.convert();
			frame.SetObjectArrayElement(ja, i, v.l);
		} else
		{
			frame.SetObjectArrayElement(ja, i, v[i].l);
		}
	}

	// Call the method
	jobject o;
	{
		JPPyCallRelease call;
		o =	 frame.callMethod(m_Method.get(), self, ja);
	}


	JP_TRACE("ReturnType", retType->getCanonicalName());

	// Deal with the return
	if (retType->isPrimitive())
	{
		JP_TRACE("Return primitive");
		JPClass *boxed = (dynamic_cast<JPPrimitiveType*>( retType))->getBoxedClass(frame);
		JPValue out = retType->getValueFromObject(frame, JPValue(boxed, o));
		return retType->convertToPythonObject(frame, out.getValue(), false);
	} else
	{
		JP_TRACE("Return object");
		jvalue v;
		v.l = o;
		return retType->convertToPythonObject(frame, v, false);
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
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

string JPMethod::matchReport(JPPyObjectVector& args)
{
	ensureTypeCache();
	JPJavaFrame frame = JPJavaFrame::outer();
	std::stringstream res;

	res << m_ReturnType->getCanonicalName() << " (";

	bool isFirst = true;
	for (auto & m_ParameterType : m_ParameterTypes)
	{
		if (isFirst && !isStatic())
		{
			isFirst = false;
			continue;
		}
		isFirst = false;
		res << m_ParameterType->getCanonicalName();
	}

	res << ") ==> ";

	JPMethodMatch methodMatch(frame, args, false);
	matches(frame, methodMatch, !isStatic(), args);
	// GCOVR_EXCL_START
	switch (methodMatch.m_Type)
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
	// GCOVR_EXCL_STOP
	res << std::endl;
	return res.str();
}

bool JPMethod::checkMoreSpecificThan(JPMethod* other) const
{
	for (auto m_MoreSpecificOverload : m_MoreSpecificOverloads)
	{
		if (other == m_MoreSpecificOverload)
			return true;
	}
	return false;
}

void JPMethod::ensureTypeCache()
{
	if (m_ReturnType != (JPClass*) (-1))
		return;

	JPContext_global->getTypeManager()->populateMethod(this, m_Method.get());
}
