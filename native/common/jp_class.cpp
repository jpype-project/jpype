// --- file: common/jp_class.cpp ---
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
#include "jp_array.h"
#include "jp_arrayclass.h"
#include "jp_field.h"
#include "jp_methoddispatch.h"
#include "jp_method.h"
#include "jp_proxy.h"

JPClass::JPClass(JPJavaFrame& frame,
		jclass clss,
		const string& name,
		jint modifiers)
{
	m_Context = frame.getContext();
	m_Class = frame.storeGlobal(clss);
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
{
	m_Context = frame.getContext();
	m_Class = frame.storeGlobal(clss);
	m_CanonicalName = name;
	m_SuperClass = super;
	m_Interfaces = interfaces;
	m_Modifiers = modifiers;
}

JPClass::~JPClass() 
{
	tryRelease(m_Class);
}

void JPClass::setHost(PyObject* host)
{
	m_Host = JPPyObject::use(host);
}

void JPClass::setHints(PyObject* host)
{
	m_Hints = JPPyObject::use(host);
}

jclass JPClass::getJavaClass(JPJavaFrame& frame) const
{
	jclass cls = (jclass) frame.retrieveGlobal(m_Class);
	// This sanity check should not be possible to exercise
	if (cls == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Class is null"); // GCOVR_EXCL_LINE
	return cls;
}

void JPClass::ensureMembers(JPJavaFrame& frame)
{
	JPContext* context = frame.getContext();
	JPTypeManager* typeManager = context->getTypeManager();
	typeManager->populateMembers(frame, this);
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
		ss << getName(frame);
	else
		ss << "L" << getName(frame) << ";";
	return frame.findClassByName(ss.str());
}

jarray JPClass::newArrayOf(JPJavaFrame& frame, jsize sz)
{
	return frame.NewObjectArray(sz, getJavaClass(frame), nullptr);
}
//</editor-fold>
//<editor-fold desc="acccessors" defaultstate="collapsed">

// GCOVR_EXCL_START
// This is currently only used in tracing
string JPClass::toString(JPJavaFrame& frame) const
{
	return frame.toString(getJavaClass(frame));
}
// GCOVR_EXCL_STOP

string JPClass::getName(JPJavaFrame& frame) const
{
	return frame.toString(frame.CallObjectMethodA(
			getJavaClass(frame), frame.getContext()->m_Class_GetNameID, nullptr));
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
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
	{
		std::stringstream err;
		err << "unable to convert to " << getCanonicalName(frame);
		JP_RAISE(PyExc_TypeError, err.str());
	}
	jobject val = match.convert().l;
	frame.SetStaticObjectField(c, fid, val);
	JP_TRACE_OUT;
}

void JPClass::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JP_TRACE_IN("JPClass::setField");
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
	{
		std::stringstream err;
		err << "unable to convert to " << getCanonicalName(frame);
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

	// Match every item before starting the conversion, as we won't be
	// able to abort once we start writing into the array. The matched
	// items and their JPMatch results are held here (rather than
	// re-matching in a second pass below) so findJavaConversion runs
	// exactly once per item -- re-matching would recompute a decision
	// already made, and for a nested array element that decision can
	// itself be an expensive recursive match.
	JPPySequence seq = JPPySequence::use(vals);
	std::vector<JPPyObject> items;
	std::vector<JPMatch> matches;
	items.reserve(length);
	matches.reserve(length);
	JP_TRACE("Verify argument types");
	for (int i = 0; i < length; i++)
	{
		items.push_back(seq[i]);
		matches.emplace_back(frame, items.back().get());
		if (findJavaConversion(matches.back()) < JPMatch::_implicit)
			JP_RAISE(PyExc_TypeError, "Unable to convert");
	}

	JP_TRACE("Copy");
	int index = start;
	for (int i = 0; i < length; i++, index += step)
		frame.SetObjectArrayElement(array, index, matches[i].convert().l);
	JP_TRACE_OUT;
}

void JPClass::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* val)
{
	JP_TRACE_IN("JPClass::setArrayItem");
	JPMatch match(frame, val);
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

JPArray* JPClass::createArrayWrapper(const JPValue& value)
{
	return new JPArrayObject(value);
}

JPArrayClass* JPClass::createArrayClass(JPJavaFrame& frame, jclass cls,
		const string& name, JPClass* superClass, jint modifiers)
{
	return new JPArrayClass(frame, cls, name, superClass, this, modifiers);
}

//</editor-fold>
//<editor-fold desc="conversion" defaultstate="collapsed">

JPValue JPClass::getValueFromObject(JPJavaFrame& frame, const JPValue& obj)
{
	JP_TRACE_IN("JPClass::getValueFromObject");
	return JPValue(this, obj.getJavaObject(frame));
	JP_TRACE_OUT;
}

JPPyObject JPClass::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{
	JP_TRACE_IN("JPClass::convertToPythonObject");
	JPClass *cls = this;
	JPContext* context = frame.getContext();
	PyJPModuleState* state = context->modulestate;
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

		// findClassForObject is a JNI upcall into Java's TypeManager (a
		// bytecode-level HashMap.get, not just a native call) -- for the
		// very common case where the runtime class is exactly the
		// declared one (no covariant override in play), a cheap
		// GetObjectClass + IsSameObject against the class we already hold
		// a global ref to answers the same question without it.
		if (!frame.IsSameObject(frame.GetObjectClass(value.l), getJavaClass(frame)))
		{
			cls = frame.findClassForObject(value.l);
			if (cls != this)
				return cls->convertToPythonObject(frame, value, true);
		}
	}

	// Special path for proxy that need automatic unwrapping
	if (isProxy())
	{
		jlong hostPtr = frame.CallStaticLongMethodA(context->m_ProxyTypeClass, context->m_ProxyType_GetInstanceID, &value);
		JPProxy *proxy = (JPProxy*) hostPtr;
		// Smuggler guard: this proxy's PyObject* was allocated by the
		// interpreter that created it (proxy->m_Context), not necessarily
		// the interpreter running right now. Handing pproxy->m_Target
		// straight back into a different interpreter's Python code is a
		// cross-interpreter object-safety violation - own-GIL
		// subinterpreters (plan/MultiPhaseInit.md) have separate
		// allocators/arenas, so touching it here would be memory
		// corruption, not just a wrong answer. See plan/Smuggler.md.
		if (proxy->m_Context != context)
		{
			JP_RAISE(PyExc_RuntimeError,
					"Python object crossed into a different interpreter "
					"than the one that created it (smuggled proxy)");
		}
		PyJPProxy *pproxy = proxy->m_Instance;
		if (pproxy->m_Convert && pproxy->m_Target != Py_None)
			return JPPyObject::use(pproxy->m_Target);
		else
			return JPPyObject::use((PyObject*) pproxy);
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
		PyJPModuleState* st = frame.getContext()->modulestate;
		JPPyObject tuple1 = JPPyTuple_Pack(st->JObjectKey, tuple0.get());
		// Exceptions need new and init
		obj = JPPyObject::call(PyObject_Call(wrapper.get(), tuple1.get(), nullptr));
	} else
	{
		PyTypeObject *type = ((PyTypeObject*) wrapper.get());
		// Simple objects don't have a new or init function.  If this type is
		// abstract (kept layout-trivial so it can be mixed into any foreign
		// family -- see PyJPClass_FromSpecWithBases), redirect the actual
		// allocation to its hidden concrete companion, exactly as
		// PyJPObject_new does for the ordinary constructor path. offset is
		// no longer usable to detect abstract-ness (see PyJPObject_new's
		// comment) -- tp_concrete is the real signal.
		PyTypeObject *allocType = PyJPClass_getConcrete(type);
		if (allocType == nullptr)
			allocType = type;
		PyObject *obj2 = allocType->tp_alloc(allocType, 0);
		JP_PY_CHECK_NULL(obj2);

		if (allocType != type)
		{
			// Polymorph back to the canonical/abstract type, so
			// type(instance) stays consistent with cls->getHost() and any
			// other identity-sensitive code -- mirrors PyJPObject_new's own
			// polymorph-back (see pyjp_object.cpp) and the legacy
			// PyJPValue_alloc's Py_SET_TYPE trick.
			Py_INCREF(type);
			Py_SET_TYPE(obj2, type);
			Py_DECREF(allocType);
		}

		obj = JPPyObject::claim(obj2);
	}

	// Fill in the Java slot
	PyJPValue_assignJavaSlot(frame, obj.get(), JPValue(cls, value));
	return obj;
	JP_TRACE_OUT;
}

JPMatch::Type JPClass::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPClass::findJavaConversionImpl");
	// A dynamic proxy can only ever be assigned to an interface -- never
	// a plain class, which this is (an actual interface is constructed
	// as a JPInterfaceType instead; see
	// TypeFactoryNative_defineObjectClass) -- so proxyConversion is
	// deliberately not tried here at all.
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| pythonConversion->matches(this, match)
			|| hintsConversion->matches(this, match))
		return match.type;
	JP_TRACE("No match");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPClass::clearConversionCache()
{
	m_ConversionCache.clear();
}

