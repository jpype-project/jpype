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
#include <jpype.h>

jobject JPPrimitiveType::convertToJavaObject(HostRef* obj)
{
	JPCleaner cleaner;
	JPTypeName tname = getObjectType();
	JPClass* c = JPTypeManager::findClass(tname);

	jclass jc = c->getClass();
	cleaner.addGlobal(jc);

	vector<HostRef*> args(1);
	args[0] = obj;

	JPObject* o = c->newInstance(args);
	jobject res = o->getObject();
	delete o;

	return res;
}

HostRef* JPByteType::asHostObject(jvalue val) 
{
	return JPEnv::getHost()->newInt(val.b);
}

HostRef* JPByteType::asHostObjectFromObject(jvalue val)
{
	long v = JPJni::intValue(val.l);
	return JPEnv::getHost()->newInt(v);
} 

EMatchType JPByteType::canConvertToJava(HostRef* obj)
{
	JPCleaner cleaner;
	if (JPEnv::getHost()->isNone(obj))
	{
		return _none;
	}

	if (JPEnv::getHost()->isInt(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isLong(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_byte)
		{
			return _exact;
		}
	}


	return _none;
}

jvalue JPByteType::convertToJava(HostRef* obj)
{
	jvalue res;
	if (JPEnv::getHost()->isInt(obj))
	{
		jint l = JPEnv::getHost()->intAsInt(obj);;
		if (l < JPJni::s_minByte || l > JPJni::s_maxByte)
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java byte");
		}
		res.b = (jbyte)l;
	}
	else if (JPEnv::getHost()->isLong(obj))
	{
		jlong l = JPEnv::getHost()->longAsLong(obj);
		if (l < JPJni::s_minByte || l > JPJni::s_maxByte)
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java byte");
		}
		res.b = (jbyte)l;
	}
	else if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	return res;
}

HostRef* JPByteType::convertToDirectBuffer(HostRef* src)
{
	TRACE_IN("JPByteType::convertToDirectBuffer");
	JPCleaner cleaner;
	if (JPEnv::getHost()->isByteBuffer(src))
	{

		char* rawData;
		long size;
		JPEnv::getHost()->getByteBufferPtr(src, &rawData, size);

		jobject obj = JPEnv::getJava()->NewDirectByteBuffer(rawData, size);
		cleaner.addLocal(obj);

		jvalue v;
		v.l = obj;
		JPTypeName name = JPJni::getClassName(v.l);
		JPType* type = JPTypeManager::getType(name);
		return type->asHostObject(v);
	}

	RAISE(JPypeException, "Unable to convert to Direct Buffer");
	TRACE_OUT;
}

//----------------------------------------------------------------------------

HostRef* JPShortType::asHostObject(jvalue val) 
{
	return JPEnv::getHost()->newInt(val.s);
}

HostRef* JPShortType::asHostObjectFromObject(jvalue val)
{
	long v = JPJni::intValue(val.l);
	return JPEnv::getHost()->newInt(v);
} 

EMatchType JPShortType::canConvertToJava(HostRef* obj)
{
	if (JPEnv::getHost()->isNone(obj))
	{
		return _none;
	}

	if (JPEnv::getHost()->isInt(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isLong(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_short)
		{
			return _exact;
		}
	}

	return _none;
}

jvalue JPShortType::convertToJava(HostRef* obj)
{
	jvalue res;
	if (JPEnv::getHost()->isInt(obj))
	{
		jint l = JPEnv::getHost()->intAsInt(obj);;
		if (l < JPJni::s_minShort || l > JPJni::s_maxShort)
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java short");
		}

		res.s = (jshort)l;
	}
	else if (JPEnv::getHost()->isLong(obj))
	{
		jlong l = JPEnv::getHost()->longAsLong(obj);;
		if (l < JPJni::s_minShort || l > JPJni::s_maxShort)
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java short");
		}
		res.s = (jshort)l;
	}
	else if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	return res;
}

HostRef* JPShortType::convertToDirectBuffer(HostRef* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
	
}


//-------------------------------------------------------------------------------


HostRef* JPIntType::asHostObject(jvalue val) 
{
	return JPEnv::getHost()->newInt(val.i);
}

HostRef* JPIntType::asHostObjectFromObject(jvalue val)
{
	long v = JPJni::intValue(val.l);
	return JPEnv::getHost()->newInt(v);
} 

EMatchType JPIntType::canConvertToJava(HostRef* obj)
{
	if (JPEnv::getHost()->isNone(obj))
	{
		return _none;
	}

	if (JPEnv::getHost()->isInt(obj))
	{
		return _exact;
	}

	if (JPEnv::getHost()->isLong(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_int)
		{
			return _exact;
		}
	}

	return _none;
}

jvalue JPIntType::convertToJava(HostRef* obj)
{
	jvalue res;
	if (JPEnv::getHost()->isInt(obj))
	{
		jint l = JPEnv::getHost()->intAsInt(obj);;
		if (l < JPJni::s_minInt || l > JPJni::s_maxInt)
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java int");
		}

		res.i = (jint)l;
	}
	else if (JPEnv::getHost()->isLong(obj))
	{
		jlong l = JPEnv::getHost()->longAsLong(obj);;
		if (l < JPJni::s_minInt || l > JPJni::s_maxInt)
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java int");
		}
		res.i = (jint)l;
	}
	else if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}

	return res;
}

HostRef* JPIntType::convertToDirectBuffer(HostRef* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
	
}

//-------------------------------------------------------------------------------

HostRef* JPLongType::asHostObject(jvalue val) 
{
	TRACE_IN("JPLongType::asHostObject");
	return JPEnv::getHost()->newLong(val.j);
	TRACE_OUT;
}

