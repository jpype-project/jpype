/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include <utility>

#include <Python.h>
#include "jpype.h"
#include "jp_arrayclass.h"
#include "jp_classhints.h"
#include "jp_proxy.h"
#include "jp_stringtype.h"

#include "pyjp.h"

JPMatch::JPMatch()
{
	conversion = nullptr;
	frame = nullptr;
	object = nullptr;
	type = JPMatch::_none;
	slot = (JPValue*) - 1;
	closure = nullptr;
}

JPMatch::JPMatch(JPJavaFrame *fr, PyObject *obj)
{
	conversion = nullptr;
	frame = fr;
	object = obj;
	type = JPMatch::_none;
	slot = (JPValue*) - 1;
	closure = nullptr;
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
	if (conversion == nullptr)
		JP_RAISE(PyExc_SystemError, "Fail in conversion"); // GCOVR_EXCL_LINE
	return conversion->convert(*this);
}

JPMethodMatch::JPMethodMatch(JPJavaFrame &frame, JPPyObjectVector& args, bool callInstance)
: m_Arguments(args.size())
{
	m_Type = JPMatch::_none;
	m_IsVarIndirect = false;
	m_Overload = nullptr;
	m_Offset = 0;
	m_Skip = 0;
	m_Hash = callInstance ? 0 : 1000;
	for (size_t i = 0; i < args.size(); ++i)
	{
		PyObject *arg = args[i];
		m_Arguments[i] = JPMatch(&frame, arg);

		// This is an LCG used to compute a hash code for the incoming
		// arguments using (A*X+A_i) mod2^64 where A_i is the address of each
		// type the argument list.  The hash will be checked to avoid needing
		// to resolve the method if the same overload is called twice. There
		// is only a speed cost if there is a collision, so we don't need to
		// prove this is a perfect hash function.
		m_Hash *= 0x10523C01;
		m_Hash += (long) (Py_TYPE(arg));
	}
}

JPConversion::~JPConversion() = default;
JPClassHints::JPClassHints()
{
	m_ConvertJava = false;
}

JPClassHints::~JPClassHints()
{
	for (auto & conversion : conversions)
	{
		delete conversion;
	}
	conversions.clear();
}

JPMatch::Type JPClassHints::getConversion(JPMatch& match, JPClass *cls)
{
	JPConversion *best = nullptr;
	for (auto & conversion : conversions)
	{
		JPMatch::Type quality = conversion->matches(cls, match);
		if (quality > JPMatch::_explicit)
			return match.type;
		if (quality != JPMatch::_none)
			best = conversion;
	}
	match.conversion = best;
	if (best == nullptr)
		return match.type = JPMatch::_none;
	return match.type = JPMatch::_explicit;
}

void JPIndexConversion::getInfo(JPClass *cls, JPConversionInfo &info)
{
	PyObject *typing = PyImport_AddModule("jpype.protocol");
	JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsIndex"));
	PyList_Append(info.implicit, proto.get());
}

void JPNumberConversion::getInfo(JPClass *cls, JPConversionInfo &info)
{
	JPIndexConversion::getInfo(cls, info);
	PyObject *typing = PyImport_AddModule("jpype.protocol");
	JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsFloat"));
	PyList_Append(info.implicit, proto.get());
}

/**
 * Conversion for all user specified conversions.
 */
class JPPythonConversion : public JPConversion
{
public:

	explicit JPPythonConversion(PyObject *method)
	{
		method_ = JPPyObject::use(method);
	}

	~JPPythonConversion() override = default;

