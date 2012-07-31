/*****************************************************************************
   Copyright 2004 Steve M�nard

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
#ifndef _PYTHON_ENV_H_
#define _PYTHON_ENV_H_

#define PY_CHECK(op) op; { \
	PyObject* __ex = PyErr_Occurred(); \
	if (__ex) { 	\
		throw new PythonException(); \
	}\
};

/**
 * Exception wrapper for python-generated exceptions
 */
class PythonException : public HostException
{
public :
	PythonException();	
	PythonException(PythonException& ex);

	virtual ~PythonException();
	
	
	bool isJavaException();
	PyObject* getJavaException();
	
public :
	PyObject* m_ExceptionClass;
	PyObject* m_ExceptionValue;
	
};

/** 
 * JPythonEnvHelper
 */
class JPythonEnvHelper
{
};

/** 
 * JPyErr
 */
class JPyErr : public JPythonEnvHelper
{
public :
	static void setString(PyObject*exClass, const char* str);
	static void setObject(PyObject*exClass, PyObject* obj);
	static void check()
	{
		PY_CHECK(;);
	}
};

/** 
 * JPyArg 
 */
class JPyArg : public JPythonEnvHelper
{
public :
	template<typename T1>
	static void parseTuple(PyObject* arg, char* pattern, T1 a1)
	{
		PY_CHECK( PyArg_ParseTuple(arg, pattern, a1) );
	}

	template<typename T1, typename T2>
	static void parseTuple(PyObject* arg, char* pattern, T1 a1, T2 a2)
	{
		PY_CHECK( PyArg_ParseTuple(arg, pattern, a1, a2) );
	}

	template<typename T1, typename T2, typename T3>
	static void parseTuple(PyObject* arg, char* pattern, T1 a1, T2 a2, T3 a3)
	{
		PY_CHECK( PyArg_ParseTuple(arg, pattern, a1, a2, a3) );
	}

