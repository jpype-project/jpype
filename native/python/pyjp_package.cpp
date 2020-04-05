#include "jpype.h"
#include "pyjp.h"
#include "jp_stringtype.h"
#include <structmember.h>

class JPPackage
{
public:
	string m_Name;
	JPObjectRef m_Object;

	JPPackage(const char *v)
	: m_Name(v)
	{
	}
} ;

#ifdef __cplusplus
extern "C"
{
#endif

struct PyJPPackage
{
	PyObject_HEAD
	PyObject *m_Dict;
	JPPackage *m_Package;
} ;

PyTypeObject *PyJPPackage_Type = NULL;

static PyJPPackage *PyJPPackage_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPPackage_new");
	PyJPPackage *self = (PyJPPackage*) type->tp_alloc(type, 0);
	JP_PY_CHECK();

	char *v;
	if (!PyArg_ParseTuple(args, "s", &v))
		return NULL;

	self->m_Dict = PyDict_New();
	self->m_Package = new JPPackage(v);
	return self;
	JP_PY_CATCH(NULL);
}

static int PyJPPackage_traverse(PyJPPackage *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Dict);
	return 0;
}

static int PyJPPackage_clear(PyJPPackage *self)
{
	Py_CLEAR(self->m_Dict);
	return 0;
}

static void PyJPPackage_dealloc(PyJPPackage *self)
{
	JP_PY_TRY("PyJPPackage_dealloc");
	delete self->m_Package;
	PyObject_GC_UnTrack(self);
	PyJPPackage_clear(self);
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH_NONE();
}

static jobject getPackage(JPJavaFrame &frame, PyJPPackage *self)
{
	// If we already have a loaded object, use it.
	if (self->m_Package->m_Object.get() != NULL)
		return self->m_Package->m_Object.get();

	// Attempt to load the object.
	self->m_Package->m_Object = JPObjectRef(frame.getContext(),
			frame.getPackage(self->m_Package->m_Name));

	// Found it, use it.
	if (self->m_Package->m_Object.get() != NULL)
		return self->m_Package->m_Object.get();

	// Otherwise, this is a bad package.
	PyErr_Format(PyExc_AttributeError, "Java package '%s' is not valid",
			self->m_Package->m_Name.c_str());
	return NULL;
}

