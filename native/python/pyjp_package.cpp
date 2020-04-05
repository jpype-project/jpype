#include "jpype.h"
#include "pyjp.h"
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

static PyObject *PyJPPackage_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPPackage_new");
	PyJPPackage *self = (PyJPPackage*) type->tp_alloc(type, 0);
	JP_PY_CHECK();

	char *v;
	if (!PyArg_ParseTuple(args, "s", &v))
		return -1;

	self->m_Dict = PyDict_New();
	self->m_Package = new JPPackage(v);
	return self;
	JP_PY_CATCH(NULL);
}

static int PyJPPackage_traverse(PyJPPackage *f, visitproc visit, void *arg)
{
	Py_VISIT(f->m_Dict);
	return 0;
}

static void PyJPPackage_clear(PyJPPackage *f)
{
	Py_CLEAR(f->m_Dict);
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

static PyObject *PyJPPackage_getattro(PyJPPackage *self, PyObject *attr)
{
	JP_PY_TRY("PyJPPackage_getattro");
	if (!PyUnicode_Check(attr))
	{
		PyErr_Format(PyExc_TypeError, "str required for getattr");
		return NULL;
	}

	JPContext* context = JPContext_global;
	PyObject *value = PyObject_GetAttr((PyObject*) self, attr);
	if (value == NULL)
	{
		PyErr_Format(PyExc_AttributeError, "unable to get attribute `%U`", attr);
		return NULL;
	}

	if (context->isRunning())
	{
		JPJavaFrame frame(context);
		if (self->m_Package->m_Object.get() == NULL)
		{
			self->m_Package->m_Object = JPObjectRef(context,
					frame.getPackage(self->m_Package->m_Name));
			if (self->m_Package->m_Object.get() == NULL)
			{
				PyErr_Format(PyExc_AttributeError, "bad Java package");
				return NULL;
			}
		}

		jobject obj = frame.getPackageObject(self->m_Package->m_Object.get(), JPPyString::asStringUTF8(attr));
		PyObject *out = NULL;
		if (frame.IsInstanceOf(obj, context->_java_lang_Class))
			out = PyJPClass_create(frame, frame.findClass((jclass) obj));
		else if (frame.IsInstanceOf(obj, context->_java_lang_String))
		{
			JPPyTuple args = JPPyTuple::newTuple(1);
			JPPyObject u(JPPyRef::_call, PyUnicode_FromFormat("%s.%U", self->m_Package->m_Name, attr));
			args.setItem(0, u.get());
			out = PyObject_Call((PyObject*) PyJPPackage_Type, args.get(), NULL);
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
		PyDict_SetItem(self->m_Dict, attr, out); // This does not steal
		return out;
	} else
	{
		// Prior to starting the JVM we always return a package to be
		// consistent with old behavior.  This is somewhat unsafe as
		// we cannot check if it is a valid package.
		JPPyTuple args = JPPyTuple::newTuple(1);
		JPPyObject u(JPPyRef::_call, PyUnicode_FromFormat("%s.%U", self->m_Package->m_Name, attr));
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
	return PyUnicode_FromFormat("<Java package '%s'>", self->m_Package->m_Name.c_str());
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPPackage_call(PyJPPackage *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPPackage_str");
	PyErr_Format(PyExc_TypeError, "Package `%s` is not callable.", self->m_Package->m_Name.c_str());
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPPackage_dir(PyJPPackage *self)
{
	JP_PY_TRY("PyJPPackage_str");
	JPContext* context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	if (self->m_Package->m_Object.get() == NULL)
	{
		self->m_Package->m_Object = JPObjectRef(context,
				frame.getPackage(self->m_Package->m_Name));
		if (self->m_Package->m_Object.get() == NULL)
		{
			PyErr_Format(PyExc_AttributeError, "bad Java package");
			return NULL;
		}
	}

	jarray o = frame.getPackageContents(self->m_Package->m_Object);
	Py_ssize_t len = frame.GetArrayLength(o);
	JPPyObject out(JPPyRef::_call, PyList_New(len));
	for (Py_ssize_t i = 0;  i < len; ++i)
	{
		string str = frame.toStringUTF8((jstring) frame.GetObjectArrayElement(o, (jsize) i))
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
	{"__dir__", PyJPPackage_dir, METH_NOARGS},
	{NULL},
};

static PyType_Slot packageSlots[] = {
	{Py_tp_new,      (void*) &PyJPPackage_new},
	{Py_tp_dealloc,  (void*) &PyJPPackage_dealloc},
	{Py_tp_traverse, (void*) &PyJPPackage_traverse},
	{Py_tp_clear,    (void*) &PyJPPackage_clear},
	{Py_tp_getattro, (void*) &PyJPPackage_getattro},
	{Py_tp_setattro, (void*) &PyJPPackage_setattro},
	{Py_tp_str,      (void*) &PyJPPackage_str},
	{Py_tp_call,     (void*) &PyJPPackage_call},
	{Py_tp_members,  (void*) packageMembers},
	{0}
};

static PyType_Spec packageSpec = {
	"_jpype._JPackage",
	0,
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
