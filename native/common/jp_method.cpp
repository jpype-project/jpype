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
#include <jpype.h>
#include <jp_method.h>

JPMethod::JPMethod(JPClass* claz,
		const string& name,
		jobject mth,
		jmethodID mid,
		JPClass *returnType,
		JPClassList parameterTypes,
		JPMethodList& moreSpecific,
		jint modifiers)
: m_Method(claz->getContext(), mth)
{
	this->m_Class = claz;
	this->m_Name = name;
	this->m_MethodID = mid;
	this->m_ReturnType = returnType;
	this->m_ParameterTypes = parameterTypes;
	this->m_MoreSpecificOverloads = moreSpecific;
	this->m_Modifiers = modifiers;
}

JPMethod::~JPMethod()
{
}

string JPMethod::toString() const
{
	return m_Name;
}

JPMatch::Type matchVars(JPJavaFrame &frame, JPMethodMatch& match, JPPyObjectVector &arg, size_t start, JPClass *vartype)
{
	JP_TRACE_IN_C("JPMethod::matchVars");
	JPArrayClass *arraytype = (JPArrayClass*) vartype;
	JPClass *type = arraytype->getComponentType();
	size_t len = arg.size();

	JPMatch::Type lastMatch = JPMatch::_exact;
	for (size_t i = start; i < len; i++)
	{
		JPMatch::Type quality = type->getJavaConversion(match[i], frame, arg[i]);

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
	JP_TRACE_OUT_C;
}

JPMatch::Type JPMethod::matches(JPJavaFrame &frame, JPMethodMatch& match, bool callInstance,
		JPPyObjectVector& arg)
{
	JP_TRACE_IN("JPMethod::matches");
	match.overload = this;
	match.offset = 0;
	match.skip = 0;

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

	match.type = JPMatch::_exact;
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
		JPClass* type = m_ParameterTypes[tlen - 1];
		if (len < tlen - 1)
		{
			return match.type = JPMatch::_none;
		}

		// Hard, could be direct array or an array.
		if (len == tlen)
		{
			// Try direct
			size_t last = tlen - 1 - match.offset;
			PyObject* obj = arg[last];
			--len;
			match.type = type->getJavaConversion(match.argument[last], frame, obj);
			if (match.type < JPMatch::_implicit)
			{
				// Try indirect
				match.type = matchVars(frame, match, arg, last, type);
				match.isVarIndirect = true;
				JP_TRACE("Match vargs indirect", match.type);
			} else
			{
				match.isVarDirect = true;
				JP_TRACE("Match vargs direct", match.type);
			}
		} else if (len > tlen)
		{
			// Must match the array type
			len = tlen - 1;
			match.type = matchVars(frame, match, arg, tlen - 1 + match.offset, type);
			match.isVarIndirect = true;
			JP_TRACE("Match vargs indirect", match.type);
		} else if (len < tlen)
		{
			match.isVarIndirect = true;
			JP_TRACE("Match vargs empty");
		}

		if (match.type < JPMatch::_implicit)
		{
			return match.type;
		}
	}

	JP_TRACE("Start match");
	for (size_t i = 0; i < len; i++)
	{
		size_t j = i + match.offset;
		JPClass *type = m_ParameterTypes[i];
		JP_TRACE("compare", i, j, type->toString(), JPPyObject::getTypeName(arg[j]));
		JPMatch::Type ematch = type->getJavaConversion( match.argument[j], frame, arg[j]);
		JP_TRACE("result", ematch);
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
		JP_TRACE("Pack varargs");
		len = tlen - 1;
		JPArrayClass* type = (JPArrayClass*) m_ParameterTypes[tlen - 1];
		v[tlen - 1 - match.skip] = type->convertToJavaVector(arg, tlen - 1, (jsize) arg.size());
	}

	JP_TRACE("Pack fixed total=", len - match.offset);
	for (size_t i = match.skip; i < len; i++)
	{
		JPClass* type = m_ParameterTypes[i - match.offset];
		JPConversion *conversion = match.argument[i].conversion;
		JP_TRACE("Convert", i - match.offset, i, type->getCanonicalName(), conversion);
		if (conversion == NULL)
			JP_RAISE_RUNTIME_ERROR("Conversion is null");
		v[i - match.skip] = conversion->convert(frame, type, arg[i]);
	}
	JP_TRACE_OUT;
}

