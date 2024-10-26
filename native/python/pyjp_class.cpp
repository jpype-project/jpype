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
#include <algorithm>
#include <Python.h>
#include <dictobject.h>
#include <frameobject.h>
#include <object.h>
#include <structmember.h>
#include <tupleobject.h>
#include "jp_class.h"
#include "jp_extension.hpp"
#include "jpype.h"
#include "pyjp.h"
#include "jp_array.h"
#include "jp_arrayclass.h"
#include "jp_boxedtype.h"
#include "jp_field.h"
#include "jp_methoddispatch.h"
#include "jp_primitive_accessor.h"
#include "pyjp_module.hpp"


using namespace std::literals;

struct PyJPClass
{
	PyHeapTypeObject ht_type;
	JPClass *m_Class;
	PyObject *m_Doc;
};

PyObject* PyJPClassMagic = nullptr;

#ifdef __cplusplus
extern "C" {
#endif

int PyJPClass_Check(PyObject* obj)
{
	return PyJP_IsInstanceSingle(obj, PyJPClass_Type);
}

static int PyJPClass_traverse(PyJPClass *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Doc);
	return 0;
}

static int PyJPClass_clear(PyJPClass *self)
{
	Py_CLEAR(self->m_Doc);
	return 0;
}

PyObject* examine(PyObject *module, PyObject *other);

