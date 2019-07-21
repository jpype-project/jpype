/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#ifndef _JP_FLOAT_TYPE_H_
#define _JP_FLOAT_TYPE_H_

class JPFloatType : public JPPrimitiveType
{
public:

	JPFloatType(JPContext* context, jclass clss, const string& name, JPBoxedType* boxedClass, jint modifiers);
	virtual ~JPFloatType();

public:
	typedef jfloat type_t;
	typedef jfloatArray array_t;

	static inline jfloat& field(jvalue& v)
	{
		return v.f;
	}
	
	static inline const jfloat& field(const jvalue& v)
	{
		return v.f;
	}


public:
	virtual JPMatch::Type getJavaConversion(JPMatch& match, JPJavaFrame& frame, PyObject* pyobj) override;
	virtual JPPyObject  convertToPythonObject(jvalue val) override;
	virtual JPValue     getValueFromObject(jobject obj) override;

	virtual JPPyObject  invokeStatic(JPJavaFrame& frame, jclass, jmethodID, jvalue*) override;
	virtual JPPyObject  invoke(JPJavaFrame& frame, jobject, jclass, jmethodID, jvalue*) override;

	virtual JPPyObject  getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid) override;
	virtual void        setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* val) override;
	virtual JPPyObject  getField(JPJavaFrame& frame, jobject c, jfieldID fid) override;
	virtual void        setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* val) override;

	virtual jarray      newArrayInstance(JPJavaFrame& frame, jsize size) override;
	virtual JPPyObject  getArrayRange(JPJavaFrame& frame, jarray, jsize start, jsize length) override;
	virtual void        setArrayRange(JPJavaFrame& frame, jarray, jsize, jsize, PyObject*) override;
	virtual JPPyObject  getArrayItem(JPJavaFrame& frame, jarray, jsize ndx) override;
	virtual void        setArrayItem(JPJavaFrame& frame, jarray, jsize ndx, PyObject* val) override;

	template <class T> T assertRange(const T& l)
	{
		if (l < -_Float_Max || l > _Float_Max)
		{
			JP_RAISE_OVERFLOW_ERROR("Cannot convert value to Java float");
		}
		return l;
	}

	virtual char getTypeCode() override
	{
		return 'F';
	}

	virtual jlong getAsLong(jvalue v) override
	{
		return field(v);
	}

	virtual jdouble getAsDouble(jvalue v) override
	{
		return field(v);
	}

private:
	jdouble _Float_Max;
	jmethodID _FloatValueID;
} ;

#endif // _JP_FLOAT_TYPE_H_
