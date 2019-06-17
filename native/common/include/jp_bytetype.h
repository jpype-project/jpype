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
#ifndef _JPBYTE_TYPE_H_
#define _JPBYTE_TYPE_H_

class JPByteType : public JPPrimitiveType
{
public:

	JPByteType();
	virtual ~JPByteType();

public:
	typedef jbyte type_t;
	typedef jbyteArray array_t;

	inline jbyte& field(jvalue& v)
	{
		return v.b;
	}

	inline jbyte field(const jvalue& v) const
	{
		return v.b;
	}

public:
	virtual JPMatch::Type  canConvertToJava(PyObject* obj) override;
	virtual jvalue      convertToJava(PyObject* obj) override;
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

	JPPyObject toBytes(JPJavaFrame& frame, jarray);

	// Only Byte supports direct buffer convertion
	virtual jobject   convertToDirectBuffer(PyObject* src);

	virtual bool isSubTypeOf(JPClass* other) const override;

	template <class T> T assertRange(const T& l)
	{
		if (l < JPJni::s_Byte_Min || l > JPJni::s_Byte_Max)
		{
			JP_RAISE_OVERFLOW_ERROR("Cannot convert value to Java byte");
		}
		return l;
	}
} ;

#endif // _JPBYTE_TYPE_H_

