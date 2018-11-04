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

jarray JPCharType::newArrayInstance(JPJavaFrame& frame, int sz)
{
	return frame.NewCharArray(sz);
}

HostRef* JPCharType::getStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetStaticCharField(c, fid);
	
	return asHostObject(v);
}

HostRef* JPCharType::getInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetCharField(c, fid);
	
	return asHostObject(v);
}

HostRef* JPCharType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallStaticCharMethodA(claz, mth, val);
	return asHostObject(v);
}

HostRef* JPCharType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallNonvirtualCharMethodA(obj, clazz, mth, val);
	return asHostObject(v);
}

void JPCharType::setStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticCharField(c, fid, val);
}

void JPCharType::setInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetCharField(c, fid, val);
}

vector<HostRef*> JPCharType::getArrayRange(JPJavaFrame& frame, jarray a, int start, int length)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetCharArrayElements, &JPJavaFrame::ReleaseCharArrayElements);

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

void JPCharType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, vector<HostRef*>& vals)
{	
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetCharArrayElements, &JPJavaFrame::ReleaseCharArrayElements);

	type_t* val = accessor.get();
	for (int i = 0; i < length; i++)
	{
		HostRef* pv = vals[i];
		val[start+i] = field(convertToJava(pv));
	}
	accessor.commit();
}

void JPCharType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, PyObject* sequence)
{
	if (setViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
			&JPJavaFrame::SetCharArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetCharArrayElements, &JPJavaFrame::ReleaseCharArrayElements);

	type_t* val = accessor.get();
	for (Py_ssize_t i = 0; i < length; ++i) 
	{
		PyObject* o = PySequence_GetItem(sequence, i);
		long v =  PyInt_AsLong(o);
		if (v == -1) { CONVERSION_ERROR_HANDLE(i, o); }
		Py_DECREF(o);
		val[start+i] = (type_t) v;
	}
	accessor.commit();
}

HostRef* JPCharType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	array_t array = (array_t)a;
	type_t val;
	frame.GetCharArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return asHostObject(v);
}

void JPCharType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx , HostRef* obj)
{
	array_t array = (array_t)a;
	type_t val = field(convertToJava(obj));
	frame.SetCharArrayRegion(array, ndx, 1, &val);
}

PyObject* JPCharType::getArrayRangeToSequence(JPJavaFrame& frame, jarray a, int start, int length)
{
	PyObject* res = NULL;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetCharArrayElements, &JPJavaFrame::ReleaseCharArrayElements);

	type_t* val = accessor.get();
	if (sizeof(Py_UNICODE) == sizeof(jchar))
	{
		// FIXME this is an error, the encoding used by JAVA does not match standard UTF16.
		res = PyUnicode_FromUnicode((const Py_UNICODE *) val + start, length);
	}
	else
	{
		res = PyUnicode_FromUnicode(NULL, length);
		Py_UNICODE *pchars = PyUnicode_AS_UNICODE(res);

		for (Py_ssize_t i = start; i < length; i++)
			pchars[i] = (Py_UNICODE) val[i];
	}
	return res;
}

