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
#include <jpype.h>

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

void JPClass::assignMembers(JPMethodDispatch* ctor,
		JPMethodDispatchList& methods,
		JPFieldList& fields)
{
	m_Constructors = ctor;
	m_Methods = methods;
	m_Fields = fields;
}

//<editor-fold desc="new" defaultstate="collapsed">

JPValue JPClass::newInstance(JPPyObjectVector& args)
{
	ASSERT_NOT_NULL(m_Constructors);
	return m_Constructors->invokeConstructor(args);
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
	JP_TRACE_IN("JPClass::getStaticValue");
	jobject r = frame.GetStaticObjectField(c, fid);
	JPClass* type = this;
	if (r != NULL)
		type = frame.getContext()->getTypeManager()->findClassForObject(r);
	jvalue v;
	v.l = r;
	return type->convertToPythonObject(frame, v);
	JP_TRACE_OUT;
}

JPPyObject JPClass::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	JP_TRACE_IN("JPClass::getInstanceValue");
	jobject r = frame.GetObjectField(c, fid);
	JPClass* type = this;
	if (r != NULL)
		type = frame.getContext()->getTypeManager()->findClassForObject(r);
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
		type = frame.getContext()->getTypeManager()->findClassForObject(v.l);

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
		type = frame.getContext()->getTypeManager()->findClassForObject(v.l);

	return type->convertToPythonObject(frame, v);

	JP_TRACE_OUT;
}

void JPClass::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JP_TRACE_IN("JPClass::setStaticValue");
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
	{
		stringstream err;
		err << "unable to convert to " << getCanonicalName();
		JP_RAISE_TYPE_ERROR(err.str().c_str());
	}
	jobject val = match.conversion->convert(&frame, this, obj).l;
	frame.SetStaticObjectField(c, fid, val);
	JP_TRACE_OUT;
}

void JPClass::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JP_TRACE_IN("JPClass::setInstanceValue");
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
	{
		stringstream err;
		err << "unable to convert to " << getCanonicalName();
		JP_RAISE_TYPE_ERROR(err.str().c_str());
	}
	jobject val = match.conversion->convert(&frame, this, obj).l;
	frame.SetObjectField(c, fid, val);
	JP_TRACE_OUT;
}

JPPyObject JPClass::getArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length)
{
	JP_TRACE_IN("JPClass::getArrayRange");
	jobjectArray array = (jobjectArray) a;

	JPPyTuple res(JPPyTuple::newTuple(length));

	jvalue v;
	for (int i = 0; i < length; i++)
	{
		JPClass *type = this;
		v.l = frame.GetObjectArrayElement(array, i + start);
		if (v.l != NULL)
			type = frame.getContext()->getTypeManager()->findClassForObject(v.l);
		res.setItem(i, type->convertToPythonObject(frame, v).get());
	}

	return res;
	JP_TRACE_OUT;
}

void JPClass::setArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length, PyObject* vals)
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
		PyObject* v = seq[i].get();
		if (getJavaConversion(&frame, match, v) < JPMatch::_implicit)
		{
			JP_RAISE_TYPE_ERROR("Unable to convert");
		}
	}

	JP_TRACE("Copy");
	for (int i = 0; i < length; i++)
	{
		PyObject* v = seq[i].get();
		getJavaConversion(&frame, match, v);
		frame.SetObjectArrayElement(array, i + start, match.conversion->convert(&frame, this, v).l);
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
		JP_RAISE_TYPE_ERROR("Unable to convert");
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
		retType = frame.getContext()->getTypeManager()->findClassForObject(v.l);
	return retType->convertToPythonObject(frame, v);
	JP_TRACE_OUT;
}

//</editor-fold>
//<editor-fold desc="conversion" defaultstate="collapsed">

JPValue JPClass::getValueFromObject(const JPValue& obj)
{
	JP_TRACE_IN("JPClass::getValueFromObject");
	jvalue res;
	res.l = obj.getJavaObject();
	return JPValue(this, res);
	JP_TRACE_OUT;
}

JPPyObject JPClass::convertToPythonObject(JPJavaFrame& frame, jvalue obj)
{
	JP_TRACE_IN("JPClass::convertToPythonObject");

	// FIXME returning None likely incorrect from java prospective.
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

	JPClass *cls = frame.getContext()->getTypeManager()->findClassForObject(obj.l);
	return JPPythonEnv::newJavaObject(JPValue(cls, obj));
	JP_TRACE_OUT;
}

JPMatch::Type JPClass::getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj)
{
	JP_TRACE_IN("JPClass::getJavaConversion");
	JP_TRACE("Python", JPPyObject::getTypeName(pyobj));
	if (nullConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
	{
		JP_TRACE("Match null conversion");
		return match.type;
	}
	if (objectConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
	{
		JP_TRACE("Match object conversion");
		return match.type;
	}
	if (proxyConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
	{
		JP_TRACE("Match proxy conversion");
		return match.type;
	}

	// APPLY USER SUPPLIED CONVERSIONS
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
//<editor-fold desc="utility">

bool JPClass::isInstance(JPJavaFrame& frame, JPValue& val)
{
	JPClass* cls = val.getClass();
	if (cls->isPrimitive())
		return false;
	return frame.IsInstanceOf(val.getValue().l, m_Class.get());
}

//</editor-fold>