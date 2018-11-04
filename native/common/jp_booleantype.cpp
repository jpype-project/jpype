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
	if (JPEnv::getHost()->isInt(obj) || JPEnv::getHost()->isLong(obj))
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
	else if (JPEnv::getHost()->isLong(obj))
	{
		res.z = (jboolean)JPEnv::getHost()->longAsLong(obj);
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

jarray JPBooleanType::newArrayInstance(JPJavaFrame& frame, int sz)
{
	return frame.NewBooleanArray(sz);
}

HostRef* JPBooleanType::getStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetStaticBooleanField(c, fid);
	return asHostObject(v);
}

HostRef* JPBooleanType::getInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetBooleanField(c, fid);
	return asHostObject(v);
}

HostRef* JPBooleanType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallStaticBooleanMethodA(claz, mth, val);
	return asHostObject(v);
}

HostRef* JPBooleanType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallNonvirtualBooleanMethodA(obj, clazz, mth, val);
	return asHostObject(v);
}

void JPBooleanType::setStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticBooleanField(c, fid, val);
}

void JPBooleanType::setInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetBooleanField(c, fid, val);
}

vector<HostRef*> JPBooleanType::getArrayRange(JPJavaFrame& frame, jarray a, int start, int length)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetBooleanArrayElements, &JPJavaFrame::ReleaseBooleanArrayElements);

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

void JPBooleanType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, vector<HostRef*>& vals)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetBooleanArrayElements, &JPJavaFrame::ReleaseBooleanArrayElements);

	type_t* val = accessor.get();
	for (int i = 0; i < length; i++)
	{
		HostRef* pv = vals[i];
		val[start+i] = field(convertToJava(pv));
	}
	accessor.commit();
}

void JPBooleanType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, PyObject* sequence)
{
	if (setViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
			&JPJavaFrame::SetBooleanArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetBooleanArrayElements, &JPJavaFrame::ReleaseBooleanArrayElements);

	type_t* val = accessor.get();
	for (Py_ssize_t i = 0; i < length; ++i)
	{
		PyObject* o = PySequence_GetItem(sequence, i);
// FIXME this is suspect as we are supposed to be boolean, not integers
		long v = PyInt_AsLong(o);
		if (v == -1) { CONVERSION_ERROR_HANDLE(i, o); }
		Py_DECREF(o);
		val[start+i] = (type_t) v;
	}
	accessor.commit();
}

HostRef* JPBooleanType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	array_t array = (array_t)a;
	type_t val;
	frame.GetBooleanArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return asHostObject(v);
}

void JPBooleanType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx , HostRef* obj)
{
	array_t array = (array_t)a;
	type_t val = field(convertToJava(obj));
	frame.SetBooleanArrayRegion(array, ndx, 1, &val);
}

PyObject* JPBooleanType::getArrayRangeToSequence(JPJavaFrame& frame, jarray a, int start, int length)
{
	return getSlice<type_t>(frame, a, start, length, NPY_BOOL, PyBool_FromLong);
}

