/*****************************************************************************
   Copyright 2019 Karl Einar Nelson

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
#include <Python.h>
#include "jpype.h"
#include "jp_classhints.h"
#include "jp_arrayclass.h"
#include "jp_stringtype.h"
#include "pyjp.h"

JPConversion::~JPConversion()
{
}

JPClassHints::JPClassHints()
{
}

JPClassHints::~JPClassHints()
{
}

JPMatch::Type JPClassHints::getConversion(JPMatch& match, JPJavaFrame *frame, JPClass *cls, PyObject *obj)
{
	JPConversion *best = NULL;
	for (std::list<JPConversion*>::iterator iter = conversions.begin();
			iter != conversions.end(); ++iter)
	{
		JPMatch::Type quality = (*iter)->matches(match, frame, cls, obj);
		if (quality > JPMatch::_explicit)
			return match.type;
		if (quality != JPMatch::_none)
			best = (*iter);
	}
	match.conversion = best;
	if (best == NULL)
		return match.type = JPMatch::_none;
	return match.type = JPMatch::_explicit;
}

/**
 * Conversion for all user specified conversions.
 */
class JPPythonConversion : public JPConversion
{
public:

	JPPythonConversion(PyObject *method)
	: method_(JPPyRef::_use, method)
	{
	}

	virtual ~JPPythonConversion()
	{
	}

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPPythonConversion::convert");
		JPPyTuple args(JPPyTuple::newTuple(2));
		args.setItem(0, (PyObject*) cls->getHost());
		args.setItem(1, (PyObject*) pyobj);
		JPPyObject ret = JPPyObject(JPPyRef::_call,
				PyObject_Call(method_.get(), args.get(), NULL));
		JPValue *value = PyJPValue_getJavaSlot(ret.get());
		if (value != NULL)
		{
			jvalue v = value->getValue();
			JP_TRACE("Value", v.l);
			v.l = frame->NewLocalRef(v.l);
			return v;
		}
		JPProxy *proxy = PyJPProxy_getJPProxy(ret.get());
		if (proxy != NULL)
		{
			jvalue v = proxy->getProxy();
			JP_TRACE("Proxy", v.l);
			v.l = frame->NewLocalRef(v.l);
			return v;
		}
		JP_RAISE(PyExc_TypeError, "Bad type conversion");
		JP_TRACE_OUT;
	}
private:

	JPPyObject method_;
} ;

//<editor-fold desc="attribute conversion" defaultstate="collapsed">

class JPAttributeConversion : public JPPythonConversion
{
public:

	JPAttributeConversion(const string &attribute, PyObject *method)
	: JPPythonConversion(method), attribute_(attribute)
	{
	}

	virtual ~JPAttributeConversion()
	{
	}

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *obj) override
	{
		JP_TRACE_IN("JPAttributeConversion::matches");
		JPPyObject attr(JPPyRef::_accept, PyObject_GetAttrString(obj, attribute_.c_str()));
		if (!attr.isNull())
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return JPMatch::_none;
		JP_TRACE_OUT;
	}

private:
	std::string attribute_;

} ;

void JPClassHints::addAttributeConversion(const string &attribute, PyObject *conversion)
{
	JP_TRACE_IN("JPClassHints::addAttributeConversion", this);
	JP_TRACE(attribute);
	conversions.push_back(new JPAttributeConversion(attribute, conversion));
	JP_TRACE_OUT;
}

//</editor-fold>
//<editor-fold desc="type conversion" defaultstate="collapsed">

class JPTypeConversion : public JPPythonConversion
{
public:

	JPTypeConversion(PyObject *type, PyObject *method, bool exact)
	: JPPythonConversion(method), type_(JPPyRef::_use, type), exact_(exact)
	{
	}

	virtual ~JPTypeConversion()
	{
	}

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPTypeConversion::matches");
		if ((exact_ && ((PyObject*) Py_TYPE(pyobj)) == type_.get())
				|| PyObject_IsInstance(pyobj, type_.get()))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return JPMatch::_none;
		JP_TRACE_OUT;
	}

private:
	JPPyObject type_;
	bool exact_;
} ;

void JPClassHints::addTypeConversion(PyObject *type, PyObject *method, bool exact)
{
	JP_TRACE_IN("JPClassHints::addTypeConversion", this);
	conversions.push_back(new JPTypeConversion(type, method, exact));
	JP_TRACE_OUT;
}
//</editor-fold>