JPMatch::Type JPClass::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPClass::findJavaConversion");
	if (m_ConversionCacheGeneration != JPClassHints::s_Generation)
	{
		m_ConversionCache.clear();
		m_ConversionCacheGeneration = JPClassHints::s_Generation;
	}

	auto *type = Py_TYPE(match.object);
	JPConversion *cachedConversion;
	JPMatch::Type cachedType;
	if (m_ConversionCache.lookup(type, cachedConversion, cachedType))
	{
		match.conversion = cachedConversion;
		// See the comment on m_ConversionCache: every cacheable conversion
		// uses closure == this, except this one fixed, known exception.
		match.closure = (cachedConversion == boxBooleanConversion)
				? (void*) match.frame->getContext()->_java_lang_Boolean
				: (void*) this;
		return match.type = cachedType;
	}

	match.cacheable = true;
	JPMatch::Type result = findJavaConversionImpl(match);
	if (match.cacheable)
		m_ConversionCache.store(type, match.conversion, result);
	return result;
	JP_TRACE_OUT;
}

namespace
{

// Shared by sequenceCheck/sequenceCheckList/sequenceCheckTuple below:
// given one already-fetched element, either take the bare-compare fast
// path against the running {cachedType, cachedQuality} slot, or fall to
// findJavaConversion and (re)fill that slot when the result is cacheable.
// Factored out so the three container-specific loops share this logic
// textually instead of tripling it -- each loop itself still stays
// branch-free per element, since only the *indexing* operation differs
// between them, not this step.
inline void sequenceCheckStep(JPClass *self, JPMatch &match, PyObject *obj,
		PyTypeObject *&cachedType, JPMatch::Type &cachedQuality)
{
	PyTypeObject *itemType = Py_TYPE(obj);
	if (itemType == cachedType)
	{
		if (cachedQuality < match.type)
			match.type = cachedQuality;
		return;
	}

	JPMatch imatch(*match.frame, obj);
	self->findJavaConversion(imatch);
	if (imatch.cacheable)
	{
		cachedType = itemType;
		cachedQuality = imatch.type;
	}
	if (imatch.type < match.type)
		match.type = imatch.type;
}

} // namespace

