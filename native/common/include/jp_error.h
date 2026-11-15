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

/* The exception-model split: one concrete C++ type per exception origin,
 * replacing the single
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
	 * Java exception on the current JNI frame. context is the JPContext to
	 * do the throwing through - required since not every origin captures
	 * one at construction time (JPPythonError/JPInternalError can be built
	 * from macro sites with no context in scope, e.g. deep inside generic
	 * conversion code), so the caller - which always has a real context or
	 * frame in hand at the point it is actually catching and converting -
	 * supplies it instead of falling back to an ambient global.
	 */
	virtual void toJava(JPContext* context) = 0;

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
	: JPBaseError(frame.toString(th), stackInfo),
	  m_Context(frame.getContext()),
	  m_Throwable((jthrowable) frame.NewGlobalRef(th))
	{
	}

	// The copy constructor for an object thrown as an exception must be
	// declared noexcept, including any implicitly-defined copy
	// constructors. Any function declared noexcept that terminates by
	// throwing an exception violates ERR55-CPP. Honor exception
	// specifications.
	JPJavaError(const JPJavaError& ex) noexcept
	: JPBaseError(ex), m_Context(ex.m_Context), m_Throwable(nullptr)
	{
		if (m_Context != nullptr && ex.m_Throwable != nullptr && m_Context->isRunning())
		{
			JPJavaFrame frame = JPJavaFrame::outer(m_Context);
			m_Throwable = (jthrowable) frame.NewGlobalRef(ex.m_Throwable);
		}
	}

	JPJavaError& operator=(const JPJavaError& ex) = delete;

	~JPJavaError() override
	{
		// ReleaseGlobalRef is the sanctioned destructor-safe release - it
		// checks m_Context is still running internally, so this cannot fail
		// even if the JVM has since shut down. m_Context itself is captured
		// once at construction (see above) rather than resolved ambiently,
		// so this is safe under multiple sub-interpreters.
		if (m_Context != nullptr && m_Throwable != nullptr)
			m_Context->ReleaseGlobalRef(m_Throwable);
	}

	jthrowable getThrowable() const
	{
		return m_Throwable;
	}

	void toPython() override;
	void toJava(JPContext* context) override;

private:
	JPContext* m_Context;
	jthrowable m_Throwable;
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
	void toJava(JPContext* context) override;

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
	void toJava(JPContext* context) override;

private:
	void* m_PyExcType;
	int m_OSErrorCode;
	bool m_IsOSError;
};

#endif
