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
#include "jp_platform.h"

JPPlatformAdapter::~JPPlatformAdapter()
= default;

#ifdef WIN32
#include <windows.h>

/**
 * Windows-specific platform adapter
 */
class Win32PlatformAdapter : public JPPlatformAdapter
{
private:
	HINSTANCE jvmLibrary;

	std::string formatMessage(DWORD msgCode)
	{
		LPVOID lpMsgBuf;

		DWORD rc = ::FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				msgCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) & lpMsgBuf,
				0, NULL );

		// Fail to get format
		if (rc == 0)
		{
			std::stringstream ss;
			ss << "error code " << msgCode;
			return ss.str();
		}

		std::string res((LPTSTR) lpMsgBuf);

		LocalFree(lpMsgBuf);

		return res;
	}

public:

	virtual void loadLibrary(const char* path) override
	{
		JP_TRACE_IN("Win32PlatformAdapter::loadLibrary");
		wchar_t *wpath = Py_DecodeLocale(path, NULL);
		if (wpath == NULL)
		{
			JP_RAISE(PyExc_SystemError, "Unable to get JVM path with locale.");
		}
		jvmLibrary = LoadLibraryW(wpath);
		PyMem_RawFree(wpath);
		if (jvmLibrary == NULL)
		{
			JP_RAISE_OS_ERROR_WINDOWS( GetLastError(), path);
		}
		JP_TRACE_OUT;
	}

	virtual void unloadLibrary() override
	{
		// on success return code is nonzero, TODO: handle error?
		FreeLibrary(jvmLibrary);
	}

	virtual void* getSymbol(const char* name) override
	{
		void* res = (void*) GetProcAddress(jvmLibrary, name);
		if (res == NULL)
		{
			std::stringstream msg;
			msg << "Unable to load symbol [" << name << "], error = " << formatMessage(GetLastError());
			JP_RAISE(PyExc_RuntimeError,  msg.str().c_str());
		}
		return res;
	}
} ;

#define  PLATFORM_ADAPTER Win32PlatformAdapter
#else

#if defined(_HPUX) && !defined(_IA64)
#include <dl.h>
#else
#include <dlfcn.h>
#endif // HPUX
#include <errno.h>

// The code in this modules is mostly excluded from coverage as it is only
// possible to execute during a fatal error.

class LinuxPlatformAdapter : public JPPlatformAdapter
{
private:
	void* jvmLibrary;

public:

	void loadLibrary(const char* path) override
	{
		JP_TRACE_IN("LinuxPlatformAdapter::loadLibrary");
#if defined(_HPUX) && !defined(_IA64)
		JP_TRACE("shl_load", path);
		jvmLibrary = shl_load(path, BIND_DEFERRED | BIND_VERBOSE, 0L);
#else
		JP_TRACE("dlopen", path);
		jvmLibrary = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
#endif // HPUX
		// GCOVR_EXCL_START
		if (jvmLibrary == nullptr)
		{
			JP_TRACE("null library");
			JP_TRACE("errno", errno);
			if (errno == ENOEXEC)
			{
				JP_TRACE("dignostics", dlerror());
			}
			JP_RAISE_OS_ERROR_UNIX( errno, path);
		}
		// GCOVR_EXCL_STOP
		JP_TRACE_OUT; // GCOVR_EXCL_LINE
	}

	void unloadLibrary() override
	{
		JP_TRACE_IN("LinuxPlatformAdapter::unloadLibrary");
		int r = dlclose(jvmLibrary);
		// GCOVR_EXCL_START
		if (r != 0) // error
		{
			std::cerr << dlerror() << std::endl;
		}
		// GCOVR_EXCL_STOP
		JP_TRACE_OUT; // GCOVR_EXCL_LINE
	}

	void* getSymbol(const char* name) override
	{
		JP_TRACE_IN("LinuxPlatformAdapter::getSymbol");
		JP_TRACE("Load", name);
		void* res = dlsym(jvmLibrary, name);
		JP_TRACE("Res", res);
		// GCOVR_EXCL_START
		if (res == nullptr)
		{
			JP_TRACE("errno", errno);
			std::stringstream msg;
			msg << "Unable to load symbol [" << name << "], error = " << dlerror();
			JP_RAISE(PyExc_RuntimeError,  msg.str().c_str());
		}
		// GCOVR_EXCL_STOP
		return res;
		JP_TRACE_OUT; // GCOVR_EXCL_LINE
	}
} ;

#define  PLATFORM_ADAPTER LinuxPlatformAdapter
#endif

namespace
{
PLATFORM_ADAPTER* adapter;
}

JPPlatformAdapter* JPPlatformAdapter::getAdapter()
{
	if (adapter == nullptr)
		adapter = new PLATFORM_ADAPTER();
	return adapter;
}
