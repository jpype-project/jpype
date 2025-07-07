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
#include "jpype.h"
#include "pyjp.h"
#include "jp_field.h"
#include "jp_methoddispatch.h"
#include "jp_method.h"

JPClass::JPClass(
		const string& name,
		jint modifiers)
{
	m_CanonicalName = name;
	m_SuperClass = nullptr;
	m_Interfaces = JPClassList();
	m_Modifiers = modifiers;
}

JPClass::JPClass(JPJavaFrame& frame,
		jclass clss,
		const string& name,
		JPClass* super,
		const JPClassList& interfaces,
		jint modifiers)
: m_Class(frame, clss)
{
	m_CanonicalName = name;
	m_SuperClass = super;
	m_Interfaces = interfaces;
	m_Modifiers = modifiers;
}

JPClass::~JPClass()= default;

void JPClass::setHost(PyObject* host)
{
	m_Host = JPPyObject::use(host);
}

void JPClass::setHints(PyObject* host)
{
	m_Hints = JPPyObject::use(host);
}

jclass JPClass::getJavaClass() const
{
	jclass cls = m_Class.get();
	// This sanity check should not be possible to exercise
	if (cls == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Class is null"); // GCOVR_EXCL_LINE
	return cls;
}

void JPClass::ensureMembers(JPJavaFrame& frame)
{
	JPContext* context = JPContext_global;
	JPTypeManager* typeManager = context->getTypeManager();
	typeManager->populateMembers(this);
}

void JPClass::assignMembers(JPMethodDispatch* ctor,
		JPMethodDispatchList& methods,
		JPFieldList& fields)
{
	m_Constructors = ctor;
	m_Methods = methods;
	m_Fields = fields;
}

//<editor-fold desc="new" defaultstate="collapsed">

JPValue JPClass::newInstance(JPJavaFrame& frame, JPPyObjectVector& args)
{
	if (m_Constructors == nullptr)
	{
		if (this->isInterface())
		{
			JP_RAISE(PyExc_TypeError, "Cannot create Java interface instances");
		} else
		{
			JP_RAISE(PyExc_TypeError, "Java class has no constructors");
		}
	}
	return m_Constructors->invokeConstructor(frame, args);
}

JPClass* JPClass::newArrayType(JPJavaFrame &frame, long d)
{
	if (d < 0 || d > 255)
		JP_RAISE(PyExc_ValueError, "Invalid array dimensions");
	std::stringstream ss;
	for (long i = 0; i < d; ++i)
		ss << "[";
	if (isPrimitive())
		ss << (dynamic_cast<JPPrimitiveType*>( this))->getTypeCode();
	else if (isArray())
		ss << getName();
	else
		ss << "L" << getName() << ";";
	return frame.findClassByName(ss.str());
}

jarray JPClass::newArrayOf(JPJavaFrame& frame, jsize sz)
{
	return frame.NewObjectArray(sz, getJavaClass(), nullptr);
}
//</editor-fold>
//<editor-fold desc="acccessors" defaultstate="collapsed">

// GCOVR_EXCL_START
// This is currently only used in tracing

string JPClass::toString() const
{
	// This sanity check will not be hit in normal operation
	if (JPContext_global == nullptr)
		return m_CanonicalName;  // GCOVR_EXCL_LINE
	JPJavaFrame frame = JPJavaFrame::outer();
	return frame.toString(m_Class.get());
}
// GCOVR_EXCL_STOP

string JPClass::getName() const
{
	// This sanity check will not be hit in normal operation
	if (JPContext_global == nullptr)
		return m_CanonicalName;  // GCOVR_EXCL_LINE
	JPJavaFrame frame = JPJavaFrame::outer();
	return frame.toString(frame.CallObjectMethodA(
			(jobject) m_Class.get(), JPContext_global->m_Class_GetNameID, nullptr));
}

//</editor-fold>
//<editor-fold desc="as return type" defaultstate="collapsed">

JPPyObject JPClass::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	JP_TRACE_IN("JPClass::getStaticField");
	jobject r = frame.GetStaticObjectField(c, fid);
	JPClass* type = this;
	if (r != nullptr)
		type = frame.findClassForObject(r);
	jvalue v;
	v.l = r;
	return type->convertToPythonObject(frame, v, false);
	JP_TRACE_OUT;
}