	jvalue convert(JPMatch &match) override
	{
		JP_TRACE_IN("JPPythonConversion::convert");
		JPClass *cls = ((JPClass*) match.closure);
		JPPyObject args = JPPyTuple_Pack(cls->getHost(), match.object);
		JPPyObject ret = JPPyObject::call(PyObject_Call(method_.get(), args.get(), nullptr));
		JPValue *value = PyJPValue_getJavaSlot(ret.get());
		if (value != nullptr)
		{
			jvalue v = value->getValue();
			JP_TRACE("Value", v.l);
			v.l = match.frame->NewLocalRef(v.l);
			return v;
		}
		JPProxy *proxy = PyJPProxy_getJPProxy(ret.get());
		if (proxy != nullptr)
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

	JPAttributeConversion(string attribute, PyObject *method)
	: JPPythonConversion(method), attribute_(std::move(attribute))
	{
	}

	~JPAttributeConversion() override = default;

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPAttributeConversion::matches");
		JPPyObject attr = JPPyObject::accept(PyObject_GetAttrString(match.object, attribute_.c_str()));
		if (attr.isNull())
			return JPMatch::_none;
		match.conversion = this;
		match.closure = cls;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.attributes, JPPyString::fromStringUTF8(attribute_).get());
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

class JPNoneConversion : public JPConversion
{
public:

	explicit JPNoneConversion(PyObject *type)
	{
		type_ = JPPyObject::use(type);
	}

	~JPNoneConversion() override
	= default;

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPTypeConversion::matches");
		if (!PyObject_IsInstance(match.object, type_.get()))
			return JPMatch::_none;
		match.closure = cls;
		match.conversion = this;
		match.type = JPMatch::_none;
		return JPMatch::_implicit; // Prevent further searching
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.none, type_.get());
	}

	jvalue convert(JPMatch &match) override
	{
		return jvalue();
	}

private:
	JPPyObject type_;
} ;

class JPTypeConversion : public JPPythonConversion
{
public:

	JPTypeConversion(PyObject *type, PyObject *method, bool exact)
	: JPPythonConversion(method), exact_(exact)
	{
		type_ = JPPyObject::use(type);
	}

	~JPTypeConversion() override
	= default;

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
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

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, type_.get());
	}

private:
	JPPyObject type_;
	bool exact_;
} ;

void JPClassHints::addTypeConversion(PyObject *type, PyObject *method, bool exact)
{
	JP_TRACE_IN("JPClassHints::addTypeConversion", this);
	if (PyJPClass_Check(type))
		m_ConvertJava = true;
	conversions.push_back(new JPTypeConversion(type, method, exact));
	JP_TRACE_OUT;
}

void JPClassHints::excludeConversion(PyObject *type)
{
	JP_TRACE_IN("JPClassHints::addTypeConversion", this);
	conversions.push_front(new JPNoneConversion(type));
	JP_TRACE_OUT;
}

void JPClassHints::getInfo(JPClass *cls, JPConversionInfo &info)
{
	for (auto iter = conversions.begin();
			iter != conversions.end(); ++iter)
	{
		(*iter)->getInfo(cls, info);
	}
}

class JPHintsConversion : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		auto *pyhints = (PyJPClassHints*) cls->getHints();
		JPClassHints *hints = pyhints->m_Hints;
		hints->getConversion(match, cls);
		return match.type;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		auto *pyhints = (PyJPClassHints*) cls->getHints();
		JPClassHints *hints = pyhints->m_Hints;
		hints->getInfo(cls, info);
	}

	jvalue convert(JPMatch &match) override
	{
		return jvalue();
	}
} _hintsConversion;

//</editor-fold>

class JPConversionCharArray : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionCharArray::matches");
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (match.frame == nullptr  || !JPPyString::check(match.object) ||
				acls->getComponentType() != JPContext_global->_char)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (acls->getComponentType() != JPContext_global->_char)
			return;
		PyList_Append(info.implicit, (PyObject*) & PyUnicode_Type);
	}

	jvalue convert(JPMatch &match) override
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

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionByteArray::matches");
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (match.frame == nullptr  || !PyBytes_Check(match.object) ||
				acls->getComponentType() != JPContext_global->_byte)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (acls->getComponentType() != JPContext_global->_byte)
			return;
		PyList_Append(info.implicit, (PyObject*) & PyBytes_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		JPJavaFrame frame(*match.frame);
		jvalue res;
		Py_ssize_t size = 0;
		char *buffer = nullptr;
		PyBytes_AsStringAndSize(match.object, &buffer, &size); // internal reference
		jbyteArray byteArray = frame.NewByteArray((jsize) size);
		frame.SetByteArrayRegion(byteArray, 0, (jsize) size, (jbyte*) buffer);
		res.l = frame.keep(byteArray);
		return res;
	}
} _byteArrayConversion;

