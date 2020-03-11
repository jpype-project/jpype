/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

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
#include "jpype.h"
#include "jp_field.h"
#include "jp_methoddispatch.h"
#include "jp_method.h"
#include "pyjp.h"

JPClass::JPClass(
		const string& name,
		jint modifiers)
{
	m_Context = NULL;
	m_CanonicalName = name;
	m_SuperClass = NULL;
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
	m_Context = frame.getContext();
	m_CanonicalName = name;
	m_SuperClass = super;
	m_Interfaces = interfaces;
	m_Modifiers = modifiers;
}

JPClass::~JPClass()
{
}

void JPClass::ensureMembers(JPJavaFrame& frame)
{
	JPContext* context = frame.getContext();
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
	if (m_Constructors == NULL)
		JP_RAISE(PyExc_TypeError, "Cannot create Interface instances");
	return m_Constructors->invokeConstructor(frame, args);
}

jarray JPClass::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewObjectArray(sz, getJavaClass(), NULL);
}
//</editor-fold>
//<editor-fold desc="acccessors" defaultstate="collapsed">

string JPClass::toString() const
{
	if (m_Context == 0)
		return m_CanonicalName;
	JPJavaFrame frame(m_Context);
	return frame.toString(m_Class.get());
}

string JPClass::getName() const
{
	if (m_Context == 0)
		return m_CanonicalName;
	JPJavaFrame frame(m_Context);
	return frame.toString(frame.CallObjectMethodA(
			(jobject) m_Class.get(), m_Context->m_Class_GetNameID, NULL));
}

//</editor-fold>
//<editor-fold desc="as return type" defaultstate="collapsed">

JPPyObject JPClass::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	JP_TRACE_IN("JPClass::getStaticField");
	jobject r = frame.GetStaticObjectField(c, fid);
	JPClass* type = this;
	if (r != NULL)
		type = frame.findClassForObject(r);
	jvalue v;
	v.l = r;
	return type->convertToPythonObject(frame, v);
	JP_TRACE_OUT;
}

JPPyObject JPClass::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	JP_TRACE_IN("JPClass::getField");
	jobject r = frame.GetObjectField(c, fid);
	JPClass* type = this;
	if (r != NULL)
		type = frame.findClassForObject(r);
	jvalue v;
	v.l = r;
	return type->convertToPythonObject(frame, v);
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
	if (v.l != NULL)
		type = frame.findClassForObject(v.l);

	return type->convertToPythonObject(frame, v);

	JP_TRACE_OUT;
}

JPPyObject JPClass::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	JP_TRACE_IN("JPClass::invoke");
	jvalue v;

	// Call method
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			v.l = frame.CallObjectMethodA(obj, mth, val);
		else
			v.l = frame.CallNonvirtualObjectMethodA(obj, clazz, mth, val);
	}

	// Get the return type
	JPClass *type = this;
	if (v.l != NULL)
		type = frame.findClassForObject(v.l);

	return type->convertToPythonObject(frame, v);

	JP_TRACE_OUT;
}

void JPClass::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JP_TRACE_IN("JPClass::setStaticField");
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
	{
		stringstream err;
		err << "unable to convert to " << getCanonicalName();
		JP_RAISE(PyExc_TypeError, err.str().c_str());
	}
	jobject val = match.conversion->convert(&frame, this, obj).l;
	frame.SetStaticObjectField(c, fid, val);
	JP_TRACE_OUT;
}

void JPClass::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JP_TRACE_IN("JPClass::setField");
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
	{
		stringstream err;
		err << "unable to convert to " << getCanonicalName();
		JP_RAISE(PyExc_TypeError, err.str().c_str());
	}
	jobject val = match.conversion->convert(&frame, this, obj).l;
	frame.SetObjectField(c, fid, val);
	JP_TRACE_OUT;
}

void JPClass::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* vals)
{
	JP_TRACE_IN("JPClass::setArrayRange");
	jobjectArray array = (jobjectArray) a;

	// Verify before we start the conversion, as we wont be able
	// to abort once we start
	JPPySequence seq(JPPyRef::_use, vals);
	JP_TRACE("Verify argument types");
	JPMatch match;
	for (int i = 0; i < length; i++)
	{
		JPPyObject v = seq[i];
		if (getJavaConversion(&frame, match, v.get()) < JPMatch::_implicit)
			JP_RAISE(PyExc_TypeError, "Unable to convert");
	}

	JP_TRACE("Copy");
	int index = start;
	for (int i = 0; i < length; i++, index += step)
	{
		JPPyObject v = seq[i];
		getJavaConversion(&frame, match, v.get());
		frame.SetObjectArrayElement(array, index,
				match.conversion->convert(&frame, this, v.get()).l);
	}
	JP_TRACE_OUT;
}

void JPClass::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* val)
{
	JP_TRACE_IN("JPClass::setArrayItem");
	JPMatch match;
	getJavaConversion(&frame, match, val);
	JP_TRACE("Type", getCanonicalName());
	if ( match.type < JPMatch::_implicit)
	{
		JP_RAISE(PyExc_TypeError, "Unable to convert");
	}
	jvalue v = match.conversion->convert(&frame, this, val);
	frame.SetObjectArrayElement((jobjectArray) a, ndx, v.l);
	JP_TRACE_OUT;
}

JPPyObject JPClass::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	JP_TRACE_IN("JPClass::getArrayItem");
	jobjectArray array = (jobjectArray) a;

	jobject obj = frame.GetObjectArrayElement(array, ndx);
	JPClass *retType = this;
	jvalue v;
	v.l = obj;
	if (obj != NULL)
		retType = frame.findClassForObject(v.l);
	return retType->convertToPythonObject(frame, v);
	JP_TRACE_OUT;
}

//</editor-fold>
//<editor-fold desc="conversion" defaultstate="collapsed">

JPValue JPClass::getValueFromObject(const JPValue& obj)
{
	JP_TRACE_IN("JPClass::getValueFromObject");
	return JPValue(this, obj.getJavaObject());
	JP_TRACE_OUT;
}

JPPyObject JPClass::convertToPythonObject(JPJavaFrame& frame, jvalue obj)
{
	JP_TRACE_IN("JPClass::convertToPythonObject");

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
	if (obj.l == NULL)
	{
		return JPPyObject::getNone();
	}

	JPClass *cls = frame.findClassForObject(obj.l);
	return PyJPValue_create(frame, JPValue(cls, obj));
	JP_TRACE_OUT;
}

JPMatch::Type JPClass::getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj)
{
	JP_TRACE_IN("JPClass::getJavaConversion");
	JP_TRACE("Python", JPPyObject::getTypeName(pyobj));
	if (nullConversion->matches(match, frame, this, pyobj) != JPMatch::_none
			|| objectConversion->matches(match, frame, this, pyobj) != JPMatch::_none
			|| proxyConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;

	// Apply user supplied conversions
	if (!m_Hints.isNull())
	{
		JPClassHints *hints = ((PyJPClassHints*) m_Hints.get())->m_Hints;
		if (hints->getConversion(match, frame, this, pyobj) != JPMatch::_none)
		{
			JP_TRACE("Match custom conversion");
			return match.type;
		}
	}
	JP_TRACE("No match");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

//</editor-fold>
//<editor-fold desc="hierarchy" defaultstate="collapsed">

bool JPClass::isAssignableFrom(JPJavaFrame& frame, JPClass* o)
{
	return frame.IsAssignableFrom(m_Class.get(), o->getJavaClass()) != 0;
}

//</editor-fold>