JPPyObject JPClass::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	JP_TRACE_IN("JPClass::getField");
	jobject r = frame.GetObjectField(c, fid);
	JPClass* type = this;
	if (r != nullptr)
		type = frame.findClassForObject(r);
	jvalue v;
	v.l = r;
	return type->convertToPythonObject(frame, v, false);
	JP_TRACE_OUT;
}

JPPyObject JPClass::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	JP_TRACE_IN("JPClass::invokeStatic");
	jvalue v;
	{
		JPPyCallRelease call;
		v.l = frame.CallStaticObjectMethodA(claz, mth, val);
	}

	JPClass *type = this;
	if (v.l != nullptr)
		type = frame.findClassForObject(v.l);

	return type->convertToPythonObject(frame, v, false);

	JP_TRACE_OUT;
}

JPPyObject JPClass::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	JP_TRACE_IN("JPClass::invoke");
	jvalue v;

	// Call method
	{
		JPPyCallRelease call;
		if (obj == nullptr)
			JP_RAISE(PyExc_ValueError, "method called on null object");
		if (clazz == nullptr)
			v.l = frame.CallObjectMethodA(obj, mth, val);
		else
			v.l = frame.CallNonvirtualObjectMethodA(obj, clazz, mth, val);
	}

	// Get the return type
	JPClass *type = this;
	if (v.l != nullptr)
		type = frame.findClassForObject(v.l);

	return type->convertToPythonObject(frame, v, false);

	JP_TRACE_OUT;
}

void JPClass::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JP_TRACE_IN("JPClass::setStaticField");
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
	{
		std::stringstream err;
		err << "unable to convert to " << getCanonicalName();
		JP_RAISE(PyExc_TypeError, err.str());
	}
	jobject val = match.convert().l;
	frame.SetStaticObjectField(c, fid, val);
	JP_TRACE_OUT;
}

void JPClass::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JP_TRACE_IN("JPClass::setField");
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
	{
		std::stringstream err;
		err << "unable to convert to " << getCanonicalName();
		JP_RAISE(PyExc_TypeError, err.str());
	}
	jobject val = match.convert().l;
	frame.SetObjectField(c, fid, val);
	JP_TRACE_OUT;
}

void JPClass::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* vals)
{
	JP_TRACE_IN("JPClass::setArrayRange");
	auto array = (jobjectArray) a;

	// Verify before we start the conversion, as we wont be able
	// to abort once we start
	JPPySequence seq = JPPySequence::use(vals);
	JP_TRACE("Verify argument types");
	for (int i = 0; i < length; i++)
	{
		JPPyObject v = seq[i];
		JPMatch match(&frame, v.get());
		if (findJavaConversion(match) < JPMatch::_implicit)
			JP_RAISE(PyExc_TypeError, "Unable to convert");
	}

	JP_TRACE("Copy");
	int index = start;
	for (int i = 0; i < length; i++, index += step)
	{
		JPPyObject v = seq[i];
		JPMatch match(&frame, v.get());
		findJavaConversion(match);
		frame.SetObjectArrayElement(array, index, match.convert().l);
	}
	JP_TRACE_OUT;
}

void JPClass::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* val)
{
	JP_TRACE_IN("JPClass::setArrayItem");
	JPMatch match(&frame, val);
	findJavaConversion(match);
	JP_TRACE("Type", getCanonicalName());
	if ( match.type < JPMatch::_implicit)
	{
		JP_RAISE(PyExc_TypeError, "Unable to convert");
	}
	jvalue v = match.convert();
	frame.SetObjectArrayElement((jobjectArray) a, ndx, v.l);
	JP_TRACE_OUT;
}