class JPConversionBuffer : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionBuffer::matches");
		auto *acls = dynamic_cast<JPArrayClass*>( cls);
		JPClass *componentType = acls->getComponentType();
		if ( !componentType->isPrimitive())
			return match.type = JPMatch::_none;
		// If is isn't a buffer we can skip
		JPPyBuffer	buffer(match.object, PyBUF_ND | PyBUF_FORMAT);
		if (!buffer.valid())
		{
			PyErr_Clear();
			return match.type = JPMatch::_none;
		}

		// If it is a buffer we only need to test the first item in the list
		JPPySequence seq = JPPySequence::use(match.object);
		jlong length = seq.size();
		if (length == -1 && PyErr_Occurred())
		{
			PyErr_Clear();
			return match.type = JPMatch::_none;
		}
		match.type = JPMatch::_implicit;
		if (length > 0)
		{
			JPPyObject item = seq[0];
			JPMatch imatch(match.frame, item.get());
			componentType->findJavaConversion(imatch);
			if (imatch.type < match.type)
				match.type = imatch.type;
		}
		match.closure = cls;
		match.conversion = bufferConversion;
		return match.type;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		// This will be covered by Sequence
	}

	jvalue convert(JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionBuffer::convert");
		JPJavaFrame frame(*match.frame);
		jvalue res;
		auto *acls = (JPArrayClass *) match.closure;
		auto length = (jsize) PySequence_Length(match.object);
		JPClass *ccls = acls->getComponentType();
		jarray array = ccls->newArrayOf(frame, (jsize) length);
		ccls->setArrayRange(frame, array, 0, length, 1, match.object);
		res.l = frame.keep(array);
		return res;
		JP_TRACE_OUT;
	}
}  _bufferConversion;

class JPConversionSequence : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionSequence::matches");
		if ( !PySequence_Check(match.object) || JPPyString::check(match.object))
			return match.type = JPMatch::_none;
		auto *acls = dynamic_cast<JPArrayClass*>( cls);
		JPClass *componentType = acls->getComponentType();
		JPPySequence seq = JPPySequence::use(match.object);
		jlong length = seq.size();
		if (length==-1 && PyErr_Occurred())
		{
			PyErr_Clear();
			return match.type = JPMatch::_none;
		}
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

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "Sequence"));
		PyList_Append(info.implicit, proto.get());
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (acls->getComponentType() == JPContext_global->_char)
			return;
		PyList_Append(info.none, (PyObject*) & PyUnicode_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		JPJavaFrame frame(*match.frame);
		jvalue res;
		auto *acls = (JPArrayClass *) match.closure;
		auto length = (jsize) PySequence_Length(match.object);
		JPClass *ccls = acls->getComponentType();
		jarray array = ccls->newArrayOf(frame, (jsize) length);
		ccls->setArrayRange(frame, array, 0, length, 1, match.object);
		res.l = frame.keep(array);
		return res;
	}
} _sequenceConversion;

class JPConversionNull : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionNull::matches");
		if (match.object != Py_None)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue v;
		v.l = nullptr;
		return v;
	}
} _nullConversion;

class JPConversionClass : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionClass::matches");
		if (match.frame == nullptr)
			return match.type = JPMatch::_none;
		JPClass* cls2 = PyJPClass_getJPClass(match.object);
		if (cls2 == nullptr)
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.closure = cls2;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		PyList_Append(info.implicit, (PyObject*) PyJPClass_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		auto* cls2 = (JPClass*) match.closure;
		res.l = match.frame->NewLocalRef(cls2->getJavaClass());
		return res;
	}
} _classConversion;

