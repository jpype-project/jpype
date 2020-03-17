/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#include "jpype.h"
#include "jp_platform.h"

JPPlatformAdapter::~JPPlatformAdapter()
{
}

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

		FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				msgCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) & lpMsgBuf,
				0, NULL );

		std::string res((LPTSTR) lpMsgBuf);

		LocalFree(lpMsgBuf);

		return res;
	}

public:

	virtual void loadLibrary(const char* path) override
	{
		jvmLibrary = LoadLibrary(path);
		if (jvmLibrary == NULL)
		{
			JP_RAISE_OS_ERROR_WINDOWS( GetLastError(), path);
		}
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

	virtual void loadLibrary(const char* path) override
	{
#if defined(_HPUX) && !defined(_IA64)
		jvmLibrary = shl_load(path, BIND_DEFERRED | BIND_VERBOSE, 0L);
#else
		jvmLibrary = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
#endif // HPUX

		if (jvmLibrary == NULL)
		{
			JP_RAISE_OS_ERROR_UNIX( errno, path); // GCOVR_EXCL_LINE
		}
	}

	virtual void unloadLibrary() override
	{
		int r = dlclose(jvmLibrary);
		if (r != 0) // error
		{
			cerr << dlerror() << endl;  // GCOVR_EXCL_LINE
		}
	}

	virtual void* getSymbol(const char* name) override
	{
		void* res = dlsym(jvmLibrary, name);
		if (res == NULL)
		{
			// GCOVR_EXCL_START
			std::stringstream msg;
			msg << "Unable to load symbol [" << name << "], error = " << dlerror();
			JP_RAISE(PyExc_RuntimeError,  msg.str().c_str());
			// GCOVR_EXCL_STOP
		}
		return res;
	}
} ;

#define  PLATFORM_ADAPTER LinuxPlatformAdapter
#endif

namespace
{
PLATFORM_ADAPTER adapter;
}

JPPlatformAdapter* JPPlatformAdapter::getAdapter()
{
	return &adapter;
}
