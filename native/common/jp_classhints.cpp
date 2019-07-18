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
#include <jpype.h>
#include <jp_classhints.h>
#include <Python.h>

JPClassHints::JPClassHints()
{
}

JPClassHints::~JPClassHints()
{
}

JPConversion* JPClassHints::getConversion(JPJavaFrame &frame, JPClass* cls, PyObject* obj)
{
	JPConversion* best = NULL;
	JPMatch match;
	for (std::list<JPConversion*>::iterator iter = conversions.begin();
			iter != conversions.end(); ++iter)
	{
		JPMatch::Type quality = (*iter)->matches(match, frame, cls, obj);
		if (quality > JPMatch::_explicit)
			return (*iter);
		if (quality != JPMatch::_none)
			best = (*iter);
	}
	return best;
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

	virtual jvalue convert(JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		JPPyTuple args(JPPyTuple::newTuple(2));
		args.setItem(0, (PyObject*) frame.getContext()->getHost());
		args.setItem(1, (PyObject*) pyobj);
		JPPyObject ret = method_.call(args.get(), NULL);
		JPValue *value = JPPythonEnv::getJavaValue(ret.get());
		if (value == NULL)
			JP_RAISE_TYPE_ERROR("Bad type conversion");
		return value->getValue();
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

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame &frame, JPClass *cls, PyObject *obj) override
	{
		if (JPPyObject::hasAttrString(obj, attribute_.c_str())) 
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return JPMatch::_none;
	}

private:
	std::string attribute_;

} ;

void JPClassHints::addAttributeConversion(const string &attribute, PyObject *conversion)
{
	conversions.push_back(new JPAttributeConversion(attribute, conversion));
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

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		if ((exact_ && ((PyObject*)Py_TYPE(pyobj)) == type_.get())
				|| JPPyObject::isInstance(pyobj, type_.get()))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return JPMatch::_none;
	}

private:
	JPPyObject type_;
	bool exact_;
} ;

void JPClassHints::addTypeConversion(PyObject *type, PyObject *method, bool exact)
{
	conversions.push_back(new JPTypeConversion(type, method, exact));
}
//</editor-fold>

class JPConversionNull : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		if (!JPPyObject::isNone(pyobj))
			return JPMatch::_none;
		match.conversion = this;
		match.type = JPMatch::_implicit;
		return match.type;
	}

	virtual jvalue convert(JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue v;
		v.l = NULL;
		return v;
	}
} _nullConversion;

class JPConversionClass : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		if (JPPythonEnv::getJavaClass(pyobj) == NULL)
			return JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
	}

	virtual jvalue convert(JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		JPClass* cls2 = JPPythonEnv::getJavaClass(pyobj);
		if (cls2 != NULL)
		{
			res.l = frame.NewLocalRef(cls2->getJavaClass());
			return res;
		}
	}
} _classConversion;

class JPConversionObject : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		JPValue *value = JPPythonEnv::getJavaValue(pyobj);
		if (value == NULL)
			return JPMatch::_none;
		JPClass *oc = value->getClass();
		if (oc == cls)
		{
			// hey, this is me! :)
			match.conversion = this;
			return match.type = JPMatch::_exact;
		}
		if (!frame.IsAssignableFrom(oc->getJavaClass(), cls->getJavaClass()))
			return JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
	}

	virtual jvalue convert(JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		JPValue* value = JPPythonEnv::getJavaValue(pyobj);
		if (cls != NULL)
		{
			res.l = frame.NewLocalRef(value->getValue().l);
			return res;
		}
	}
} _objectConversion;

class JPConversionJavaObjectAny : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		JPValue *value = JPPythonEnv::getJavaValue(pyobj);
		if (value == NULL)
			return JPMatch::_none;
		match.conversion = this;
		match.type = (value->getClass() == cls) ? JPMatch::_exact : JPMatch::_implicit;
		return match.type;
	}

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		jvalue res;
		JPValue *value = JPPythonEnv::getJavaValue(pyobj);
		if (dynamic_cast<JPPrimitiveType*> (value->getClass()) == NULL)
		{
			res.l = frame.NewLocalRef(value->getJavaObject());
			return res;
		}
		else
		{
			// Okay we need to box it.
			JPPrimitiveType* type = (JPPrimitiveType*) (value->getClass());
			res = boxConversion->convert(frame, type->getBoxedClass(), pyobj);
			return res;
		}
	}
} _javaObjectAnyConversion;

class JPConversionJavaValue : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		JPValue* value = JPPythonEnv::getJavaValue(pyobj);
		return *value;
	}
} _javaValueConversion;

class JPConversionString : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		if (!JPPyString::check(pyobj))
			return JPMatch::_none;
		match.conversion = this;
		match.type = JPMatch::_implicit;
		return match.type;
	}

	virtual jvalue convert(JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		string str = JPPyString::asStringUTF8(pyobj);
		res.l = frame.getContext()->fromStringUTF8(str);
		return res;
	}
} _stringConversion;


class JPConversionBox : public JPConversion
{
	typedef JPIntType base_t;
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		jvalue res;
		JPPyObjectVector args(pyobj, NULL);
		JPValue pobj = cls->newInstance(args);
		res.l = pobj.getJavaObject();
		return res;
	}
} _boxConversion;

class JPConversionBoxBoolean : public JPConversion
{
public:

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		return boxConversion->convert(frame, frame.getContext()->_java_lang_Boolean, pyobj);
	}
} _boxBooleanConversion;

class JPConversionBoxLong : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPContext* context, JPClass* cls, PyObject* pyobj)
	{
		if (JPPyLong::checkConvertable(pyobj) && JPPyLong::checkIndexable(pyobj))
		{
			match.conversion = this;
			return JPMatch::_implicit;
		}
		return JPMatch::_none;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		return boxConversion->convert(frame, frame.getContext()->_java_lang_Long, pyobj);
	}
} _boxLongConversion;

class JPConversionBoxDouble : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		if (JPPyLong::checkConvertable(pyobj) && JPPyLong::checkIndexable(pyobj))
		{
			match.conversion = this;
			return JPMatch::_implicit;
		}
		return JPMatch::_none;
	}

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		return boxConversion->convert(frame, frame.getContext()->_java_lang_Double, pyobj);
	}
} _boxDoubleConversion;

class JPConversionUnbox : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		JPValue* value = JPPythonEnv::getJavaValue(pyobj);
		return cls->getValueFromObject(value->getJavaObject());
	}
} _unboxConversion;

class JPConversionProxy : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		JPProxy* proxy = JPPythonEnv::getJavaProxy(pyobj);
		if (proxy == NULL)
			return JPMatch::_none;

		// Check if any of the interfaces matches ...
		vector<JPClass*> itf = proxy->getInterfaces();
		for (unsigned int i = 0; i < itf.size(); i++)
		{
			if (frame.IsAssignableFrom(itf[i]->getJavaClass(), cls->getJavaClass()))
			{
				JP_TRACE("implicit proxy");
				match.conversion = this;
				return match.type = JPMatch::_implicit;
			}
		}
		return JPMatch::_none;
	}

	virtual jvalue convert(JPJavaFrame &frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		JPProxy* proxy = JPPythonEnv::getJavaProxy(pyobj);
		res.l = proxy->getProxy();
		return res;
	}
} _proxyConversion;

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