class JPConversionObject : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionObject::matches");
		JPValue *value = match.getJavaSlot();
		if (value == nullptr || match.frame == nullptr)
			return match.type = JPMatch::_none;
		match.conversion = this;
		JPClass *oc = value->getClass();
		if (oc == nullptr)
			return match.type = JPMatch::_none;
		if (oc == cls)
		{
			// hey, this is me! :)
			return match.type = JPMatch::_exact;
		}
		bool assignable = match.frame->IsAssignableFrom(oc->getJavaClass(), cls->getJavaClass()) != 0;
		JP_TRACE("assignable", assignable, oc->getCanonicalName(), cls->getCanonicalName());
		match.type = (assignable ? JPMatch::_derived : JPMatch::_none);

		// User has request a Java class to class conversion.  We must pass through check it.
		if (!assignable)
		{
			auto *pyhints = (PyJPClassHints*) cls->getHints();
			JPClassHints *hints = pyhints->m_Hints;
			if (hints->m_ConvertJava)
				return match.type;
		}

		// This is the one except to the conversion rule patterns.
		// If it is a Java value then we must prevent it from proceeding
		// through the conversion rules even if it was not a match.
		// Thus the return result and the match type differ here.
		return JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		PyList_Append(info.exact, PyJPClass_create(frame, cls).get());
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		JPValue* value = match.getJavaSlot();
		res.l = match.frame->NewLocalRef(value->getValue().l);
		return res;
	}
} _objectConversion;

JPMatch::Type JPConversionJavaValue::matches(JPClass *cls, JPMatch &match)
{
	JP_TRACE_IN("JPConversionJavaValue::matches");
	JPValue *slot = match.getJavaSlot();
	if (slot == nullptr || slot->getClass() != cls)
		return match.type = JPMatch::_none;
	match.conversion = this;
	return match.type = JPMatch::_exact;
	JP_TRACE_OUT;
}

void JPConversionJavaValue::getInfo(JPClass *cls, JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	PyList_Append(info.exact, PyJPClass_create(frame, cls).get());
}

jvalue JPConversionJavaValue::convert(JPMatch &match)
{
	JPValue* value = match.getJavaSlot();
	return *value;
}

JPConversionJavaValue _javaValueConversion;

class JPConversionString : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionString::matches");
		if (match.frame == nullptr || !JPPyString::check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		if (cls == JPContext_global->_java_lang_String)
			return match.type = JPMatch::_exact;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, (PyObject*) & PyUnicode_Type);
	}

	jvalue convert(JPMatch &match) override
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

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		JPPyObjectVector args(match.object, nullptr);
		auto *cls = (JPClass*) match.closure;
		JPValue pobj = cls->newInstance(*match.frame, args);
		res.l = pobj.getJavaObject();
		return res;
	}
} ;

class JPConversionBoxBoolean : public JPConversionBox
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match)  override
	{
		JP_TRACE_IN("JPConversionBoxBoolean::matches");
		if (!PyBool_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.closure = JPContext_global->_java_lang_Boolean;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, (PyObject*) & PyBool_Type);
	}

} _boxBooleanConversion;

class JPConversionBoxLong : public JPConversionBox
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match)  override
	{
		JP_TRACE_IN("JPConversionBoxLong::matches");
		if (match.frame == nullptr)
			return match.type = JPMatch::_none;
		if (PyLong_CheckExact(match.object) || PyIndex_Check(match.object))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsIndex"));
		PyList_Append(info.implicit, proto.get());
	}

	jvalue convert(JPMatch &match) override
	{
		PyTypeObject* type = Py_TYPE(match.object);
		const char *name = type->tp_name;
		match.closure = JPContext_global->_java_lang_Long;
		if (strncmp(name, "numpy", 5) == 0)
		{
			// We only handle specific sized types, all others go to long.
			if (strcmp(&name[5], ".int8") == 0)
				match.closure = JPContext_global->_java_lang_Byte;
			else if (strcmp(&name[5], ".int16") == 0)
				match.closure = JPContext_global->_java_lang_Short;
			else if (strcmp(&name[5], ".int32") == 0)
				match.closure = JPContext_global->_java_lang_Integer;
		}
		return JPConversionBox::convert(match);
	}
} _boxLongConversion;