	template<typename T1, typename T2, typename T3, typename T4>
	static void parseTuple(PyObject* arg, char* pattern, T1 a1, T2 a2, T3 a3, T4 a4)
	{
		PY_CHECK( PyArg_ParseTuple(arg, pattern, a1, a2, a3, a4)) ;
		
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	static void parseTuple(PyObject* arg, char* pattern, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
	{
		PY_CHECK( PyArg_ParseTuple(arg, pattern, a1, a2, a3, a4, a5)) ;
		
	}

private :
	static void check_exception();

};

/**
 * JPyString
 */
class JPyString : public JPythonEnvHelper
{
public :
	static bool check(PyObject* obj);
	static bool checkStrict(PyObject*);
	static bool checkUnicode(PyObject*);
	static string asString(PyObject* obj);
	static JCharString asJCharString(PyObject* obj);
	static Py_ssize_t AsStringAndSize(PyObject *obj, char **buffer, Py_ssize_t *);
	static Py_UNICODE* AsUnicode(PyObject *obj);

	static PyObject* fromUnicode(const jchar*, int);
	static PyObject* fromString(const char*);
};

/**
 * JPyTuple
 */
class JPySequence : public JPythonEnvHelper
{
public :
	static bool check(PyObject* obj);
	static PyObject* newTuple(Py_ssize_t ndx);
	static PyObject* newList(Py_ssize_t ndx);
	
	static PyObject* getItem(PyObject* tuple, Py_ssize_t ndx);
	static void setItem(PyObject* tuple, Py_ssize_t ndx, PyObject* val);
};

/**
 * JPyObject
 */
class JPyObject : public JPythonEnvHelper
{
public :
	static Py_ssize_t length(PyObject* obj);
	static bool hasAttr(PyObject*, PyObject*);
	static PyObject* getAttr(PyObject*, PyObject*);
	static PyObject* call(PyObject*, PyObject*, PyObject*);
	static PyObject* getAttrString(PyObject*, const char*);
	static void setAttrString(PyObject*, const char*, PyObject *);
	static bool isInstance(PyObject* obj, PyObject* t);
	static bool isSubclass(PyObject* obj, PyObject* t);

};

class JPyInt : public JPythonEnvHelper
{
public :
	static PyObject* fromLong(long l);	
	static bool check(PyObject*);
	static long asLong(PyObject*);
};

class JPyLong : public JPythonEnvHelper
{
public :
	static PyObject* fromLongLong(PY_LONG_LONG l);	
	static bool check(PyObject*);
	static PY_LONG_LONG asLongLong(PyObject*);
};

class JPyBoolean : public JPythonEnvHelper
{
public :
	static bool isTrue(PyObject* o)
	{
		return o == Py_True;
	}
	
	static bool isFalse(PyObject* o)
	{
		return o == Py_False;
	}

	static PyObject* getTrue();	
	static PyObject* getFalse();		
};

class JPyFloat : public JPythonEnvHelper
{
public :
	static PyObject* fromDouble(double l);	
	static bool check(PyObject*);
	static double asDouble(PyObject*);
};

class JPyDict : public JPythonEnvHelper
{
public :
	static bool check(PyObject* obj);
	static bool contains(PyObject*, PyObject*);
	static PyObject* getItem(PyObject*, PyObject*);
	static PyObject* getKeys(PyObject*);
	static PyObject* copy(PyObject*);

	static PyObject* newInstance();
	static void setItemString(PyObject*, PyObject*, const char*);
};

class JPyCObject : public JPythonEnvHelper
{
public :
	static bool check(PyObject* obj);
	static PyObject* fromVoid(void* data, void (*destr)(void *));
	static PyObject* fromVoidAndDesc(void* data, void* desc, void (*destr)(void *, void*));
	static void* asVoidPtr(PyObject*);
	static void* getDesc(PyObject*);
};

class JPyType : public JPythonEnvHelper
{
public :
	static bool check(PyObject* obj);
	static bool isSubclass(PyObject*, PyObject*);
};

class JPyHelper : public JPythonEnvHelper
{
public :
	static void dumpSequenceRefs(PyObject* seq, const char* comment); 
};

#undef PY_CHECK

#define PY_STANDARD_CATCH \
catch(JavaException* ex) \
{ \
	try { \
		JPypeJavaException::errorOccurred(); \
	} \
	catch(...) \
	{ \
		JPEnv::getHost()->setRuntimeException("An unknown error occured while handling a Java Exception"); \
	}\
	delete ex; \
}\
catch(JPypeException* ex)\
{\
	try { \
		JPEnv::getHost()->setRuntimeException(ex->getMsg()); \
	} \
	catch(...) \
	{ \
		JPEnv::getHost()->setRuntimeException("An unknown error occured while handling a JPype Exception"); \
	}\
	delete ex; \
}\
catch(PythonException* ex) \
{ \
	delete ex; \
} \
catch(...) \
{\
	JPEnv::getHost()->setRuntimeException("Unknown Exception"); \
} \

#define PY_LOGGING_CATCH \
catch(JavaException* ex) \
{ \
	try { \
	cout << "Java error occured : " << ex->message << endl; \
		JPypeJavaException::errorOccurred(); \
	} \
	catch(...) \
	{ \
		JPEnv::getHost()->setRuntimeException("An unknown error occured while handling a Java Exception"); \
	}\
	delete ex; \
}\
catch(JPypeException* ex)\
{\
	try { \
		cout << "JPype error occured" << endl; \
		JPEnv::getHost()->setRuntimeException(ex->getMsg()); \
	} \
	catch(...) \
	{ \
		JPEnv::getHost()->setRuntimeException("An unknown error occured while handling a JPype Exception"); \
	}\
	delete ex; \
}\
catch(PythonException* ex) \
{ \
	cout << "Pyhton error occured" << endl; \
	delete ex; \
} \
catch(...) \
{\
	cout << "Unknown error occured" << endl; \
	JPEnv::getHost()->setRuntimeException("Unknown Exception"); \
} \

#endif // _PYTHON_ENV_H_
