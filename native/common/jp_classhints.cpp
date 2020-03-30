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

JPMatch::JPMatch()
{
	conversion = NULL;
	frame = NULL;
	object = NULL;
	type = JPMatch::_none;
	slot = (JPValue*) - 1;
	closure = 0;
}

JPMatch::JPMatch(JPJavaFrame *fr, PyObject *obj)
{
	conversion = NULL;
	frame = fr;
	object = obj;
	type = JPMatch::_none;
	slot = (JPValue*) - 1;
	closure = 0;
}

JPValue *JPMatch::getJavaSlot()
{
	if (slot == (JPValue*) - 1)
		return slot = PyJPValue_getJavaSlot(object);
	return slot;
}

jvalue JPMatch::convert()
{
	// Sanity check, this should not happen
	if (conversion == NULL)
		JP_RAISE(PyExc_SystemError, "Fail in conversion"); // GCOVR_EXCL_LINE
	return conversion->convert(*this);
}

JPMethodMatch::JPMethodMatch(JPJavaFrame &frame, JPPyObjectVector& args)
: argument(args.size())
{
	type = JPMatch::_none;
	isVarIndirect = false;
	overload = 0;
	offset = 0;
	skip = 0;
	for (size_t i = 0; i < args.size(); ++i)
	{
		argument[i] = JPMatch(&frame, args[i]);
	}
}

JPConversion::~JPConversion()
{
}

JPClassHints::JPClassHints()
{
}

JPClassHints::~JPClassHints()
{
	for (std::list<JPConversion*>::iterator iter = conversions.begin();
			iter != conversions.end(); ++iter)
	{
		delete *iter;
	}
	conversions.clear();
}

JPMatch::Type JPClassHints::getConversion(JPMatch& match, JPClass *cls)
{
	JPConversion *best = NULL;
	for (std::list<JPConversion*>::iterator iter = conversions.begin();
			iter != conversions.end(); ++iter)
	{
		JPMatch::Type quality = (*iter)->matches(match, cls);
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

	virtual ~JPPythonConversion()  // GCOVR_EXCL_LINE
	{
	}

	virtual jvalue convert(JPMatch &match) override
	{
		JP_TRACE_IN("JPPythonConversion::convert");
		JPPyTuple args(JPPyTuple::newTuple(2));
		JPClass *cls = ((JPClass*) match.closure);
		args.setItem(0, (PyObject*) cls->getHost());
		args.setItem(1, (PyObject*) match.object);
		JPPyObject ret = JPPyObject(JPPyRef::_call,
				PyObject_Call(method_.get(), args.get(), NULL));
		JPValue *value = PyJPValue_getJavaSlot(ret.get());
		if (value != NULL)
		{
			jvalue v = value->getValue();
			JP_TRACE("Value", v.l);
			v.l = match.frame->NewLocalRef(v.l);
			return v;
		}
		JPProxy *proxy = PyJPProxy_getJPProxy(ret.get());
		if (proxy != NULL)
		{
			jvalue v = proxy->getProxy();
			JP_TRACE("Proxy", v.l);
			v.l = match.frame->NewLocalRef(v.l);
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

	virtual ~JPAttributeConversion()  // GCOVR_EXCL_LINE
	{
	}

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPAttributeConversion::matches");
		JPPyObject attr(JPPyRef::_accept, PyObject_GetAttrString(match.object, attribute_.c_str()));
		if (attr.isNull())
			return JPMatch::_none;
		match.conversion = this;
		match.closure = cls;
		return match.type = JPMatch::_implicit;
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

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPTypeConversion::matches");
		if ((exact_ && ((PyObject*) Py_TYPE(match.object)) == type_.get())
				|| PyObject_IsInstance(match.object, type_.get()))
		{
			match.closure = cls;
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

class JPHintsConversion : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls)
	{
		PyJPClassHints *pyhints = (PyJPClassHints*) cls->getHints();
		if (pyhints == NULL)
			return match.type = JPMatch::_none;
		JPClassHints *hints = pyhints->m_Hints;
		hints->getConversion(match, cls);
		return match.type;
	}

	virtual jvalue convert(JPMatch &match)
	{
		return jvalue();
	}
} _hintsConversion;

//</editor-fold>

class JPConversionCharArray : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionCharArray::matches");
		JPArrayClass* acls = (JPArrayClass*) cls;
		if (match.frame == NULL  || !JPPyString::check(match.object) ||
				acls->getComponentType() != match.getContext()->_char)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		JPJavaFrame *frame = match.frame;
		JP_TRACE("char[]");
		jvalue res;

		// Convert to a string
		string str = JPPyString::asStringUTF8(match.object);

		// Convert to new java string
		jstring jstr = frame->fromStringUTF8(str);

		// call toCharArray()
		res.l = frame->toCharArray(jstr);
		return res;
	}
} _charArrayConversion;

class JPConversionByteArray : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionByteArray::matches");
		JPArrayClass* acls = (JPArrayClass*) cls;
		if (match.frame == NULL  || !PyBytes_Check(match.object) ||
				acls->getComponentType() != match.frame->getContext()->_byte)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		JPJavaFrame frame(*match.frame);
		jvalue res;
		Py_ssize_t size = 0;
		char *buffer = NULL;
		PyBytes_AsStringAndSize(match.object, &buffer, &size); // internal reference
		jbyteArray byteArray = frame.NewByteArray((jsize) size);
		frame.SetByteArrayRegion(byteArray, 0, (jsize) size, (jbyte*) buffer);
		res.l = frame.keep(byteArray);
		return res;
	}
} _byteArrayConversion;