class JPConversionBoxDouble : public JPConversionBox
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionBoxDouble::matches");
		if (match.frame == nullptr)
			return match.type = JPMatch::_none;
		if (PyNumber_Check(match.object))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsFloat"));
		PyList_Append(info.implicit, proto.get());
	}

	jvalue convert(JPMatch &match) override
	{
		PyTypeObject* type = Py_TYPE(match.object);
		const char *name = type->tp_name;
		match.closure = JPContext_global->_java_lang_Double;
		if (strncmp(name, "numpy", 5) == 0)
		{
			// We only handle specific sized types, all others go to double.
			if (strcmp(&name[5], ".float32") == 0)
				match.closure = JPContext_global->_java_lang_Float;
		}
		return JPConversionBox::convert(match);
	}
} _boxDoubleConversion;

class JPConversionJavaObjectAny : public JPConversionBox
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionJavaObjectAny::matches");
		JPValue *value = match.getJavaSlot();
		if (value == nullptr || match.frame == nullptr || value->getClass() == nullptr)
			return match.type = JPMatch::_none;
		match.conversion = this;
		if (value->getClass()->isPrimitive())
			match.type = JPMatch::_implicit;
		else if (value->getClass() == cls)
			match.type = JPMatch::_exact;
		else
			match.type = JPMatch::_derived;
		return match.type;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		PyList_Append(info.implicit, PyJPClass_create(frame, JPContext_global->_java_lang_Object).get());
	}

	jvalue convert(JPMatch &match) override
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
			auto* type = dynamic_cast<JPPrimitiveType*> (value->getClass());
			match.closure = type->getBoxedClass(*frame);
			res = JPConversionBox::convert(match);
			return res;
		}
	}
} _javaObjectAnyConversion;

class JPConversionJavaNumberAny : public JPConversionJavaObjectAny
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionJavaNumberAny::matches");
		JPContext *context = JPContext_global;
		JPValue *value = match.getJavaSlot();
		// This converter only works for number types, thus boolean and char
		// are excluded.
		if (value == nullptr || match.frame == nullptr || value->getClass() == nullptr
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
		// Otherwise, check if it is assignable according to Java
		bool assignable = match.frame->IsAssignableFrom(oc->getJavaClass(), cls->getJavaClass()) != 0;
		return match.type = (assignable ? JPMatch::_implicit : JPMatch::_none);
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, (PyObject*) PyJPNumberLong_Type);
		PyList_Append(info.implicit, (PyObject*) PyJPNumberFloat_Type);
	}

} _javaNumberAnyConversion;

class JPConversionUnbox : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JPContext *context = JPContext_global;
		if (context == nullptr)
			return match.type = JPMatch::_none;
		JPValue *slot = match.slot;
		auto *pcls = dynamic_cast<JPPrimitiveType*>( cls);
		if (slot->getClass() != pcls->getBoxedClass(*match.frame))
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.closure = cls;
		return match.type = JPMatch::_implicit;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		auto *pcls = dynamic_cast<JPPrimitiveType*>( cls);
		PyList_Append(info.implicit,
				PyJPClass_create(frame, pcls->getBoxedClass(frame)).get());
	}

	jvalue convert(JPMatch &match) override
	{
		JPValue* value = match.getJavaSlot();
		auto *cls = (JPClass*) match.closure;
		return cls->getValueFromObject(*match.frame, *value);
	}
} _unboxConversion;

class JPConversionProxy : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionProxy::matches");
		JPProxy* proxy = PyJPProxy_getJPProxy(match.object);
		if (proxy == nullptr || match.frame == nullptr)
			return match.type = JPMatch::_none;

		// Check if any of the interfaces matches ...
		vector<JPClass*> itf = proxy->getInterfaces();
		for (auto & i : itf)
		{
			if (match.frame->IsAssignableFrom(i->getJavaClass(), cls->getJavaClass()))
			{
				JP_TRACE("implicit proxy");
				match.conversion = this;
				return match.type = JPMatch::_implicit;
			}
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
	}

	jvalue convert(JPMatch &match) override
	{
		return PyJPProxy_getJPProxy(match.object)->getProxy();
	}
} _proxyConversion;

JPConversion *hintsConversion = &_hintsConversion;
JPConversion *charArrayConversion = &_charArrayConversion;
JPConversion *byteArrayConversion = &_byteArrayConversion;
JPConversion *bufferConversion = &_bufferConversion;
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
