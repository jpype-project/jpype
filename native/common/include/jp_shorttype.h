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
#ifndef _JP_SHORT_TYPE_H_
#define _JP_SHORT_TYPE_H_

class JPShortType : public JPPrimitiveType
{
public:

	JPShortType();
	virtual ~JPShortType();

	typedef jshort type_t;
	typedef jshortArray array_t;

	static inline jshort& field(jvalue& v)
	{
		return v.s;
	}

	static inline const jshort& field(const jvalue& v)
	{
		return v.s;
	}

	virtual JPClass* getBoxedClass(JPContext *context) const
	{
		return context->_java_lang_Short;
	}

	virtual JPMatch::Type findJavaConversion(JPMatch &match) override;
	virtual JPPyObject  convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) override;
	virtual JPValue     getValueFromObject(const JPValue& obj) override;

	virtual JPPyObject  invokeStatic(JPJavaFrame& frame, jclass, jmethodID, jvalue*) override;
	virtual JPPyObject  invoke(JPJavaFrame& frame, jobject, jclass, jmethodID, jvalue*) override;

	virtual JPPyObject  getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid) override;
	virtual void        setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* val) override;
	virtual JPPyObject  getField(JPJavaFrame& frame, jobject c, jfieldID fid) override;
	virtual void        setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* val) override;

	virtual jarray      newArrayInstance(JPJavaFrame& frame, jsize size) override;
	virtual void        setArrayRange(JPJavaFrame& frame, jarray,
			jsize start, jsize length, jsize step,
			PyObject *sequence) override;
	virtual JPPyObject  getArrayItem(JPJavaFrame& frame, jarray, jsize ndx) override;
	virtual void        setArrayItem(JPJavaFrame& frame, jarray, jsize ndx, PyObject* val) override;

	virtual char getTypeCode() override
	{
		return 'S';
	}

	virtual jlong getAsLong(jvalue v) override
	{
		return field(v);
	}

	virtual jdouble getAsDouble(jvalue v) override
	{
		return field(v);
	}

	template <class T> static T assertRange(const T& l)
	{
		if (l < -32768 || l > 32767)
		{
			JP_RAISE(PyExc_OverflowError, "Cannot convert value to Java short");
		}
		return l;
	}

	virtual void getView(JPArrayView& view) override;
	virtual void releaseView(JPArrayView& view) override;
	virtual const char* getBufferFormat() override;
	virtual ssize_t getItemSize() override;
	virtual void copyElements(JPJavaFrame &frame,
			jarray a, jsize start, jsize len,
			void* memory, int offset) override;

	virtual PyObject *newMultiArray(JPJavaFrame &frame,
			JPPyBuffer &buffer, int subs, int base, jobject dims) override;

} ;

#endif // _JP_SHORT_TYPE_H_
