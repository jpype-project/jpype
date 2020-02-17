#ifndef JP_PRIMITIVE_ACCESSOR_H
#define JP_PRIMITIVE_ACCESSOR_H

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
	jint _commit;

public:

	JPPrimitiveArrayAccessor(JPJavaFrame& frame, jarray array, accessFnc access, releaseFnc release)
	: _frame(frame), _array((array_t) array), _release(release)
	{
		_commit = JNI_ABORT;
		_elem = ((&_frame)->*access)(_array, &_iscopy);
	}

	~JPPrimitiveArrayAccessor()
	{
		((&_frame)->*_release)(_array, _elem, _commit);
	}

	ptr_t get()
	{
		return _elem;
	}

	void commit()
	{
		_commit = 0;
	}
} ;

#endif /* JP_PRIMITIVE_ACCESSOR_H */