HostRef* JPLongType::asHostObjectFromObject(jvalue val)
{
	jlong v = JPJni::longValue(val.l);
	return JPEnv::getHost()->newLong(v);
} 

EMatchType JPLongType::canConvertToJava(HostRef* obj)
{
	if (JPEnv::getHost()->isNone(obj))
	{
		return _none;
	}

	if (JPEnv::getHost()->isInt(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isLong(obj))
	{
		return _exact;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_int)
		{
			return _exact;
		}
	}



	return _none;
}

jvalue JPLongType::convertToJava(HostRef* obj)
{
	jvalue res;
	if (JPEnv::getHost()->isInt(obj))
	{
		res.j = (jlong)JPEnv::getHost()->intAsInt(obj);
	}
	else if (JPEnv::getHost()->isLong(obj))
	{
		res.j = (jlong)JPEnv::getHost()->longAsLong(obj);
	}
	else if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	return res;
}

HostRef* JPLongType::convertToDirectBuffer(HostRef* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
	
}

//-------------------------------------------------------------------------------
HostRef* JPFloatType::asHostObject(jvalue val) 
{
	return JPEnv::getHost()->newFloat(val.f);
}

HostRef* JPFloatType::asHostObjectFromObject(jvalue val)
{
	double v = JPJni::doubleValue(val.l);
	return JPEnv::getHost()->newFloat(v);
} 

EMatchType JPFloatType::canConvertToJava(HostRef* obj)
{
	if (JPEnv::getHost()->isNone(obj))
	{
		return _none;
	}

	if (JPEnv::getHost()->isFloat(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_float)
		{
			return _exact;
		}
	}


	return _none;
}

jvalue JPFloatType::convertToJava(HostRef* obj)
{
	jvalue res;
	if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	else
	{
		double l = JPEnv::getHost()->floatAsDouble(obj);
		if (l > 0 && (l < JPJni::s_minFloat || l > JPJni::s_maxFloat))
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java float");
		}
		else if (l < 0 && (l > -JPJni::s_minFloat || l < -JPJni::s_maxFloat))
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java float");
		}
		res.f = (jfloat)l;
	}
	return res;
}

HostRef* JPFloatType::convertToDirectBuffer(HostRef* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
	
}

//---------------------------------------------------------------------------

HostRef* JPDoubleType::asHostObject(jvalue val) 
{
	HostRef* res = JPEnv::getHost()->newFloat(val.d);

	return res;
}

HostRef* JPDoubleType::asHostObjectFromObject(jvalue val)
{
	double v = JPJni::doubleValue(val.l);
	return JPEnv::getHost()->newFloat(v);
} 

EMatchType JPDoubleType::canConvertToJava(HostRef* obj)
{
	if (JPEnv::getHost()->isNone(obj))
	{
		return _none;
	}

	if (JPEnv::getHost()->isFloat(obj))
	{
		return _exact;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_double)
		{
			return _exact;
		}
	}

	return _none;
}

jvalue JPDoubleType::convertToJava(HostRef* obj)
{
	jvalue res;
	if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	else
	{
		res.d = (jdouble)JPEnv::getHost()->floatAsDouble(obj);
	}
	return res;
}

HostRef* JPDoubleType::convertToDirectBuffer(HostRef* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
	
}

//----------------------------------------------------------------

HostRef* JPCharType::asHostObject(jvalue val)   
{
	jchar str[2];
	str[0] = val.c;
	str[1] = 0;
	
	return JPEnv::getHost()->newStringFromUnicode(str, 1);
}

HostRef* JPCharType::asHostObjectFromObject(jvalue val)
{
	jchar str[2];
	str[0] = JPJni::charValue(val.l);
	str[1] = 0;
	
	return JPEnv::getHost()->newStringFromUnicode(str, 1);
} 

EMatchType JPCharType::canConvertToJava(HostRef* obj)
{
	JPCleaner cleaner;
	if (JPEnv::getHost()->isNone(obj))
	{
		return _none;
	}

	if (JPEnv::getHost()->isString(obj) && JPEnv::getHost()->getStringLength(obj) == 1)
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_char)
		{
			return _exact;
		}
	}



	return _none;
}

jvalue JPCharType::convertToJava(HostRef* obj)
{
	JPCleaner cleaner;
	jvalue res;

	if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	else
	{
		JCharString str = JPEnv::getHost()->stringAsJCharString(obj);

		res.c = str[0];
	}
	return res;
}

HostRef* JPCharType::convertToDirectBuffer(HostRef* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
	
}

//----------------------------------------------------------------------------------------

HostRef* JPBooleanType::asHostObject(jvalue val) 
{
	if (val.z)
	{
		return JPEnv::getHost()->getTrue();
	}
	return JPEnv::getHost()->getFalse();
}

HostRef* JPBooleanType::asHostObjectFromObject(jvalue val)
{
	bool z = JPJni::booleanValue(val.l);
	if (z)
	{
		return JPEnv::getHost()->getTrue();
	}
	return JPEnv::getHost()->getFalse();
} 

EMatchType JPBooleanType::canConvertToJava(HostRef* obj)
{
	if (JPEnv::getHost()->isInt(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_boolean)
		{
			return _exact;
		}
	}

	return _none;
}

jvalue JPBooleanType::convertToJava(HostRef* obj)
{
	jvalue res;
	if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	else
	{
		res.z = (jboolean)JPEnv::getHost()->intAsInt(obj);
	}
	return res;
}

HostRef* JPBooleanType::convertToDirectBuffer(HostRef* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
}