JPPyObject JPClass::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	JP_TRACE_IN("JPClass::getArrayItem");
	auto array = (jobjectArray) a;

	jobject obj = frame.GetObjectArrayElement(array, ndx);
	JPClass *retType = this;
	jvalue v;
	v.l = obj;
	if (obj != nullptr)
		retType = frame.findClassForObject(v.l);
	return retType->convertToPythonObject(frame, v, false);
	JP_TRACE_OUT;
}

//</editor-fold>
//<editor-fold desc="conversion" defaultstate="collapsed">

JPValue JPClass::getValueFromObject(JPJavaFrame& frame, const JPValue& obj)
{
	JP_TRACE_IN("JPClass::getValueFromObject");
	return JPValue(this, obj.getJavaObject());
	JP_TRACE_OUT;
}

JPPyObject JPClass::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{
	JP_TRACE_IN("JPClass::convertToPythonObject");
	JPClass *cls = this;
	if (!cast)
	{
		//  Returning None likely incorrect from java prospective.
		//  Java still knows the type of null objects thus
		//  converting to None would pose a problem as we lose type.
		//  We would need subclass None for this to make sense so we
		//  can carry both the type and the null, but Python considers
		//  None a singleton so this is not an option.
		//
		//  Of course if we don't mind that "Object is None" would
		//  fail, but "Object == None" would be true, the we
		//  could support null objects properly.  However, this would
		//  need to work as "None == Object" which may be hard to
		//  achieve.
		//
		// We will still need to have the concept of null objects
		// but we can get those through JObject(None, cls).
		if (value.l == nullptr)
		{
			return JPPyObject::getNone();
		}

		cls = frame.findClassForObject(value.l);
		if (cls != this)
			return cls->convertToPythonObject(frame, value, true);
	}

	JPPyObject obj;
	JPPyObject wrapper = PyJPClass_create(frame, cls);

	if (isThrowable())
	{
		JPPyObject tuple0;
		if (value.l == nullptr)
		{
			tuple0 = JPPyObject::call(PyTuple_New(0));
		} else
		{
			jstring m = frame.getMessage((jthrowable) value.l);
			if (m != nullptr)
			{
				tuple0 = JPPyTuple_Pack(
						JPPyString::fromStringUTF8(frame.toStringUTF8(m)).get());
			} else
			{
				tuple0 = JPPyTuple_Pack(
						JPPyString::fromStringUTF8(frame.toString(value.l)).get());
			}
		}
		JPPyObject tuple1 = JPPyTuple_Pack(_JObjectKey, tuple0.get());
		// Exceptions need new and init
		obj = JPPyObject::call(PyObject_Call(wrapper.get(), tuple1.get(), nullptr));
	} else
	{
		PyTypeObject *type = ((PyTypeObject*) wrapper.get());
		// Simple objects don't have a new or init function
		PyObject *obj2 = type->tp_alloc(type, 0);
		JP_PY_CHECK();
		obj = JPPyObject::claim(obj2);
	}

	// Fill in the Java slot
	PyJPValue_assignJavaSlot(frame, obj.get(), JPValue(cls, value));
	return obj;
	JP_TRACE_OUT;
}

JPMatch::Type JPClass::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPClass::findJavaConversion");
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| proxyConversion->matches(this, match)
			|| hintsConversion->matches(this, match))
		return match.type;
	JP_TRACE("No match");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

PyObject* JPClass::getHints()
{
	PyObject* out = m_Hints.get();
	if (out != nullptr)
		return out;
	// Force creation
	JPJavaFrame frame = JPJavaFrame::outer();
	PyJPClass_create(frame, this);
	return m_Hints.get();
}

void JPClass::getConversionInfo(JPConversionInfo &info)
{
	JP_TRACE_IN("JPClass::getConversionInfo");
	JPJavaFrame frame = JPJavaFrame::outer();
	objectConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
	JP_TRACE_OUT;
}

//</editor-fold>
//<editor-fold desc="hierarchy" defaultstate="collapsed">

bool JPClass::isAssignableFrom(JPJavaFrame& frame, JPClass* o)
{
	return frame.IsAssignableFrom(m_Class.get(), o->getJavaClass()) != 0;
}

//</editor-fold>
