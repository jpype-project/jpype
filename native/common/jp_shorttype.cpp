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

HostRef* JPShortType::asHostObject(jvalue val)
{
	return JPEnv::getHost()->newInt(field(val));
}

HostRef* JPShortType::asHostObjectFromObject(jvalue val)
{
	return JPEnv::getHost()->newInt(JPJni::intValue(val.l));
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
			JPEnv::getHost()->raise("JPShortType::convertToJava");
		}
		field(res) = (type_t)l;
	}
	else if (JPEnv::getHost()->isLong(obj))
	{
		jlong l = JPEnv::getHost()->longAsLong(obj);;
		if (l < JPJni::s_minShort || l > JPJni::s_maxShort)
		{
			JPEnv::getHost()->setTypeError("Cannot convert value to Java short");
			JPEnv::getHost()->raise("JPShortType::convertToJava");
		}
		field(res) = (type_t)l;
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

jarray JPShortType::newArrayInstance(JPJavaFrame& frame, int sz)
{
	return frame.NewShortArray(sz);
}

HostRef* JPShortType::getStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetStaticShortField(c, fid);
	return asHostObject(v);
}

HostRef* JPShortType::getInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetShortField(c, fid);
	return asHostObject(v);
}

HostRef* JPShortType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallStaticShortMethodA(claz, mth, val);
	return asHostObject(v);
}

HostRef* JPShortType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallNonvirtualShortMethodA(obj, clazz, mth, val);
	return asHostObject(v);
}

void JPShortType::setStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticShortField(c, fid, val);
}

void JPShortType::setInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetShortField(c, fid, val);
}

vector<HostRef*> JPShortType::getArrayRange(JPJavaFrame& frame, jarray a, int start, int length)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetShortArrayElements, &JPJavaFrame::ReleaseShortArrayElements);

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

void JPShortType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, vector<HostRef*>& vals)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetShortArrayElements, &JPJavaFrame::ReleaseShortArrayElements);

	type_t* val = accessor.get();
	for (int i = 0; i < length; i++)
	{
		HostRef* pv = vals[i];
		val[start+i] = field(convertToJava(pv));
	}
	accessor.commit();
}

void JPShortType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, PyObject* sequence)
{
	if (setViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
		&JPJavaFrame::SetShortArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetShortArrayElements, &JPJavaFrame::ReleaseShortArrayElements);

	type_t* val = accessor.get();
	for (Py_ssize_t i = 0; i < length; ++i) {
		PyObject* o = PySequence_GetItem(sequence, i);
		type_t v = (type_t) PyInt_AsLong(o);
		if (v == -1) { CONVERSION_ERROR_HANDLE(i, o); }
		Py_DECREF(o);
		val[start+i] = v;
	}
	accessor.commit();
}

HostRef* JPShortType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	array_t array = (array_t)a;
	type_t val;
	frame.GetShortArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return asHostObject(v);
}

void JPShortType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx , HostRef* obj)
{
	array_t array = (array_t)a;
	type_t val = field(convertToJava(obj));
	frame.SetShortArrayRegion(array, ndx, 1, &val);
}

PyObject* JPShortType::getArrayRangeToSequence(JPJavaFrame& frame, jarray a, int lo, int hi) {
	return getSlice<type_t>(frame, a, lo, hi, NPY_SHORT, PyInt_FromLong);
}