class JPConversionSequence : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionSequence::matches");
		if ( !PySequence_Check(match.object) || JPPyString::check(match.object))
			return match.type = JPMatch::_none;
		JPArrayClass *acls = (JPArrayClass*) cls;
		JPClass *componentType = acls->getComponentType();
		JPPySequence seq(JPPyRef::_use, match.object);
		jlong length = seq.size();
		match.type = JPMatch::_implicit;
		for (jlong i = 0; i < length && match.type > JPMatch::_none; i++)
		{
			// This is a special case.  Sequences produce new references
			// so we must hold the reference in a container while the
			// the match is caching it.
			JPPyObject item = seq[i];
			JPMatch imatch(match.frame, item.get());
			componentType->findJavaConversion(imatch);
			if (imatch.type < match.type)
				match.type = imatch.type;
		}
		match.closure = cls;
		match.conversion = sequenceConversion;
		return match.type;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		JPJavaFrame frame(*match.frame);
		jvalue res;
		JPArrayClass *acls = (JPArrayClass *) match.closure;
		JP_TRACE("sequence");
		JPPySequence seq(JPPyRef::_use, match.object);
		jsize length = (jsize) seq.size();

		jarray array = acls->getComponentType()->newArrayInstance(frame, (jsize) length);
		for (jsize i = 0; i < length; i++)
		{
			JPPyObject item = seq[i];
			acls->getComponentType()->setArrayItem(frame, array, i, item.get());
		}
		res.l = frame.keep(array);
		return res;
	}
} _sequenceConversion;

class JPConversionNull : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionNull::matches");
		if (match.object != Py_None)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		jvalue v;
		v.l = NULL;
		return v;
	}
} _nullConversion;

class JPConversionClass : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionClass::matches");
		if (match.frame == NULL)
			return match.type = JPMatch::_none;
		JPClass* cls2 = PyJPClass_getJPClass(match.object);
		if (cls2 == NULL)
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.closure = cls2;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		jvalue res;
		JPClass* cls2 = (JPClass*) match.closure;
		res.l = match.frame->NewLocalRef(cls2->getJavaClass());
		return res;
	}
} _classConversion;