PyObject* PyJPClass_FromSpecWithBases(PyType_Spec *spec, PyObject *bases)
{
	JP_PY_TRY("PyJPClass_FromSpecWithBases");
#if PY_VERSION_HEX>=0x030c0000
	// Starting in Python 3.12 there is a function for creating from a meta class
	// that replaces this madeness.
	PyTypeObject *type = (PyTypeObject*) PyType_FromMetaclass((PyTypeObject*) PyJPClass_Type, NULL, spec, bases);
	if (type == nullptr)
		return (PyObject*) type;
#else
	// Python lacks a FromSpecWithMeta so we are going to have to fake it here.
	auto* type = (PyTypeObject*) PyJPClass_Type->tp_alloc(PyJPClass_Type, 0);
	auto* heap = (PyHeapTypeObject*) type;
	type->tp_flags = spec->flags | Py_TPFLAGS_HEAPTYPE;
	type->tp_name = spec->name;
	const char *s = strrchr(spec->name, '.');
	if (s == nullptr)
		s = spec->name;
	else
		s++;
	heap->ht_qualname = PyUnicode_FromString(s);
	heap->ht_name = heap->ht_qualname;
	Py_INCREF(heap->ht_name);
	if (bases == nullptr)
	{
		// do NOT use JPPyTuple_Pack here
		type->tp_bases = PyTuple_Pack(1, (PyObject*) & PyBaseObject_Type);
	}
	else
	{
		type->tp_bases = bases;
		Py_INCREF(bases);
	}
	type->tp_base = (PyTypeObject*) PyTuple_GetItem(type->tp_bases, 0);
	Py_INCREF(type->tp_base);
	type->tp_as_async = &heap->as_async;
	type->tp_as_buffer = &heap->as_buffer;
	type->tp_as_mapping = &heap->as_mapping;
	type->tp_as_number = &heap->as_number;
	type->tp_as_sequence = &heap->as_sequence;
	type->tp_basicsize = spec->basicsize;
	if (spec->basicsize == 0)
		type->tp_basicsize = type->tp_base->tp_basicsize;
	type->tp_itemsize = spec->itemsize;
	if (spec->itemsize == 0)
		type->tp_itemsize = type->tp_base->tp_itemsize;
	type->tp_alloc = PyJPValue_alloc;
	type->tp_free = PyJPValue_free;
	type->tp_finalize = (destructor) PyJPValue_finalize;
	for (PyType_Slot* slot = spec->slots; slot->slot; slot++)
	{
		switch (slot->slot)
		{
			case Py_tp_finalize:
				type->tp_finalize = (destructor) slot->pfunc;
				break;
			case Py_tp_alloc:
				type->tp_alloc = (allocfunc) slot->pfunc;
				break;
			case Py_tp_free:
				type->tp_free = (freefunc) slot->pfunc;
				break;
			case Py_tp_new:
				type->tp_new = (newfunc) slot->pfunc;
				break;
			case Py_tp_init:
				type->tp_init = (initproc) slot->pfunc;
				break;
			case Py_tp_getattro:
				type->tp_getattro = (getattrofunc) slot->pfunc;
				break;
			case Py_tp_setattro:
				type->tp_setattro = (setattrofunc) slot->pfunc;
				break;
			case Py_tp_dealloc:
				type->tp_dealloc = (destructor) slot->pfunc;
				break;
			case Py_tp_str:
				type->tp_str = (reprfunc) slot->pfunc;
				break;
			case Py_tp_repr:
				type->tp_repr = (reprfunc) slot->pfunc;
				break;
			case Py_tp_methods:
				type->tp_methods = (PyMethodDef*) slot->pfunc;
				break;
			case Py_sq_item:
				heap->as_sequence.sq_item = (ssizeargfunc) slot->pfunc;
				break;
			case Py_sq_length:
				heap->as_sequence.sq_length = (lenfunc) slot->pfunc;
				break;
			case Py_mp_ass_subscript:
				heap->as_mapping.mp_ass_subscript = (objobjargproc) slot->pfunc;
				break;
			case Py_tp_hash:
				type->tp_hash = (hashfunc) slot->pfunc;
				break;
			case Py_nb_int:
				heap->as_number.nb_int = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_float:
				heap->as_number.nb_float = (unaryfunc) slot->pfunc;
				break;
			case Py_tp_richcompare:
				type->tp_richcompare = (richcmpfunc) slot->pfunc;
				break;
			case Py_mp_subscript:
				heap->as_mapping.mp_subscript = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_index:
				heap->as_number.nb_index = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_absolute:
				heap->as_number.nb_absolute = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_and:
				heap->as_number.nb_and = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_or:
				heap->as_number.nb_or = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_xor:
				heap->as_number.nb_xor = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_add:
				heap->as_number.nb_add = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_subtract:
				heap->as_number.nb_subtract = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_multiply:
				heap->as_number.nb_multiply = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_rshift:
				heap->as_number.nb_rshift = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_lshift:
				heap->as_number.nb_lshift = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_negative:
				heap->as_number.nb_negative = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_bool:
				heap->as_number.nb_bool = (inquiry) slot->pfunc;
				break;
			case Py_nb_invert:
				heap->as_number.nb_invert = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_positive:
				heap->as_number.nb_positive = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_floor_divide:
				heap->as_number.nb_floor_divide = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_divmod:
				heap->as_number.nb_divmod = (binaryfunc) slot->pfunc;
				break;
			case Py_tp_getset:
				type->tp_getset = (PyGetSetDef*) slot->pfunc;
				break;
#if PY_VERSION_HEX >= 0x03090000
			case Py_bf_getbuffer:
				type->tp_as_buffer->bf_getbuffer = (getbufferproc) slot->pfunc;
				break;
			case Py_bf_releasebuffer:
				type->tp_as_buffer->bf_releasebuffer = (releasebufferproc) slot->pfunc;
				break;
#endif
				// GCOVR_EXCL_START
			default:
				PyErr_Format(PyExc_TypeError, "slot %d not implemented", slot->slot);
				JP_RAISE_PYTHON();
				// GCOVR_EXCL_STOP
		}
	}

	// GC objects are required to implement clear and traverse, this is a
	// safety check to make sure we implemented all properly.	This error should
	// never happen in production code.
	if (PyType_IS_GC(type) && (
			type->tp_traverse==nullptr ||
			type->tp_clear==nullptr))
	{
		PyErr_Format(PyExc_TypeError, "GC requirements failed for %s", spec->name);
		JP_RAISE_PYTHON();
	}

#endif

	// Make sure our memory model is used
	type->tp_alloc = (allocfunc) PyJPValue_alloc;
	type->tp_finalize = (destructor) PyJPValue_finalize;

	PyType_Ready(type);
	PyDict_SetItemString(type->tp_dict, "__module__", PyUnicode_FromString("_jpype"));
	return (PyObject*) type;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static void PyJPClass_hook_extends(JPJavaFrame &frame, JPClass &cls, PyObject *members) {
	const JPFieldList &instFields = cls.getFields();
	for (auto instField : instFields) {
		JPPyObject fieldName(JPPyString::fromStringUTF8(instField->getName()));
		PyDict_SetItem(members, fieldName.get(), PyJPField_create(instField).get());
	}

	const JPMethodDispatchList& m_Methods = cls.getMethods();
	for (auto m_Method : m_Methods) {
		// TODO: it would be great to not have to go through java to call our python methods
		JPPyObject methodName(JPPyString::fromStringUTF8(m_Method->getName()));
		PyDict_SetItem(members, methodName.get(), PyJPMethod_create(m_Method, nullptr).get());
	}

	JPValue value{frame.getContext()->_java_lang_Class, (jobject) cls.getJavaClass()};
	PyJPValue_assignJavaSlot(frame, (PyObject*)cls.getHost(), value);
}

static int PyJPClass_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_init");

	if (!PyObject_IsInstance(self, (PyObject*) PyJPClass_Type))
	{
		PyErr_SetString(PyExc_TypeError, "Type incorrect");
		return -1;
	}

	PyTypeObject *type = (PyTypeObject*) self;

#if PY_VERSION_HEX >= 0x030d0000
	// Python 3.13 - This flag will try to place the dictionary are part of the object which
	// adds an unknown number of bytes to the end of the object making it impossible
	// to attach our needed data.  If we kill the flag then we get usable behavior.
	type->tp_flags &= ~Py_TPFLAGS_INLINE_VALUES;
#endif

	// Verify that we were called internally
	int magic = 0;
	if (kwargs == PyJPClassMagic || (kwargs != nullptr && PyDict_GetItemString(kwargs, "internal") != nullptr))
	{
		magic = 1;
		kwargs = nullptr;
	}
	if (magic == 0)
	{
		// returns a tuple of the Class pointer and the overrides
		JPPyObject tmp = JPPyObject::call(PyObject_Call(_JExtension, args, kwargs));
		unsigned long long value = PyLong_AsUnsignedLongLong(PyTuple_GetItem(tmp.get(), 0));
		JPExtensionType *cls = reinterpret_cast<JPExtensionType *>(static_cast<uintptr_t>(value));
		((PyJPClass*) self)->m_Class = cls;

		// set host early to prevent double creation
		cls->setHost(self);

		if (!cls->wasReloaded()) {

			JPJavaFrame frame = JPJavaFrame::outer(cls->getContext());

			// fill out the Java class members
			cls->setOverrides(frame, PyTuple_GetItem(tmp.get(), 1));
			cls->ensureMembers(frame);

			// finish setting members in our Python class that got skipped due to the host being set
			PyJPClass_hook_extends(frame, *cls, ((PyTypeObject *)self)->tp_dict);
		}
	}

	PyObject *name = nullptr;
	PyObject *bases = nullptr;
	PyObject *members = nullptr;
	if (!PyArg_ParseTuple(args, "OOO", &name, &bases, &members))
		return -1;

	//	 Check that all types are Java types
	if (!PyTuple_Check(bases))
	{
		PyErr_SetString(PyExc_TypeError, "Bases must be a tuple");
		return -1;
	}

	JP_BLOCK("PyJPClass_new::verify")
	{
		// Watch for final classes
		Py_ssize_t len = PyTuple_Size(bases);
		for (Py_ssize_t i = 0; i < len; ++i)
		{
			PyObject *item = PyTuple_GetItem(bases, i);
			JPClass *cls = PyJPClass_getJPClass(item);
			if (cls != nullptr && cls->isFinal())
			{
				PyErr_Format(PyExc_TypeError, "Cannot extend final class '%s'",
						((PyTypeObject*) item)->tp_name);
			}
		}
	}

	// We must make sure that all classes have our allocator
	type->tp_alloc = (allocfunc) PyJPValue_alloc;
	type->tp_finalize = (destructor) PyJPValue_finalize;
	((PyJPClass*) self)->m_Doc = nullptr;

	// Call the type init
	int rc = PyType_Type.tp_init(self, args, nullptr);
	if (rc == -1)
		return rc; // GCOVR_EXCL_LINE no clue how to trigger this one

	// GCOVR_EXCL_START
	// Sanity checks.  Not testable
	if (type == nullptr)
		return -1;
	if (type->tp_finalize != nullptr && type->tp_finalize != (destructor) PyJPValue_finalize)
	{
		PyErr_SetString(PyExc_TypeError, "finalizer conflict");
		return -1;
	}

	// This sanity check is trigger if the user attempts to build their own
	// type wrapper with a __del__ method defined.	It is hard to trigger.
	if (type->tp_alloc != (allocfunc) PyJPValue_alloc
			&& type->tp_alloc != PyBaseObject_Type.tp_alloc)
	{
		PyErr_SetString(PyExc_TypeError, "alloc conflict");
		return -1;
	}
	// GCOVR_EXCL_STOP

#if PY_VERSION_HEX < 0x03090000
	// This was required at one point but I don't know what version it applied to.
	if (PyObject_IsSubclass((PyObject*) type, (PyObject*) PyJPException_Type))
	{
		type->tp_new = PyJPException_Type->tp_new;
	}
#endif

	return rc;
	JP_PY_CATCH(-1);
}