class JPConversionCharArray : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionCharArray::matches");
		JPArrayClass* acls = (JPArrayClass*) cls;
		if (frame == NULL  || !JPPyString::check(pyobj) ||
				acls->getComponentType() != frame->getContext()->_char)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JPContext *context = frame->getContext();
		JP_TRACE("char[]");
		jvalue res;

		// Convert to a string
		string str = JPPyString::asStringUTF8(pyobj);

		// Convert to new java string
		jstring jstr = frame->fromStringUTF8(str);

		// call toCharArray()
		res.l = frame->CallObjectMethodA(jstr, context->m_String_ToCharArrayID, 0);
		return res;
	}
} _charArrayConversion;

class JPConversionByteArray : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionByteArray::matches");
		JPArrayClass* acls = (JPArrayClass*) cls;
		if (frame == NULL  || !PyBytes_Check(pyobj) ||
				acls->getComponentType() != frame->getContext()->_byte)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPJavaFrame *frame2, JPClass *cls, PyObject *pyobj) override
	{
		JPJavaFrame frame(*frame2);
		jvalue res;
		Py_ssize_t size = 0;
		char *buffer = NULL;

		if (PyBytes_Check(pyobj))
		{
			PyBytes_AsStringAndSize(pyobj, &buffer, &size); // internal reference
		}
		jbyteArray byteArray = frame.NewByteArray((jsize) size);
		frame.SetByteArrayRegion(byteArray, 0, (jsize) size, (jbyte*) buffer);
		res.l = frame.keep(byteArray);
		return res;
	}
} _byteArrayConversion;

class JPConversionSequence : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame *frame2, JPClass *cls, PyObject *pyobj) override
	{
		JPJavaFrame frame(*frame2);
		jvalue res;
		JPArrayClass *acls = (JPArrayClass *) cls;
		JP_TRACE("sequence");
		JPPySequence seq(JPPyRef::_use, pyobj);
		jsize length = (jsize) seq.size();

		jarray array = acls->getComponentType()->newArrayInstance(frame, (jsize) length);
		for (jsize i = 0; i < length; i++)
		{
			acls->getComponentType()->setArrayItem(frame, array, i, seq[i].get());
		}
		res.l = frame.keep(array);
		return res;
	}
} _sequenceConversion;

class JPConversionNull : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionNull::matches");
		if (!JPPyObject::isNone(pyobj))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue v;
		v.l = NULL;
		return v;
	}
} _nullConversion;

class JPConversionClass : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionClass::matches");
		if (frame == NULL)
			return match.type = JPMatch::_none;
		JPClass* cls2 = PyJPClass_getJPClass(pyobj);
		if (cls2 == NULL)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		JPClass* cls2 = PyJPClass_getJPClass(pyobj);
		if (cls2 != NULL)
		{
			res.l = frame->NewLocalRef(cls2->getJavaClass());
			return res;
		}
		JP_RAISE(PyExc_TypeError, "Python object is not a Java class");
	}
} _classConversion;

class JPConversionObject : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionObject::matches");
		JPValue *value = PyJPValue_getJavaSlot(pyobj);
		if (value == NULL || frame == NULL)
			return match.type = JPMatch::_none;
		match.conversion = this;
		JPClass *oc = value->getClass();
		if (oc == NULL)
			return match.type = JPMatch::_none;
		if (oc == cls)
		{
			// hey, this is me! :)
			return match.type = JPMatch::_exact;
		}
		bool assignable = frame->IsAssignableFrom(oc->getJavaClass(), cls->getJavaClass()) != 0;
		JP_TRACE("assignable", assignable, oc->getCanonicalName(), cls->getCanonicalName());
		match.type = (assignable ? JPMatch::_implicit : JPMatch::_none);

		// This is the one except to the conversion rule patterns.
		// If it is a Java value then we must prevent it from proceeding
		// through the conversion rules even if it was not a match.
		// Thus the return result and the match type differ here.
		return JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		JPValue* value = PyJPValue_getJavaSlot(pyobj);
		if (cls != NULL)
		{
			res.l = frame->NewLocalRef(value->getValue().l);
			return res;
		}
		JP_RAISE(PyExc_TypeError, "Python object is not a Java value");
	}
} _objectConversion;