class JPConversionObject : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionObject::matches");
		JPValue *value = match.getJavaSlot();
		if (value == NULL || match.frame == NULL)
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
		bool assignable = match.frame->IsAssignableFrom(oc->getJavaClass(), cls->getJavaClass()) != 0;
		JP_TRACE("assignable", assignable, oc->getCanonicalName(), cls->getCanonicalName());
		match.type = (assignable ? JPMatch::_implicit : JPMatch::_none);

		// This is the one except to the conversion rule patterns.
		// If it is a Java value then we must prevent it from proceeding
		// through the conversion rules even if it was not a match.
		// Thus the return result and the match type differ here.
		return JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		jvalue res;
		JPValue* value = match.getJavaSlot();
		res.l = match.frame->NewLocalRef(value->getValue().l);
		return res;
	}
} _objectConversion;

class JPConversionJavaValue : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionJavaValue::matches");
		JPValue *slot = match.getJavaSlot();
		if (slot == NULL || slot->getClass() != cls)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_exact;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		JPValue* value = match.getJavaSlot();
		return *value;
	}
} _javaValueConversion;

class JPConversionString : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionString::matches");
		if (match.frame == NULL || !JPPyString::check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		if (cls == match.getContext()->_java_lang_String)
			return match.type = JPMatch::_exact;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		jvalue res;
		string str = JPPyString::asStringUTF8(match.object);
		res.l = match.frame->fromStringUTF8(str);
		return res;
	}
} _stringConversion;

class JPConversionBox : public JPConversion
{
public:

	virtual jvalue convert(JPMatch &match) override
	{
		jvalue res;
		JPPyObjectVector args(match.object, NULL);
		JPClass *cls = (JPClass*) match.closure;
		JPValue pobj = cls->newInstance(*match.frame, args);
		res.l = pobj.getJavaObject();
		return res;
	}
} ;

class JPConversionBoxBoolean : public JPConversionBox
{
public:

	JPMatch::Type matches(JPMatch &match, JPClass *cls)  override
	{
		JP_TRACE_IN("JPConversionBoxBoolean::matches");
		if (!PyBool_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.closure = match.frame->getContext()->_java_lang_Boolean;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

} _boxBooleanConversion;

class JPConversionBoxLong : public JPConversionBox
{
public:

	JPMatch::Type matches(JPMatch &match, JPClass *cls)  override
	{
		JP_TRACE_IN("JPConversionBoxLong::matches");
		if (match.frame == NULL)
			return match.type = JPMatch::_none;
		if (PyLong_CheckExact(match.object) || PyIndex_Check(match.object))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	jvalue convert(JPMatch &match)  override
	{
		PyTypeObject* type = Py_TYPE(match.object);
		JPJavaFrame *frame = match.frame;
		const char *name = type->tp_name;
		match.closure = frame->getContext()->_java_lang_Long;
		if (strncmp(name, "numpy", 5) == 0)
		{
			// We only handle specific sized types, all others go to long.
			if (strcmp(&name[5], ".int8") == 0)
				match.closure = frame->getContext()->_java_lang_Byte;
			else if (strcmp(&name[5], ".int16") == 0)
				match.closure = frame->getContext()->_java_lang_Short;
			else if (strcmp(&name[5], ".int32") == 0)
				match.closure = frame->getContext()->_java_lang_Integer;
		}
		return JPConversionBox::convert(match);
	}
} _boxLongConversion;

class JPConversionBoxDouble : public JPConversionBox
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionBoxDouble::matches");
		if (match.frame == NULL)
			return match.type = JPMatch::_none;
		if (PyNumber_Check(match.object))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		JPJavaFrame *frame = match.frame;
		PyTypeObject* type = Py_TYPE(match.object);
		const char *name = type->tp_name;
		match.closure = frame->getContext()->_java_lang_Double;
		if (strncmp(name, "numpy", 5) == 0)
		{
			// We only handle specific sized types, all others go to double.
			if (strcmp(&name[5], ".float32") == 0)
				match.closure = frame->getContext()->_java_lang_Float;
		}
		return JPConversionBox::convert(match);
	}
} _boxDoubleConversion;

class JPConversionJavaObjectAny : public JPConversionBox
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionJavaObjectAny::matches");
		JPValue *value = match.getJavaSlot();
		if (value == NULL || match.frame == NULL || value->getClass() == NULL)
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.type = (value->getClass() == cls) ? JPMatch::_exact : JPMatch::_implicit;
		return match.type;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		jvalue res;
		JPJavaFrame *frame = match.frame;
		JPValue *value = match.getJavaSlot();
		if (!value->getClass()->isPrimitive())
		{
			res.l = frame->NewLocalRef(value->getJavaObject());
			return res;
		} else
		{
			// Okay we need to box it.
			JPPrimitiveType* type = (JPPrimitiveType*) (value->getClass());
			match.closure = type->getBoxedClass(frame->getContext());
			res = JPConversionBox::convert(match);
			return res;
		}
	}
} _javaObjectAnyConversion;