static void PyJPClass_dealloc(PyJPClass *self)
{
	JP_PY_TRY("PyJPClass_dealloc");
	PyObject_GC_UnTrack(self);
	PyJPClass_clear(self);
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH_NONE(); // GCOVR_EXCL_LINE
}

PyObject* PyJPClass_mro(PyTypeObject *self)
{
	Py_ssize_t sz = PySequence_Size(self->tp_bases);
	std::list<PyObject*> bases;
	bases.push_back((PyObject*) self);

	// Merge together all bases
	std::list<PyTypeObject*> out;
	for (auto iter = bases.begin();
			iter != bases.end(); ++iter)
	{
		PyObject *l = ((PyTypeObject*) * iter)->tp_bases;
		sz = PySequence_Size(l);
		for (Py_ssize_t i = 0; i < sz; ++i)
		{
			PyObject *obj = PyTuple_GetItem(l, i);
			bool found = (std::find(bases.begin(), bases.end(), obj) != bases.end());
			if (!found)
			{
				bases.push_back(obj);
			}
		}
	}

	while (!bases.empty())
	{
		PyObject* front = bases.front();
		bases.pop_front();
		for (auto iter = bases.begin();
				iter != bases.end(); ++iter)
		{
			if (PySequence_Contains(((PyTypeObject*) * iter)->tp_bases, front))
			{
				bases.push_back(front);
				front = nullptr;
				break;
			}
		}
		if (front != nullptr)
		{
			out.push_back((PyTypeObject*)front);
			auto* next = (PyObject*) ((PyTypeObject*) front)->tp_base;
			if (next)
			{
				bases.remove(next);
				bases.push_front(next);
			}
		}
	}

	PyObject *obj = PyTuple_New((Py_ssize_t)out.size());
	int j = 0;
	for (auto iter = out.begin();
			iter != out.end(); ++iter)
	{
		Py_INCREF(*iter);
		PyTuple_SetItem(obj, j++, (PyObject*)*iter);
	}
	return obj;
}

PyObject *PyJPClass_getattro(PyObject *obj, PyObject *name)
{
	JP_PY_TRY("PyJPClass_getattro");
	if (!PyUnicode_Check(name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(name)->tp_name);
		return nullptr;
	}

	const char *name_tmp = PyUnicode_AsUTF8(name);
	(void) name_tmp;

	// Private members are accessed directly
	PyObject* pyattr = PyType_Type.tp_getattro(obj, name);
	if (pyattr == nullptr)
		return nullptr;
	JPPyObject attr = JPPyObject::claim(pyattr);

	// Private members go regardless
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return attr.keep();

	// Methods
	if (Py_TYPE(attr.get()) == PyJPMethod_Type)
		return attr.keep();

	// Don't allow properties to be rewritten
	if (!PyObject_IsInstance(attr.get(), (PyObject*) & PyProperty_Type))
		return attr.keep();

	const char *name_str = PyUnicode_AsUTF8(name);
	PyErr_Format(PyExc_AttributeError, "Field '%s' is static", name_str);
	return nullptr;
	JP_PY_CATCH(nullptr);
}