class JPConversionJavaObjectAny : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionJavaObjectAny::matches");
		JPValue *value = PyJPValue_getJavaSlot(pyobj);
		if (value == NULL || frame == NULL || value->getClass() == NULL)
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.type = (value->getClass() == cls) ? JPMatch::_exact : JPMatch::_implicit;
		return match.type;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPJavaFrame *frame, JPClass* cls, PyObject* pyobj) override
	{
		jvalue res;
		JPValue *value = PyJPValue_getJavaSlot(pyobj);
		if (value == NULL)
			JP_RAISE(PyExc_TypeError, "Python object is not a Java value");
		if (!value->getClass()->isPrimitive())
		{
			res.l = frame->NewLocalRef(value->getJavaObject());
			return res;
		} else
		{
			// Okay we need to box it.
			JPPrimitiveType* type = (JPPrimitiveType*) (value->getClass());
			res = boxConversion->convert(frame, type->getBoxedClass(frame->getContext()), pyobj);
			return res;
		}
	}
} _javaObjectAnyConversion;

class JPConversionJavaValue : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JPValue* value = PyJPValue_getJavaSlot(pyobj);
		return *value;
	}
} _javaValueConversion;

class JPConversionString : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionString::matches");
		if (!JPPyString::check(pyobj) || frame == NULL)
			return match.type = JPMatch::_none;
		match.conversion = this;
		if (cls == frame->getContext()->_java_lang_String)
			return match.type = JPMatch::_exact;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		string str = JPPyString::asStringUTF8(pyobj);
		res.l = frame->fromStringUTF8(str);
		return res;
	}
} _stringConversion;

class JPConversionBox : public JPConversion
{
	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionBox::convert");
		jvalue res;
		JPPyObjectVector args(pyobj, NULL);
		JPValue pobj = cls->newInstance(*frame, args);
		res.l = pobj.getJavaObject();
		return res;
		JP_TRACE_OUT;
	}
} _boxConversion;

class JPConversionBoxBoolean : public JPConversion
{
public:

	jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		return boxConversion->convert(frame, frame->getContext()->_java_lang_Boolean, pyobj);
	}
} _boxBooleanConversion;

class JPConversionBoxLong : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject* pyobj)  override
	{
		JP_TRACE_IN("JPConversionBoxLong::matches");
		if (frame == NULL)
			return match.type = JPMatch::_none;
		if (PyLong_CheckExact(pyobj) || PyIndex_Check(pyobj))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj)  override
	{
		return boxConversion->convert(frame, frame->getContext()->_java_lang_Long, pyobj);
	}
} _boxLongConversion;

class JPConversionBoxDouble : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionBoxDouble::matches");
		if (frame == NULL)
			return match.type = JPMatch::_none;
		if (PyLong_CheckExact(pyobj) || PyIndex_Check(pyobj))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		return boxConversion->convert(frame, frame->getContext()->_java_lang_Double, pyobj);
	}
} _boxDoubleConversion;

class JPConversionUnbox : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JPValue* value = PyJPValue_getJavaSlot(pyobj);
		return cls->getValueFromObject(*value);
	}
} _unboxConversion;

class JPConversionProxy : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JP_TRACE_IN("JPConversionProxy::matches");
		JPProxy* proxy = PyJPProxy_getJPProxy(pyobj);
		if (proxy == NULL || frame == NULL)
			return match.type = JPMatch::_none;

		// Check if any of the interfaces matches ...
		vector<JPClass*> itf = proxy->getInterfaces();
		for (unsigned int i = 0; i < itf.size(); i++)
		{
			if (frame->IsAssignableFrom(itf[i]->getJavaClass(), cls->getJavaClass()))
			{
				JP_TRACE("implicit proxy");
				match.conversion = this;
				return match.type = JPMatch::_implicit;
			}
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		return PyJPProxy_getJPProxy(pyobj)->getProxy();
	}
} _proxyConversion;


JPConversion *charArrayConversion = &_charArrayConversion;
JPConversion *byteArrayConversion = &_byteArrayConversion;
JPConversion *sequenceConversion = &_sequenceConversion;
JPConversion *nullConversion = &_nullConversion;
JPConversion *classConversion = &_classConversion;
JPConversion *objectConversion = &_objectConversion;
JPConversion *javaObjectAnyConversion = &_javaObjectAnyConversion;
JPConversion *javaValueConversion = &_javaValueConversion;
JPConversion *stringConversion = &_stringConversion;
JPConversion *boxConversion = &_boxConversion;
JPConversion *boxBooleanConversion = &_boxBooleanConversion;
JPConversion *boxLongConversion = &_boxLongConversion;
JPConversion *boxDoubleConversion = &_boxDoubleConversion;
JPConversion *unboxConversion = &_unboxConversion;
JPConversion *proxyConversion = &_proxyConversion;
