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

JPConversion* JPClassHints::getConversion(JPContext* context, JPClass* cls, PyObject* obj)
{
	JPConversion* best = NULL;
	for (std::list<JPConversion*>::iterator iter = conversions.begin();
		iter != conversions.end(); ++iter)
	{
		JPMatch::Type match = (*iter)->matches(context, cls, obj);
		if (match > JPMatch._explicit)
			return (*iter);
		if (match != JPMatch::_none)
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

	JPPythonConversion(PyObject* method)
	: method_(JPPyRef::_use, method)
	{
	}

	virtual ~JPPythonConversion()
	{
	}

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* obj) override
	{
		JPPyTuple args(JPPyTuple::newTuple(2));
		args.setItem(0, (PyObject*) frame.getContext()->getHost());
		args.setItem(1, (PyObject*) obj);
		JPPyObject obj = method_.call(args.get(), NULL);
		JPValue* value = JPPythonEnv::getJavaValue(obj.get());
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

	JPAttributeConversion(const string& attribute, PyObject* method)
	: JPPythonConversion(method), attribute_(attribute)
	{
	}

	virtual ~JPAttributeConversion()
	{
	}

	virtual JPMatch::Type matches(JPContext* context, JPClass* cls, PyObject* obj) override
	{
		return (JPPyObject::hasAttrString(obj, attribute_.c_str())) ? JPMatch::_implicit : JPMatch::_none;
	}

private:
	std::string attribute_;

} ;

void JPClassHints::addAttributeConversion(const string& attribute, PyObject* conversion)
{
	conversions.push_back(new JPAttributeConversion(attribute, conversion));
}
//</editor-fold>
//<editor-fold desc="type conversion" defaultstate="collapsed">

class JPTypeConversion : public JPPythonConversion
{
public:

	JPTypeConversion(PyObject* type, PyObject* method, bool exact)
	: JPPythonConversion(method), type_(JPPyRef::_use, type), exact_(exact)
	{
	}

	virtual ~JPTypeConversion()
	{
	}

	virtual JPMatch::Type matches(JPContext* context, JPClass* cls, PyObject* obj) override
	{
		if (exact_)
		{
			return Py_TYPE(obj) == type_.get() ? JPMatch::_implicit : JPMatch::_none;
		}
		return JPPyObject::isInstance(obj, type_.get()) ? JPMatch::_implicit : JPMatch::_none;
	}

private:
	JPPyObject type_;
	bool exact_;
} ;

void JPClassHints::addTypeConversion(PyObject* type, PyObject* method, bool exact)
{
	conversions.push_back(new JPTypeConversion(type, method, exact));
}
//</editor-fold>

class JPConversionNull : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		if (!JPPyObject::isNone(pyobj))
			return JPMatch::_none;
		match.conversion = this;
		match.type = JPMatch::_implicit;
		return match.type;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		jvalue v;
		v.l = NULL;
		return v;
	}
} _nullConversion;
JPConversion* nullConversion = &_nullConversion;

class JPConversionProxy : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
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

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		jvalue res;
		JPProxy* proxy = JPPythonEnv::getJavaProxy(pyobj);
		res.l = proxy->getProxy();
		return res;
	}
} _proxyConversion;
JPConversion* proxyConversion = &_proxyConversion;

class JPConversionClass : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		if (JPPythonEnv::getJavaClass(pyobj) == NULL)
			return JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		jvalue res;
		JPClass* cls = JPPythonEnv::getJavaClass(pyobj);
		if (cls != NULL)
		{
			res.l = frame.NewLocalRef(cls->getJavaClass());
			return res;
		}
	}
} _classConversion;
JPConversion* classConversion = &_classConversion;

class JPConversionObject : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		JPValue* value = JPPythonEnv::getJavaValue(pyobj);
		if (value == NULL)
			return JPMatch::_none;
		JPClass* oc = value->getClass();
		if (oc == this)
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

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		jvalue res;
		JPClass* cls = JPPythonEnv::getJavaClass(pyobj);
		if (cls != NULL)
		{
			res.l = frame.NewLocalRef(cls->getJavaClass());
			return res;
		}
	}
} _objectConversion;
JPConversion* objectConversion = &_objectConversion;

class JPConversionString : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		if (!JPPyString::check(pyobj))
			return JPMatch::_none;
		match.conversion = this;
		match.type = JPMatch::_implicit;
		return match.type;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		jvalue res;
		string str = JPPyString::asStringUTF8(pyobj);
		res.l = frame.getContext()->fromStringUTF8(str);
		return res;
	}
} _stringConversion;
JPConversion* stringConversion = &_stringConversion;

class JPConversionJavaValue : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		JPValue* value = JPPythonEnv::getJavaValue(pyobj);
		return *value;
	}
} _javaValueConversion;
JPConversion* javaValueConversion = &_javaValueConversion;

class JPConversionUnbox : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		JPValue* value = JPPythonEnv::getJavaValue(pyobj);
		return cls->getValueFromObject(value->getJavaObject());
	}
} _unboxConversion;
JPConversion* unboxConversion = &_unboxConversion;