int PyJPClass_setattro(PyObject *self, PyObject *attr_name, PyObject *v)
{
	JP_PY_TRY("PyJPClass_setattro");
	PyJPModule_getContext();
	if (!PyUnicode_Check(attr_name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				attr_name->ob_type->tp_name);
		return -1;
	}

	// Private members are accessed directly
	if (PyUnicode_GetLength(attr_name) && PyUnicode_ReadChar(attr_name, 0) == '_')
		return PyType_Type.tp_setattro(self, attr_name, v);

	JPPyObject f = JPPyObject::accept(PyJP_GetAttrDescriptor((PyTypeObject*) self, attr_name));
	if (f.isNull())
	{
		const char *name_str = PyUnicode_AsUTF8(attr_name);
		PyErr_Format(PyExc_AttributeError, "Field '%s' is not found", name_str);
		return -1;
	}

	descrsetfunc desc = Py_TYPE(f.get())->tp_descr_set;
	if (desc != nullptr)
		return desc(f.get(), self, v);

	// Not a descriptor
	const char *name_str = PyUnicode_AsUTF8(attr_name);
	PyErr_Format(PyExc_AttributeError,
			"Static field '%s' is not settable on Java '%s' object",
			name_str, ((PyTypeObject*) self)->tp_name);
	return -1;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClass_subclasscheck(PyTypeObject *type, PyTypeObject *test)
{
	JP_PY_TRY("PyJPClass_subclasscheck");
	if (test == type)
		Py_RETURN_TRUE;

	// GCOVR_EXCL_START
	// This is triggered only if the user asks for isInstance when the
	// JVM is shutdown. It should not happen in normal operations.
	if (!JPContext_global->isRunning())
	{
		if ((PyObject*) type == _JObject)
			return PyBool_FromLong(PyJP_IsSubClassSingle(PyJPObject_Type, test));
		return PyBool_FromLong(PyJP_IsSubClassSingle(type, test));
	}
	// GCOVR_EXCL_STOP

	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);

	// Check for class inheritance first
	JPClass *testClass = PyJPClass_getJPClass((PyObject*) test);
	JPClass *typeClass = PyJPClass_getJPClass((PyObject*) type);
	if (testClass == nullptr)
		Py_RETURN_FALSE;
	if (testClass == typeClass)
		Py_RETURN_TRUE;
	if (typeClass != nullptr)
	{
		if (typeClass->isPrimitive())
			Py_RETURN_FALSE;
		bool b = frame.IsAssignableFrom(testClass->getJavaClass(), typeClass->getJavaClass()) != 0;
		return PyBool_FromLong(b);
	}

	// Otherwise check for special cases
	if ((PyObject*) type == _JInterface)
		return PyBool_FromLong(testClass->isInterface());
	if ((PyObject*) type == _JObject)
		return PyBool_FromLong(!testClass->isPrimitive());
	if ((PyObject*) type == _JArray)
		return PyBool_FromLong(testClass->isArray());
	if ((PyObject*) type == _JException)
		return PyBool_FromLong(testClass->isThrowable());

	PyObject* mro1 = test->tp_mro;
	Py_ssize_t n1 = PyTuple_Size(mro1);
	for (int i = 0; i < n1; ++i)
	{
		if (PyTuple_GetItem(mro1, i) == (PyObject*) type)
			Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPClass_prepare(PyObject *, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) {
	if (nargs < 2 || (kwnames != nullptr && PyDict_GET_SIZE(kwnames) > 0)) {
		return PyDict_New();
	}
	PyObject *bases = args[1];
	if (PyTuple_Size(bases) < 1) {
		return PyDict_New();
	}
	JPClass *cls = PyJPClass_getJPClass(PyTuple_GetItem(bases, 0));
	if (cls == nullptr) {
		return PyDict_New();
	}
	// extension
	return PyObject_CallNoArgs(_JClassTable);
}

static PyObject *PyJPClass_class(PyObject *self, PyObject *)
{
	JP_PY_TRY("PyJPClass_class");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPValue* javaSlot = PyJPValue_getJavaSlot(self);
	if (javaSlot == nullptr)
	{
		PyErr_SetString(PyExc_AttributeError, "Java slot is null");
		return nullptr;
	}
	return javaSlot->getClass()->convertToPythonObject(frame, javaSlot->getValue(), false).keep();
	JP_PY_CATCH(nullptr);
}

static int PyJPClass_setClass(PyObject *self, PyObject *type, PyObject *)
{
	JP_PY_TRY("PyJPClass_setClass", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPValue* javaSlot = PyJPValue_getJavaSlot(type);
	if (javaSlot == nullptr || javaSlot->getClass() != context->_java_lang_Class)
	{
		PyErr_SetString(PyExc_TypeError, "Java class instance is required");
		return -1;
	}
	if (PyJPValue_isSetJavaSlot(self))
	{
		PyErr_SetString(PyExc_AttributeError, "Java class can't be set");
		return -1;
	}
	PyJPValue_assignJavaSlot(frame, self, *javaSlot);

	JPClass* cls = frame.findClass((jclass) javaSlot->getJavaObject());
	JP_TRACE("Set host", cls, javaSlot->getClass()->getCanonicalName().c_str());
	if (cls->getHost() == nullptr)
		cls->setHost(self);
	((PyJPClass*) self)->m_Class = cls;
	return 0;
	JP_PY_CATCH(-1);
}

static PyObject *PyJPClass_hints(PyJPClass *self, PyObject *)
{
	JP_PY_TRY("PyJPClass_hints");
	PyJPModule_getContext();
	JPPyObject hints = JPPyObject::use(self->m_Class->getHints());
	if (hints.get() == nullptr)
		Py_RETURN_NONE; // GCOVR_EXCL_LINE only triggered if JClassPost failed

	if (PyObject_HasAttrString((PyObject*) self, "returns") == 1)
		return hints.keep();

	// Copy in info.
	JPConversionInfo info;
	JPPyObject ret = JPPyObject::call(PyList_New(0));
	JPPyObject implicit = JPPyObject::call(PyList_New(0));
	JPPyObject attribs = JPPyObject::call(PyList_New(0));
	JPPyObject exact = JPPyObject::call(PyList_New(0));
	JPPyObject expl = JPPyObject::call(PyList_New(0));
	JPPyObject none = JPPyObject::call(PyList_New(0));
	info.ret = ret.get();
	info.implicit = implicit.get();
	info.attributes = attribs.get();
	info.exact = exact.get();
	info.expl = expl.get();
	info.none = none.get();
	self->m_Class->getConversionInfo(info);
	PyObject_SetAttrString(hints.get(), "returns", ret.get());
	PyObject_SetAttrString(hints.get(), "implicit", implicit.get());
	PyObject_SetAttrString(hints.get(), "exact", exact.get());
	PyObject_SetAttrString(hints.get(), "explicit", expl.get());
	PyObject_SetAttrString(hints.get(), "none", none.get());
	PyObject_SetAttrString(hints.get(), "attributes", attribs.get());
	return hints.keep();
	JP_PY_CATCH(nullptr);
}

static int PyJPClass_setHints(PyObject *self, PyObject *value, PyObject *)
{
	JP_PY_TRY("PyJPClass_setHints", self);
	PyJPModule_getContext();
	auto *cls = (PyJPClass*) self;
	PyObject *hints = cls->m_Class->getHints();
	if (hints != nullptr)
	{
		PyErr_SetString(PyExc_AttributeError, "_hints can't be set");
		return -1;
	}
	cls->m_Class->setHints(value);
	return 0;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClass_instancecheck(PyTypeObject *self, PyObject *test)
{
	// JInterface is a meta
	if ((PyObject*) self == _JInterface)
	{
		JPContext *context = PyJPModule_getContext();
		JPJavaFrame frame = JPJavaFrame::outer(context);
		JPClass *testClass = PyJPClass_getJPClass((PyObject*) test);
		return PyBool_FromLong(testClass != nullptr && testClass->isInterface());
	}
	if ((PyObject*) self == _JException)
	{
		JPContext *context = PyJPModule_getContext();
		JPJavaFrame frame = JPJavaFrame::outer(context);
		JPClass *testClass = PyJPClass_getJPClass((PyObject*) test);
		if (testClass)
			return PyBool_FromLong(testClass->isThrowable());
	}
	return PyJPClass_subclasscheck(self, Py_TYPE(test));
}

static PyObject *PyJPClass_canCast(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_canCast");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);

	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match(&frame, other);
	cls->findJavaConversion(match);

	// Report to user
	return PyBool_FromLong(match.type == JPMatch::_exact || match.type == JPMatch::_implicit);
	JP_PY_CATCH(nullptr);
}
// Added for auditing

static PyObject *PyJPClass_canConvertToJava(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_canConvertToJava");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);

	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match(&frame, other);
	cls->findJavaConversion(match);

	// Report to user
	if (match.type == JPMatch::_none)
		return JPPyString::fromStringUTF8("none"sv).keep();
	if (match.type == JPMatch::_explicit)
		return JPPyString::fromStringUTF8("explicit"sv).keep();
	if (match.type == JPMatch::_implicit)
		return JPPyString::fromStringUTF8("implicit"sv).keep();
	if (match.type == JPMatch::_derived)
		return JPPyString::fromStringUTF8("derived"sv).keep();
	if (match.type == JPMatch::_exact)
		return JPPyString::fromStringUTF8("exact"sv).keep();

	// Not sure how this could happen
	Py_RETURN_NONE; // GCOVR_EXCL_LINE
	JP_PY_CATCH(nullptr);
}

// Return true if the slice is all indices

static bool PySlice_CheckFull(PyObject *item)
{
	if (!PySlice_Check(item))
		return false;
	Py_ssize_t start, stop, step;
	int rc = PySlice_Unpack(item, &start, &stop, &step);
#if defined(ANDROID)
	return (rc == 0)&&(start == 0)&&(step == 1)&&((int) stop >= 0x7fffffff);
#else
	return (rc == 0)&&(start == 0)&&(step == 1)&&((int) stop == -1);
#endif
}

static PyObject *PyJPClass_array(PyJPClass *self, PyObject *item)
{
	JP_PY_TRY("PyJPClass_array");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);

	if (self->m_Class == NULL)
	{
		// This is only reachable through an internal Jpype type that doesn't have a
		// Java class equivalent such as JArray.
		// __getitem__ takes precedence over __class_getitem__ which forces us through
		// this path.
		// If __class_getitem__ is not implemented in this class, the raised AttributeError
		// will make its way back to Python.
		PyObject *res = PyObject_CallMethod((PyObject *)self, "__class_getitem__", "O", item);
		Py_DECREF(item);
		return res;
	}

	if (PyIndex_Check(item))
	{
		long sz = PyLong_AsLong(item);
		auto *cls = dynamic_cast<JPArrayClass*>( self->m_Class->newArrayType(frame, 1));
		JPValue v = cls->newArray(frame, sz);
		return cls->convertToPythonObject(frame, v.getValue(), true).keep();
	}

	if (PySlice_Check(item))
	{
		if (PySlice_CheckFull(item))
		{
			JPClass *cls = self->m_Class->newArrayType(frame, 1);
			return PyJPClass_create(frame, cls).keep();
		}
		PyErr_Format(PyExc_TypeError, "Bad array specification on slice");
		return nullptr;
	}

	if (PyTuple_Check(item))
	{
		Py_ssize_t dims = PyTuple_Size(item);
		Py_ssize_t i = 0;
		Py_ssize_t defined = 0;
		Py_ssize_t undefined = 0;

		std::vector<int> sz;
		for (; i < dims; ++i)
		{
			PyObject* t = PyTuple_GetItem(item, i);
			if (PyIndex_Check(t) && PyLong_AsLong(t) > 0)
			{
				defined++;
				sz.push_back(PyLong_AsLong(t));
			} else
				break;
		}
		for (; i < dims; ++i)
			if (PySlice_CheckFull(PyTuple_GetItem(item, i)))
				undefined++;
			else
				break;
		if (defined + undefined != dims)
		{
			PyErr_SetString(PyExc_TypeError, "Invalid array definition");
			return nullptr;
		}

		// Get the type
		JPClass *cls;
		if (undefined > 0)
			cls = self->m_Class->newArrayType(frame, (long)undefined);
		else
			cls = self->m_Class;

		// If no dimensions were defined then just return the type
		if (defined == 0)
			return PyJPClass_create(frame, cls).keep();

		// Otherwise create an array
		jintArray u = frame.NewIntArray((jsize)defined);
		JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, u,
				&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
		for (size_t j = 0; j < sz.size(); ++j)
			accessor.get()[j] = sz[j];
		accessor.commit();

		jvalue v;
		v.l = frame.newArrayInstance(cls->getJavaClass(), u);
		return context->_java_lang_Object->convertToPythonObject(frame, v, false).keep();
	}

	PyErr_Format(PyExc_TypeError, "Bad array specification");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPClass_cast(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_cast");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPClass *type = self->m_Class;
	JPValue *val = PyJPValue_getJavaSlot(other);

	// Cast on non-Java
	if (val == nullptr || val->getClass()->isPrimitive())
	{
		JPMatch match(&frame, other);
		type->findJavaConversion(match);
		// Otherwise, see if we can convert it
		if (match.type == JPMatch::_none)
		{
			PyErr_Format(PyExc_TypeError,
					"Unable to cast '%s' to java type '%s'",
					Py_TYPE(other)->tp_name,
					type->getCanonicalName().data()
					);
			return nullptr;
		}
		jvalue v = match.convert();
		return type->convertToPythonObject(frame, v, true).keep();
	}

	// Cast on java object
	//	if (!type->isSubTypeOf(val->getClass()))
	jobject obj = val->getJavaObject();
	if (obj == nullptr)
	{
		jvalue v;
		v.l = nullptr;
		return type->convertToPythonObject(frame, v, true).keep();
	}
	JPClass *otherClass = frame.findClassForObject(obj);
	if (otherClass == nullptr)
	{
		return type->convertToPythonObject(frame, val->getValue(), true).keep();
	}

	if (!otherClass->isAssignableFrom(frame, type))
	{
		PyErr_Format(PyExc_TypeError,
				"Unable to cast '%s' to java type '%s'",
				otherClass->getCanonicalName().data(),
				type->getCanonicalName().data()
				);
		return nullptr;
	}

	// Special case.  If the otherClass is an array and the array is
	// a slice then we need to copy it here.
	if (PyObject_IsInstance(other, (PyObject*) PyJPArray_Type))
	{
		auto *array = (PyJPArray*) other;
		if (array->m_Array->isSlice())
		{
			JPJavaFrame frame = JPJavaFrame::outer(context);
			jvalue v;
			v.l = array->m_Array->clone(frame, other);
			return type->convertToPythonObject(frame, v, true).keep();
		}
	}

	return type->convertToPythonObject(frame, val->getValue(), true).keep();

	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPClass_castEq(PyJPClass *, PyObject *)
{
	PyErr_Format(PyExc_TypeError, "Invalid operation");
	return nullptr;
}

// Added for auditing

static PyObject *PyJPClass_convertToJava(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_convertToJava");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);

	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match(&frame, other);
	cls->findJavaConversion(match);

	// If there is no conversion report a failure
	if (match.type == JPMatch::_none)
	{
		PyErr_SetString(PyExc_TypeError, "Unable to create an instance.");
		return nullptr;
	}

	// Otherwise give back a PyJPValue
	jvalue v = match.convert();
	return cls->convertToPythonObject(frame, v, true).keep();
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPClass_repr(PyJPClass *self)
{
	JP_PY_TRY("PyJPClass_repr");
	string name = ((PyTypeObject*) self)->tp_name;
	return PyUnicode_FromFormat("<java class '%s'>", name.c_str());
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPClass_getDoc(PyJPClass *self, void *)
{
	JP_PY_TRY("PyJPMethod_getDoc");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	if (self->m_Doc)
	{
		Py_INCREF(self->m_Doc);
		return self->m_Doc;
	}

	// Pack the arguments
	{
		JP_TRACE("Pack arguments");
		JPPyObject args = JPPyTuple_Pack(self);
		JP_TRACE("Call Python");
		self->m_Doc = PyObject_Call(_JClassDoc, args.get(), nullptr);
		Py_XINCREF(self->m_Doc);
		return self->m_Doc;
	}
	JP_PY_CATCH(nullptr);
}

int PyJPClass_setDoc(PyJPClass *self, PyObject *obj, void *)
{
	JP_PY_TRY("PyJPClass_setDoc");
	Py_CLEAR(self->m_Doc);
	self->m_Doc = obj;
	Py_XINCREF(self->m_Doc);
	return 0;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClass_customize(PyJPClass *self, PyObject *args, PyObject *)
{
	JP_PY_TRY("PyJPClass_customize");
	PyObject *name = nullptr;
	PyObject *value = nullptr;
	if (!PyArg_ParseTuple(args, "OO", &name, &value))
		return nullptr;
	if (PyType_Type.tp_setattro((PyObject*) self, name, value) == -1)
		return nullptr;
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyMethodDef classMethods[] = {
	{"__instancecheck__", (PyCFunction) PyJPClass_instancecheck, METH_O, ""},
	{"__subclasscheck__", (PyCFunction) PyJPClass_subclasscheck, METH_O, ""},
	{"__prepare__", (PyCFunction) PyJPClass_prepare, METH_FASTCALL | METH_KEYWORDS | METH_CLASS, ""},
	{"mro", (PyCFunction) PyJPClass_mro, METH_NOARGS, ""},
	{"_canConvertToJava", (PyCFunction) PyJPClass_canConvertToJava, METH_O, ""},
	{"_convertToJava", (PyCFunction) PyJPClass_convertToJava, METH_O, ""},
	{"_cast", (PyCFunction) PyJPClass_cast, METH_O, ""},
	{"_canCast", (PyCFunction) PyJPClass_canCast, METH_O, ""},
	{"__getitem__", (PyCFunction) PyJPClass_array, METH_O | METH_COEXIST, ""},
	{"_customize", (PyCFunction) PyJPClass_customize, METH_VARARGS, ""},
	{nullptr},
};

static PyGetSetDef classGetSets[] = {
	{"class_", (getter) PyJPClass_class, (setter) PyJPClass_setClass, ""},
	{"_hints", (getter) PyJPClass_hints, (setter) PyJPClass_setHints, ""},
	{"__doc__", (getter) PyJPClass_getDoc, (setter) PyJPClass_setDoc, nullptr, nullptr},
	{nullptr}
};

static PyType_Slot classSlots[] = {
	{ Py_tp_alloc, (void*) PyJPValue_alloc},
	{ Py_tp_finalize, (void*) PyJPValue_finalize},
	{ Py_tp_init, (void*) PyJPClass_init},
	{ Py_tp_dealloc, (void*) PyJPClass_dealloc},
	{ Py_tp_traverse, (void*) PyJPClass_traverse},
	{ Py_tp_clear, (void*) PyJPClass_clear},
	{ Py_tp_repr, (void*) PyJPClass_repr},
	{ Py_tp_getattro, (void*) PyJPClass_getattro},
	{ Py_tp_setattro, (void*) PyJPClass_setattro},
	{ Py_tp_methods, (void*) classMethods},
	{ Py_tp_getset, (void*) classGetSets},
	{ Py_mp_subscript, (void*) PyJPClass_array},
	{ Py_nb_matrix_multiply, (void*) PyJPClass_cast},
	{ Py_nb_inplace_matrix_multiply, (void*) PyJPClass_castEq},
	{0}
};

PyTypeObject* PyJPClass_Type = nullptr;
static PyType_Spec classSpec = {
	"_jpype._JClass",
	sizeof (PyJPClass),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	classSlots
};

#ifdef __cplusplus
}
#endif

void PyJPClass_initType(PyObject* module)
{
	JPPyObject bases = JPPyTuple_Pack(&PyType_Type);
	PyJPClass_Type = (PyTypeObject*) PyType_FromSpecWithBases(&classSpec, bases.get());
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JClass", (PyObject*) PyJPClass_Type);
	JP_PY_CHECK();
}

JPClass* PyJPClass_getJPClass(PyObject* obj)
{
	try
	{
		if (obj == nullptr)
			return nullptr;
		if (PyJPClass_Check(obj))
			return ((PyJPClass*) obj)->m_Class;
		JPValue* javaSlot = PyJPValue_getJavaSlot(obj);
		if (javaSlot == nullptr)
			return nullptr;
		JPClass *cls = javaSlot->getClass();
		if (cls != cls->getContext()->_java_lang_Class)
			return nullptr;
		JPJavaFrame frame = JPJavaFrame::outer(cls->getContext());
		return frame.findClass((jclass) javaSlot->getJavaObject());
	} catch (...) // GCOVR_EXCL_LINE
	{
		return nullptr; // GCOVR_EXCL_LINE
	}
}

void PyJPClass_clearJPClass(PyObject *obj) {
	try
	{
		if (obj == nullptr)
			return;
		if (PyJPClass_Check(obj)) {
			((PyJPClass*) obj)->m_Class = nullptr;
		}
	} catch (...) // GCOVR_EXCL_LINE
	{
		return; // GCOVR_EXCL_LINE
	}
}

static JPPyObject PyJPClass_getBases(JPJavaFrame &frame, JPClass* cls)
{
	JP_TRACE_IN("PyJPClass_bases");

	cls->ensureMembers(frame);

	// Decide the base for this object
	JPPyObject baseType;
	JPContext *context = PyJPModule_getContext();
	JPClass *super = cls->getSuperClass();
	if (dynamic_cast<JPBoxedType*> (cls) == cls)
	{
		if (cls == context->_java_lang_Boolean)
		{
			baseType = JPPyObject::use((PyObject*) PyJPNumberBool_Type);
		} else if (cls == context->_java_lang_Character)
		{
			baseType = JPPyObject::use((PyObject*) PyJPChar_Type);
		} else if (cls == context->_java_lang_Boolean
				|| cls == context->_java_lang_Byte
				|| cls == context->_java_lang_Short
				|| cls == context->_java_lang_Integer
				|| cls == context->_java_lang_Long
				)
		{
			baseType = JPPyObject::use((PyObject*) PyJPNumberLong_Type);
		} else if (cls == context->_java_lang_Float
				|| cls == context->_java_lang_Double
				)
		{
			baseType = JPPyObject::use((PyObject*) PyJPNumberFloat_Type);
		}
	} else if (JPModifier::isBuffer(cls->getModifiers()))
	{
		baseType = JPPyObject::use((PyObject*) PyJPBuffer_Type);
	} else if (cls == context->_java_lang_Throwable)
	{
		baseType = JPPyObject::use((PyObject*) PyJPException_Type);
	} else if (cls->isArray())
	{
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (acls->getComponentType()->isPrimitive())
			baseType = JPPyObject::use((PyObject*) PyJPArrayPrimitive_Type);
		else
			baseType = JPPyObject::use((PyObject*) PyJPArray_Type);
	} else if (cls->getCanonicalName() == "java.lang.Comparable")
	{
		baseType = JPPyObject::use((PyObject*) PyJPComparable_Type);
	} else if (super == nullptr)
	{
		baseType = JPPyObject::use((PyObject*) PyJPObject_Type);
	}

	const JPClassList& baseItf = cls->getInterfaces();
	Py_ssize_t count = (Py_ssize_t)baseItf.size() + (!baseType.isNull() ? 1 : 0) + (super != nullptr ? 1 : 0);

	// Pack into a tuple
	JPPyObject result = JPPyObject::call(PyList_New(count));
	unsigned int i = 0;
	for (; i < baseItf.size(); i++)
	{
		PyList_SetItem(result.get(), i, PyJPClass_create(frame, baseItf[i]).keep());
	}
	if (super != nullptr)
	{
		PyList_SetItem(result.get(), i++, PyJPClass_create(frame, super).keep());
	}
	if (!baseType.isNull())
	{
		PyList_SetItem(result.get(), i++, baseType.keep());
	}
	return result;
	JP_TRACE_OUT;
}

/**
 * Internal method for wrapping a returned Java class instance.
 *
 * This checks the cache for existing wrappers and then
 * transfers control to JClassFactory.	This is required because all of
 * the post load stuff needs to be in Python.
 *
 * @param cls
 * @return
 */
JPPyObject PyJPClass_create(JPJavaFrame &frame, JPClass* cls)
{
	JP_TRACE_IN("PyJPClass_create", cls);
	// Check the cache for speed

	auto *host = (PyObject*) cls->getHost();
	if (host == nullptr)
	{
		frame.newWrapper(cls);
		host = (PyObject*) cls->getHost();
	}
	return JPPyObject::use(host);
	JP_TRACE_OUT;
}

void PyJPClass_hook(JPJavaFrame &frame, JPClass* cls)
{
	JPContext *context = frame.getContext();
	auto *host = (PyObject*) cls->getHost();
	if (host != nullptr)
		return;


	JPPyObject members = JPPyObject::call(PyDict_New());
	JPPyObject args = JPPyTuple_Pack(
			JPPyString::fromStringUTF8(cls->getCanonicalName()).get(),
			PyJPClass_getBases(frame, cls).get(),
			members.get());

	// Catch creation loop,  the process of creating our parent
	host = (PyObject*) cls->getHost();
	if (host != nullptr)
		return;

	const JPFieldList & instFields = cls->getFields();
	for (auto instField : instFields)
	{
		JPPyObject fieldName(JPPyString::fromStringUTF8(instField->getName()));
		PyDict_SetItem(members.get(), fieldName.get(), PyJPField_create(instField).get());
	}
	const JPMethodDispatchList& m_Methods = cls->getMethods();
	for (auto m_Method : m_Methods)
	{
		JPPyObject methodName(JPPyString::fromStringUTF8(m_Method->getName()));
		PyDict_SetItem(members.get(), methodName.get(),
				PyJPMethod_create(m_Method, nullptr).get());
	}

	if (cls->isInterface())
	{
		const JPMethodDispatchList& m_Methods2 = context->_java_lang_Object->getMethods();
		for (auto m_Method : m_Methods2)
		{
			JPPyObject methodName(JPPyString::fromStringUTF8(m_Method->getName()));
			PyDict_SetItem(members.get(), methodName.get(),
					PyJPMethod_create(m_Method, nullptr).get());
		}
	}

	// Call the customizer to make any required changes to the tables.
	JP_TRACE("call pre");
	JPPyObject rc = JPPyObject::call(PyObject_Call(_JClassPre, args.get(), nullptr));

	JP_TRACE("type new");
	// Create the type using the meta class magic
	JPPyObject vself = JPPyObject::call(PyJPClass_Type->tp_call((PyObject*) PyJPClass_Type, rc.get(), PyJPClassMagic));
	auto *self = (PyJPClass*) vself.get();

	// Attach the javaSlot
	self->m_Class = cls;
	//	self->m_Class->postLoad();
	PyJPValue_assignJavaSlot(frame, (PyObject*) self, JPValue(context->_java_lang_Class,
			(jobject) self->m_Class->getJavaClass()));

	// Attach the cache  (adds reference, thus wrapper lives to end of JVM)
	JP_TRACE("set host");
	cls->setHost((PyObject*) self);

	// Call the post load routine to attach inner classes
	JP_TRACE("call post");
	args = JPPyTuple_Pack(self);
	JPPyObject::call(PyObject_Call(_JClassPost, args.get(), nullptr));
}
