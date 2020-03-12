#ifndef JP_PRIMITIVE_ACCESSOR_H
#define JP_PRIMITIVE_ACCESSOR_H

#include "jp_exception.h"
#include "jp_javaframe.h"

template <typename array_t, typename ptr_t>
class JPPrimitiveArrayAccessor
{
	typedef void (JPJavaFrame::*releaseFnc)(array_t, ptr_t, jint);
	typedef ptr_t (JPJavaFrame::*accessFnc)(array_t, jboolean*);

	JPJavaFrame& _frame;
	array_t _array;
	ptr_t _elem;
	releaseFnc _release;
	jboolean _iscopy;

public:

	JPPrimitiveArrayAccessor(JPJavaFrame& frame, jarray array, accessFnc access, releaseFnc release)
	: _frame(frame), _array((array_t) array), _release(release)
	{
		_elem = ((&_frame)->*access)(_array, &_iscopy);
	}

	~JPPrimitiveArrayAccessor()
	{
		// This is fallback if commit or abort is not called.
		// It should only occur in cases where a throw has
		// already been issued.
		try
		{
			if (_array)
				((&_frame)->*_release)(_array, _elem, JNI_ABORT);
		}		catch (JPypeException &ex)
		{
			// We can't throw here because it would abort.
			// But this is called on a non-op release, so
			// we will just eat it
		}
	}

	ptr_t get()
	{
		return _elem;
	}

	void commit()
	{
		// Prevent the dtor from calling a second time
		array_t a = _array;
		_array = 0;
		((&_frame)->*_release)(a, _elem, 0);
	}

	void abort()
	{
		// Prevent the dtor from calling a second time
		array_t a = _array;
		_array = 0;
		((&_frame)->*_release)(a, _elem, JNI_ABORT);
	}

} ;

template <typename base_t>
class JPConversionLong : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		jlong val = PyLong_AsLongLong(pyobj);
		if (val == -1)
			JP_PY_CHECK();
		base_t::field(res) = (typename base_t::type_t) base_t::assertRange(val);
		return res;
	}
} ;

template <typename base_t>
class JPConversionLongNumber : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		PyObject *obj = PyNumber_Long(pyobj);
		JP_PY_CHECK();
		jlong val = PyLong_AsLongLong(obj);
		Py_DECREF(obj);
		if (val == -1)
			JP_PY_CHECK();
		base_t::field(res) = (typename base_t::type_t) base_t::assertRange(val);
		return res;
	}
} ;

extern "C" JPValue* PyJPValue_getJavaSlot(PyObject* self);

template <typename base_t>
class JPConversionLongWiden : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JPValue *value = PyJPValue_getJavaSlot(pyobj);
		jvalue ret;
		base_t::field(ret) = (typename base_t::type_t) ((JPPrimitiveType*) value->getClass())->getAsLong(value->getValue());
		return ret;
	}
} ;


#endif /* JP_PRIMITIVE_ACCESSOR_H */