class JPConversionJavaNumberAny : public JPConversionJavaObjectAny
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionJavaNumberAny::matches");
		JPContext *context = match.getContext();
		JPValue *value = match.getJavaSlot();
		// This converter only works for number types, thus boolean and char
		// are excluded.
		if (value == NULL || match.frame == NULL || value->getClass() == NULL
				|| value->getClass() == context->_boolean
				|| value->getClass() == context->_char)
			return match.type = JPMatch::_none;
		match.conversion = this;
		JPClass *oc = value->getClass();
		// If it is the exact type, then it is exact
		if (oc == cls)
			return match.type = JPMatch::_exact;
		// If it is any primitive except char and boolean then implicit
		if (oc->isPrimitive())
			return match.type = JPMatch::_implicit;
		// Otherwise check if it is assignable according to Java
		bool assignable = match.frame->IsAssignableFrom(oc->getJavaClass(), cls->getJavaClass()) != 0;
		return match.type = (assignable ? JPMatch::_implicit : JPMatch::_none);
		JP_TRACE_OUT;
	}

} _javaNumberAnyConversion;

class JPConversionUnbox : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JPContext *context = match.getContext();
		if (context == NULL)
			return match.type = JPMatch::_none;
		JPValue *slot = match.slot;
		JPPrimitiveType *pcls = (JPPrimitiveType*) cls;
		if (slot->getClass() != pcls->getBoxedClass(context))
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.closure = cls;
		return match.type = JPMatch::_implicit;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		JPValue* value = match.getJavaSlot();
		JPClass *cls = (JPClass*) match.closure;
		return cls->getValueFromObject(*value);
	}
} _unboxConversion;

class JPConversionProxy : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		JP_TRACE_IN("JPConversionProxy::matches");
		JPProxy* proxy = PyJPProxy_getJPProxy(match.object);
		if (proxy == NULL || match.frame == NULL)
			return match.type = JPMatch::_none;

		// Check if any of the interfaces matches ...
		vector<JPClass*> itf = proxy->getInterfaces();
		for (unsigned int i = 0; i < itf.size(); i++)
		{
			if (match.frame->IsAssignableFrom(itf[i]->getJavaClass(), cls->getJavaClass()))
			{
				JP_TRACE("implicit proxy");
				match.conversion = this;
				return match.type = JPMatch::_implicit;
			}
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		return PyJPProxy_getJPProxy(match.object)->getProxy();
	}
} _proxyConversion;

JPConversion *hintsConversion = &_hintsConversion;
JPConversion *charArrayConversion = &_charArrayConversion;
JPConversion *byteArrayConversion = &_byteArrayConversion;
JPConversion *sequenceConversion = &_sequenceConversion;
JPConversion *nullConversion = &_nullConversion;
JPConversion *classConversion = &_classConversion;
JPConversion *objectConversion = &_objectConversion;
JPConversion *javaObjectAnyConversion = &_javaObjectAnyConversion;
JPConversion *javaNumberAnyConversion = &_javaNumberAnyConversion;
JPConversion *javaValueConversion = &_javaValueConversion;
JPConversion *stringConversion = &_stringConversion;
JPConversion *boxBooleanConversion = &_boxBooleanConversion;
JPConversion *boxLongConversion = &_boxLongConversion;
JPConversion *boxDoubleConversion = &_boxDoubleConversion;
JPConversion *unboxConversion = &_unboxConversion;
JPConversion *proxyConversion = &_proxyConversion;
