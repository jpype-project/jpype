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
#ifndef _JP_ERROR_H_
#define _JP_ERROR_H_

/* The result of the exception-model split (see plan/ExceptionRefactor.md):
 * one concrete C++ type per exception origin, replacing the single
 * type-tag-plus-union design JPypeException used to have, so each type only
 * carries (and only knows how to convert) the payload that is actually valid
 * for it, instead of relying on convention/comments to track which union
 * member and tag value go together.
 */

/**
 * Shared base: the stack-trace bookkeeping (JP_CATCH's from()) that every
 * origin needs, and nothing origin-specific.
 */
class JPBaseError : public std::runtime_error
{
public:
	explicit JPBaseError(const std::string& msg, const JPStackInfo& stackInfo)
	: std::runtime_error(msg)
	{
		from(stackInfo);
	}

	JPBaseError(const JPBaseError& ex) noexcept = default;
	JPBaseError& operator=(const JPBaseError& ex) = default;
	~JPBaseError() override = default;

	void from(const JPStackInfo& info)
	{
		m_Trace.push_back(info);
	}

	const JPStackTrace& trace() const
	{
		return m_Trace;
	}

	/** Transfer handling of this exception to Python: set the appropriate
	 * Python exception state (PyErr_Occurred() true on return).
	 */
	virtual void toPython() = 0;

	/** Transfer handling of this exception to Java: throw the appropriate
	 * Java exception on the current JNI frame.
	 */
	virtual void toJava() = 0;

	/** Put a captured Python exception back on the thread state, for
	 * callers that need it live (e.g. to chain it as a cause) before they
	 * finish handling this error themselves, rather than going through
	 * toPython()/toJava(). No-op except on JPPythonError.
	 */
	virtual void restorePythonError()
	{
	}

private:
	JPStackTrace m_Trace;
};

/**
 * A Java-originated exception: always carries the live jthrowable that was
 * thrown. Converts to Python by wrapping/finding the matching Python
 * exception class; converts to Java by rethrowing m_Throwable directly.
 */
class JPJavaError : public JPBaseError
{
public:
	JPJavaError(JPJavaFrame& frame, jthrowable th, const JPStackInfo& stackInfo)
	: JPBaseError(frame.toString(th), stackInfo), m_Throwable(frame, th)
	{
	}

	jthrowable getThrowable() const
	{
		return m_Throwable.get();
	}

	void toPython() override;
	void toJava() override;

private:
	JPThrowableRef m_Throwable;
};

/**
 * A Python-originated exception. The exception is fetched and normalized
 * at the moment of the throw (not left live on the thread state for the
 * duration of the C++ unwind - a pending exception left on the thread state
 * is visible to, and can be disturbed by, an incidental GC pass triggered
 * anywhere during that unwind, so fetching claims sole ownership before
 * anything else can touch it), so this type's own invariant is: if
 * constructed, an already-normalized instance is held. Converts to Python
 * by restoring it directly; converts to Java via the existing
 * convertPythonToJava path.
 */
class JPPythonError : public JPBaseError
{
public:
	JPPythonError(JPPyObject excValue, const JPStackInfo& stackInfo)
	: JPBaseError("Python exception", stackInfo), m_PyExcValue(std::move(excValue))
	{
	}

	/** Fetch and normalize the currently-pending Python exception off the
	 * thread state right now, at the moment of the throw, rather than
	 * leaving it live on the thread state for the whole C++ unwind - see
	 * the class-level note above for why. Used by JP_RAISE_PYTHON().
	 */
	static JPPythonError fetch(const JPStackInfo& stackInfo)
	{
		JPPyErrFrame eframe;
		eframe.normalize();
		JPPyObject excValue = eframe.m_ExceptionValue;
		eframe.clear();
		return JPPythonError(std::move(excValue), stackInfo);
	}

	JPPyObject& value()
	{
		return m_PyExcValue;
	}

	void toPython() override;
	void toJava() override;

	void restorePythonError() override
	{
		if (m_PyExcValue.get() != nullptr)
			JPPyErr::restore(m_PyExcValue);
	}

private:
	JPPyObject m_PyExcValue;
};

/**
 * Everything else JPype itself raises directly: a Python exception class
 * plus message to install fresh (formerly _python_exc), or a startup-only
 * OS error (formerly _os_error_unix/_os_error_windows). These don't have
 * an existing external exception object to preserve - they are folded into
 * one type rather than three, since none of them need the same live-object
 * bookkeeping the two hot paths above do.
 */
class JPInternalError : public JPBaseError
{
public:
	JPInternalError(void* pyExcType, const std::string& msg, const JPStackInfo& stackInfo)
	: JPBaseError(msg, stackInfo), m_PyExcType(pyExcType), m_OSErrorCode(0), m_IsOSError(false)
	{
	}

	// GCOVR_EXCL_START
	// This constructor is only used during startup for OSError.
	JPInternalError(const std::string& msg, int osErrorCode, const JPStackInfo& stackInfo)
	: JPBaseError(msg, stackInfo), m_PyExcType(nullptr), m_OSErrorCode(osErrorCode), m_IsOSError(true)
	{
	}
	// GCOVR_EXCL_STOP

	void* getPyExcType() const
	{
		return m_PyExcType;
	}

	bool isOSError() const
	{
		return m_IsOSError;
	}

	int getOSErrorCode() const
	{
		return m_OSErrorCode;
	}

	void toPython() override;
	void toJava() override;

private:
	void* m_PyExcType;
	int m_OSErrorCode;
	bool m_IsOSError;
};

#endif
