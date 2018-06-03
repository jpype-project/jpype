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
#include <jp_primitive_common.h>

HostRef* JPByteType::asHostObject(jvalue val)
{
	return JPEnv::getHost()->newInt(field(val));
}

HostRef* JPByteType::asHostObjectFromObject(jvalue val)
{
	return JPEnv::getHost()->newInt(JPJni::intValue(val.l));
}

EMatchType JPByteType::canConvertToJava(HostRef* obj)
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
			JPEnv::getHost()->raise("JPByteType::convertToJava");
		}
		field(res) = (type_t)l;
	}
	else if (JPEnv::getHost()->isLong(obj))
	{
		jlong l = JPEnv::getHost()->longAsLong(obj);
		if (l < JPJni::s_minByte || l > JPJni::s_maxByte)
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java byte");
			JPEnv::getHost()->raise("JPByteType::convertToJava");
		}
		field(res) = (type_t)l;
	}
	else if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	return res;
}

HostRef* JPByteType::convertToDirectBuffer(HostRef* src)
{
	JPJavaFrame frame;
	TRACE_IN("JPByteType::convertToDirectBuffer");
	if (JPEnv::getHost()->isByteBuffer(src))
	{

		char* rawData;
		long size;
		JPEnv::getHost()->getByteBufferPtr(src, &rawData, size);

		jobject obj = frame.NewDirectByteBuffer(rawData, size);

		jvalue v;
		v.l = obj;
		JPTypeName name = JPJni::getClassName(v.l);
		JPType* type = JPTypeManager::getType(name);
		return type->asHostObject(v);
	}

	RAISE(JPypeException, "Unable to convert to Direct Buffer");
	TRACE_OUT;
}


jarray JPByteType::newArrayInstance(JPJavaFrame& frame, int sz)
{
	return frame.NewByteArray(sz);
}

HostRef* JPByteType::getStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetStaticByteField(c, fid);
	return asHostObject(v);
}

HostRef* JPByteType::getInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetByteField(c, fid);
	return asHostObject(v);
}

HostRef* JPByteType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallStaticByteMethodA(claz, mth, val);
	return asHostObject(v);
}

HostRef* JPByteType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallNonvirtualByteMethodA(obj, clazz, mth, val);
	return asHostObject(v);
}

void JPByteType::setStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticByteField(c, fid, val);
}

void JPByteType::setInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetByteField(c, fid, val);
}

vector<HostRef*> JPByteType::getArrayRange(JPJavaFrame& frame, jarray a, int start, int length)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetByteArrayElements, &JPJavaFrame::ReleaseByteArrayElements);

	type_t* val = accessor.get();
	vector<HostRef*> res;
		
	jvalue v;
	for (int i = 0; i < length; i++)
	{
		field(v) = val[i+start];
		res.push_back(asHostObject(v));
	}
	return res;
}

void JPByteType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, vector<HostRef*>& vals)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetByteArrayElements, &JPJavaFrame::ReleaseByteArrayElements);

	type_t* val = accessor.get();
	for (int i = 0; i < length; i++)
	{
		HostRef* pv = vals[i];
		val[start+i] = field(convertToJava(pv));
	}
	accessor.commit();
}

void JPByteType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, PyObject* sequence)
{
	if (setViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
			&JPJavaFrame::SetByteArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetByteArrayElements, &JPJavaFrame::ReleaseByteArrayElements);

	type_t* val = accessor.get();
	for (Py_ssize_t i = 0; i < length; ++i) {
		PyObject* o = PySequence_GetItem(sequence, i);
#if (PY_VERSION_HEX >= 0x02070000)
		type_t v = (type_t) PyInt_AsLong(o);
		if (v == -1) { CONVERSION_ERROR_HANDLE(i, o); }
#else
		type_t v = (type_t) PyInt_AS_LONG(o);
#endif
		Py_DECREF(o);
		val[start+i] = v;
	}
	accessor.commit();
}

HostRef* JPByteType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	array_t array = (array_t)a;
	type_t val;
	frame.GetByteArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return asHostObject(v);
}

void JPByteType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx, HostRef* obj)
{
	array_t array = (array_t)a;
	type_t val = field(convertToJava(obj));
	frame.SetByteArrayRegion(array, ndx, 1, &val);
}

PyObject* JPByteType::getArrayRangeToSequence(JPJavaFrame& frame, jarray a, int lo, int hi)
{
	return getSlice<jbyte>(frame, a, lo, hi, NPY_BYTE, PyInt_FromLong);
}