static PyObject *PyJPPackage_getattro(PyJPPackage *self, PyObject *attr)
{
	JP_PY_TRY("PyJPPackage_getattro");
	if (!PyUnicode_Check(attr))
	{
		PyErr_Format(PyExc_TypeError, "str required for getattr");
		return NULL;
	}

	{
		// Check the cache
		PyObject *out = PyDict_GetItem(self->m_Dict, attr);
		if (out != NULL)
		{
			Py_INCREF(out);
			return out;
		}
	}

	string attrName = JPPyString::asStringUTF8(attr).c_str();
	// Check for private attribute
	if (attrName.compare(0, 2, "__") == 0)
		return Py_TYPE(self)->tp_base->tp_getattro((PyObject*) self, attr);

	JPContext* context = JPContext_global;
	if (context->isRunning())
	{
		JPJavaFrame frame(context);
		jobject pkg = getPackage(frame, self);
		if (pkg == NULL)
			return NULL;

		JPPyObject out;
		jobject obj = frame.getPackageObject(pkg, attrName);
		if (obj == NULL)
		{
			PyErr_Format(PyExc_AttributeError, "Java package '%s' has no attribute '%U'",
					self->m_Package->m_Name.c_str(), attr);
			return NULL;
		} else if (frame.IsInstanceOf(obj, context->_java_lang_Class->getJavaClass()))
			out = PyJPClass_create(frame, frame.findClass((jclass) obj));
		else if (frame.IsInstanceOf(obj, context->_java_lang_String->getJavaClass()))
		{
			JPPyTuple args = JPPyTuple::newTuple(1);
			JPPyObject u(JPPyRef::_call, PyUnicode_FromFormat("%s.%U",
					self->m_Package->m_Name.c_str(), attr));
			args.setItem(0, u.get());
			out = JPPyObject(JPPyRef::_call,
					PyObject_Call((PyObject*) PyJPPackage_Type, args.get(), NULL));
		} else
		{
			// We should be able to handle Python classes, datafiles, etc,
			// but that will take time to implement.  In principle, things
			// that are not packages or classes should appear as Buffers or
			// some other resource type.
			PyErr_Format(PyExc_AttributeError, "Unknown type object in package");
			return NULL;
		}
		// Cache the item for now
		PyDict_SetItem(self->m_Dict, attr, out.get()); // This does not steal
		return out.keep();
	} else
	{
		// Prior to starting the JVM we always return a package to be
		// consistent with old behavior.  This is somewhat unsafe as
		// we cannot check if it is a valid package.
		JPPyTuple args = JPPyTuple::newTuple(1);
		JPPyObject u(JPPyRef::_call, PyUnicode_FromFormat("%s.%U",
				self->m_Package->m_Name.c_str(), attr));
		args.setItem(0, u.get());

		// Note that we will not cache packages prior to starting so that
		// we don't end up with a package which is actually a class here.
		return PyObject_Call((PyObject*) PyJPPackage_Type, args.get(), NULL);
	}

	return NULL;
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPPackage_setattro(PyJPPackage *pkg, PyObject *attr, PyObject *value)
{
	JP_PY_TRY("PyJPPackage_setattro");
	PyErr_Format(PyExc_AttributeError, "Cannot set attributes on Java packages");
	return NULL;
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPPackage_str(PyJPPackage *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPPackage_str");
	return PyUnicode_FromFormat("<java package '%s'>", self->m_Package->m_Name.c_str());
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPPackage_call(PyJPPackage *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPPackage_call");
	PyErr_Format(PyExc_TypeError, "Package `%s` is not callable.", self->m_Package->m_Name.c_str());
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPPackage_dir(PyJPPackage *self)
{
	JP_PY_TRY("PyJPPackage_dir");
	JPContext* context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	jobject pkg = getPackage(frame, self);
	if (pkg == NULL)
		return NULL;

	jarray o = frame.getPackageContents(pkg);
	Py_ssize_t len = frame.GetArrayLength(o);
	JPPyObject out(JPPyRef::_call, PyList_New(len));
	for (Py_ssize_t i = 0;  i < len; ++i)
	{
		string str = frame.toStringUTF8((jstring)
				frame.GetObjectArrayElement((jobjectArray) o, (jsize) i));
		PyList_SetItem(out.get(), i, PyUnicode_FromFormat("%s", str.c_str()));
	}
	return out.keep();
	JP_PY_CATCH(NULL);
}

static PyMemberDef packageMembers[] = {
	{"__dictoffset__",  T_PYSSIZET, offsetof(PyJPPackage, m_Dict), READONLY},
	{NULL},
};

static PyMethodDef packageMethods[] = {
	{"__dir__", (PyCFunction) PyJPPackage_dir, METH_NOARGS},
	{NULL},
};

static PyType_Slot packageSlots[] = {
	{Py_tp_new,      (void*) PyJPPackage_new},
	{Py_tp_traverse, (void*) PyJPPackage_traverse},
	{Py_tp_clear,    (void*) PyJPPackage_clear},
	{Py_tp_dealloc,  (void*) PyJPPackage_dealloc},
	{Py_tp_getattro, (void*) PyJPPackage_getattro},
	{Py_tp_setattro, (void*) PyJPPackage_setattro},
	{Py_tp_str,      (void*) PyJPPackage_str},
	{Py_tp_call,     (void*) PyJPPackage_call},
	{Py_tp_members,  (void*) packageMembers},
	{Py_tp_methods,  (void*) packageMethods},
	{0}
};

static PyType_Spec packageSpec = {
	"_jpype._JPackage",
	sizeof (PyJPPackage),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	packageSlots
};

#ifdef __cplusplus
}
#endif

void PyJPPackage_initType(PyObject* module)
{
	PyJPPackage_Type = (PyTypeObject*) PyType_FromSpecWithBases(&packageSpec, NULL);
	JP_PY_CHECK_INIT();
	PyModule_AddObject(module, "_JPackage", (PyObject*) PyJPPackage_Type);
	JP_PY_CHECK_INIT();
}
