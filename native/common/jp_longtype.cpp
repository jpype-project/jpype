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

HostRef* JPLongType::asHostObject(jvalue val)
{
	return JPEnv::getHost()->newLong(field(val));
}

HostRef* JPLongType::asHostObjectFromObject(jvalue val)
{
	return JPEnv::getHost()->newLong(JPJni::longValue(val.l));
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
		if (JPEnv::getHost()->isObject(obj))
		{
			return _implicit;
		}
		return _exact;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_long)
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
		field(res) = (type_t)JPEnv::getHost()->intAsInt(obj);
	}
	else if (JPEnv::getHost()->isLong(obj))
	{
		field(res) = (type_t)JPEnv::getHost()->longAsLong(obj);
	}
	else if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	else
	{
		JPEnv::getHost()->setTypeError("Cannot convert value to Java long");
		JPEnv::getHost()->raise("JPLongType::convertToJava");
		field(res) = 0; // never reached
	}
	return res;
}

HostRef* JPLongType::convertToDirectBuffer(HostRef* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

jarray JPLongType::newArrayInstance(JPJavaFrame& frame, int sz)
{
	return frame.NewLongArray(sz);
}

HostRef* JPLongType::getStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetStaticLongField(c, fid);
	
	return asHostObject(v);
}

HostRef* JPLongType::getInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetLongField(c, fid);
	return asHostObject(v);
}

HostRef* JPLongType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallStaticLongMethodA(claz, mth, val);
	return asHostObject(v);
}

HostRef* JPLongType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallNonvirtualLongMethodA(obj, clazz, mth, val);
	return asHostObject(v);
}

void JPLongType::setStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticLongField(c, fid, val);
}

void JPLongType::setInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetLongField(c, fid, val);
}

vector<HostRef*> JPLongType::getArrayRange(JPJavaFrame& frame, jarray a, int start, int length)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);

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

void JPLongType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, vector<HostRef*>& vals)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);

	type_t* val = accessor.get();
	for (int i = 0; i < length; i++)
	{
		HostRef* pv = vals[i];
		val[start+i] = field(convertToJava(pv));
	}
	accessor.commit();
}

void JPLongType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, PyObject* sequence)
{
	if (setViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
		&JPJavaFrame::SetLongArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);

	type_t* val = accessor.get();
	for (Py_ssize_t i = 0; i < length; ++i) 
	{
		PyObject* o = PySequence_GetItem(sequence, i);
		type_t v = (type_t) PyInt_AsLong(o);
		if (v == -1) { CONVERSION_ERROR_HANDLE(i, o); }
		Py_DECREF(o);
		val[start+i] = v;
	}
	accessor.commit();
}

HostRef* JPLongType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	array_t array = (array_t)a;
	type_t val;
	frame.GetLongArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return asHostObject(v);
}

void JPLongType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx , HostRef* obj)
{
	array_t array = (array_t)a;
	type_t val = field(convertToJava(obj));
	frame.SetLongArrayRegion(array, ndx, 1, &val);
}

PyObject* JPLongType::getArrayRangeToSequence(JPJavaFrame& frame, jarray a, int lo, int hi) {
	return getSlice<type_t>(frame, a, lo, hi, NPY_INT64, PyLong_FromLong);
}