JPPyObject JPMethod::invoke(JPMethodMatch& match, JPPyObjectVector& arg, bool instance)
{
	JPContext *context = m_Class->getContext();
	JP_TRACE_IN("JPMethod::invoke");
	size_t alen = m_ParameterTypes.size();
	JPJavaFrame frame(context, (int) (8 + alen));

	JPClass* retType = m_ReturnType;

	// Pack the arguments
	vector<jvalue> v(alen + 1);
	packArgs(frame, match, v, arg);

	// Check if it is caller sensitive
	if (isCallerSensitive())
	{
		JP_TRACE("Caller sensitive method");
		//public static Object callMethod(Method method, Object obj, Object[] args)
		jobject self = NULL;
		size_t len = alen;
		if (!isStatic())
		{
			JP_TRACE("Call instance");
			len--;
			JPValue *selfObj = JPPythonEnv::getJavaValue(arg[0]);
			self = selfObj->getJavaObject();
		}

		// Convert arguments
		jobjectArray ja = frame.NewObjectArray(len, context->_java_lang_Object->getJavaClass(), NULL);
		for (jsize i = 0; i < (jsize) len; ++i)
		{
			JPClass *cls = m_ParameterTypes[i + match.skip - match.offset];
			if (cls->isPrimitive())
			{
				JPPrimitiveType* type = (JPPrimitiveType*) cls;
				JPMatch conv;
				type->getBoxedClass()->getJavaConversion(conv, frame, arg[i + match.skip]);
				frame.SetObjectArrayElement(ja, i,
						conv.conversion->convert(frame, type->getBoxedClass(), arg[i + match.skip]).l);
			} else
			{
				frame.SetObjectArrayElement(ja, i, v[i].l);
			}
		}

		// Call the method
		jobject o = context->callMethod(m_Method.get(), self, ja);

		// Deal with the return
		if (retType->isPrimitive())
		{
			JP_TRACE("Return primitive");
			JPValue out = retType->getValueFromObject(o);
			return retType->convertToPythonObject(out.getValue());
		} else
		{
			JP_TRACE("Return object");
			jvalue v;
			v.l = o;
			return retType->convertToPythonObject(v);
		}
	}

	// Invoke the method (arg[0] = this)
	if (JPModifier::isStatic(m_Modifiers))
	{
		JP_TRACE("invoke static", m_Name);
		jclass claz = m_Class->getJavaClass();
		return retType->invokeStatic(frame, claz, m_MethodID, &v[0]);
	} else
	{
		JPValue* selfObj = JPPythonEnv::getJavaValue(arg[0]);
		jobject c;
		if (selfObj == NULL)
		{
			// This only can be hit by calling an instance method as a
			// class object.  We already know it is safe to convert.
			jvalue  v = match.argument[0].conversion->convert(frame, m_Class, arg[0]);
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

JPValue JPMethod::invokeConstructor(JPMethodMatch& match, JPPyObjectVector& arg)
{
	JP_TRACE_IN("JPMethod::invokeConstructor");
	size_t alen = m_ParameterTypes.size();
	JPJavaFrame frame(m_Class->getContext(), 8 + alen);

	vector<jvalue> v(alen + 1);
	packArgs(frame, match, v, arg);

	jvalue val;
	{
		JPPyCallRelease call;
		val.l = frame.keep(frame.NewObjectA(m_Class->getJavaClass(), m_MethodID, &v[0]));
	}
	return JPValue(m_Class, val);
	JP_TRACE_OUT;
}

string JPMethod::matchReport(JPPyObjectVector& args)
{
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
