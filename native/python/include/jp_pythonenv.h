#ifndef JP_PY_ENV_H_
#define JP_PY_ENV_H_
#include <vector>

class JPStackInfo;
struct PyJPMethod;

/** This is all of the calls that are specific to creating and handling
 * the python wrapper classes of jpype.
 */
namespace JPPythonEnv
{
	void init();

	/** Convert a JPClass to the corresponding Python wrapper class.
	 *
	 * @returns a jpype.JavaClass or jpype.JavaArrayClass.
	 */
	JPPyObject newJavaClass(JPClass* jc);

	/** Convert a JPValue to the corresponding Python wrapper class.
	 *
	 * @returns instance of jpype.JavaClass or jpype.JavaArrayClass.
	 */
	JPPyObject newJavaObject(const JPValue& value);

	/** Get the JPValue from a python object.
	 *
	 * @returns a JPValue or NULL if not a JPValue container.
	 */
	JPValue* getJavaValue(PyObject* obj);

	/** Get the JPClass from a Python object.
	 *
	 * Note, always check the getJavaValue before the gettJavaClass.  All objects
	 * inherit __javaclass__ from their Python class. Thus object instances may
	 * appear as classes if not properly checked.

	 *
	 * @returns a JPClass or NULL if not a JPClass container.
	 */
	JPClass* getJavaClass(PyObject* obj);

	/** Get the JPProxy from a Python object.
	 *
	 * @returns JPProxy or NULL in not a JPProxy container.
	 */
	JPProxy* getJavaProxy(PyObject* obj);

	JPPyObject getJavaProxyCallable(PyObject* obj, const string& name);

	/** Register a python resource with jpype.
	 *
	 * @throws if python resource name is not known.
	 */
	void setResource(const string& name, PyObject* resource);

	/** Convert exceptions generated in C++ to Python.
	 *
	 * This is part of the standard exception handling in the module.  The
	 * action depends on the current exception thrown.  JPPythonException
	 * simply return to python as the error state should already be set
	 * properly. JavaException will attempt to convert to a Python type
	 * wrapper appropriate for python to receive as an exception.
	 * JPypeException are converted directly to RuntimeErrors.
	 */
	void rethrow(const JPStackInfo& info);

}

#endif