void JPClass::sequenceCheck(JPMatch& match, JPPySequence& seq, jlong length)
{
	JP_TRACE_IN("JPClass::sequenceCheck");
	// See the declaration in jp_class.h for the full rationale: a single
	// {PyTypeObject*, quality} slot for the whole scan, filled from the
	// ordinary findJavaConversion() on a miss and trusted for later
	// same-typed elements only when that call reported cacheable -- the
	// same flag findJavaConversion()'s own per-class cache already keys
	// on, set correctly by every JPConversion::matches() already, so this
	// needs no per-type knowledge to stay correct for any JPClass.
	//
	// This is the general path (used when the sequence isn't a plain list
	// or tuple -- see sequenceCheckList/Tuple for those), so element
	// access still goes through seq[i]'s ordinary PySequence_GetItem.
	match.type = JPMatch::_implicit;
	// nullptr doubles as the "nothing cached yet" sentinel -- Py_TYPE(obj)
	// is never null for a real object, so no separate bool is needed to
	// distinguish an empty slot from a real cached type.
	PyTypeObject *cachedType = nullptr;
	JPMatch::Type cachedQuality = JPMatch::_none;
	for (jlong i = 0; i < length && match.type > JPMatch::_none; i++)
	{
		JPPyObject item = seq[i];
		sequenceCheckStep(this, match, item.get(), cachedType, cachedQuality);
	}
	JP_TRACE_OUT;
}

void JPClass::sequenceCheckList(JPMatch& match, PyObject* listObj, jlong length)
{
	JP_TRACE_IN("JPClass::sequenceCheckList");
	// Same algorithm as sequenceCheck (see there for the cacheable
	// rationale), but for a PyList_CheckExact object specifically:
	// PyList_GET_ITEM indexes straight into the list's backing array with
	// a borrowed reference (valid for the object's lifetime, no
	// PySequence_GetItem protocol dispatch, no refcount churn per
	// element).
	match.type = JPMatch::_implicit;
	PyTypeObject *cachedType = nullptr;
	JPMatch::Type cachedQuality = JPMatch::_none;
	for (jlong i = 0; i < length && match.type > JPMatch::_none; i++)
	{
		PyObject *obj = PyList_GET_ITEM(listObj, (Py_ssize_t) i);
		sequenceCheckStep(this, match, obj, cachedType, cachedQuality);
	}
	JP_TRACE_OUT;
}

void JPClass::sequenceCheckTuple(JPMatch& match, PyObject* tupleObj, jlong length)
{
	JP_TRACE_IN("JPClass::sequenceCheckTuple");
	// Same as sequenceCheckList, for a PyTuple_CheckExact object.
	match.type = JPMatch::_implicit;
	PyTypeObject *cachedType = nullptr;
	JPMatch::Type cachedQuality = JPMatch::_none;
	for (jlong i = 0; i < length && match.type > JPMatch::_none; i++)
	{
		PyObject *obj = PyTuple_GET_ITEM(tupleObj, (Py_ssize_t) i);
		sequenceCheckStep(this, match, obj, cachedType, cachedQuality);
	}
	JP_TRACE_OUT;
}

PyObject* JPClass::getHints(JPJavaFrame& frame)
{
	PyObject* out = m_Hints.get();
	if (out != nullptr)
		return out;
	PyJPClass_create(frame, this);
	return m_Hints.get();
}

void JPClass::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	JP_TRACE_IN("JPClass::getConversionInfo");
	objectConversion->getInfo(frame, this, info);
	hintsConversion->getInfo(frame, this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
	JP_TRACE_OUT;
}

//</editor-fold>
//<editor-fold desc="hierarchy" defaultstate="collapsed">

bool JPClass::isAssignableFrom(JPJavaFrame& frame, JPClass* o)
{
	return frame.IsAssignableFrom(getJavaClass(frame), o->getJavaClass(frame)) != 0;
}

//</editor-fold>
