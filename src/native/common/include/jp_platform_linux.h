/*****************************************************************************
   Copyright 2004-2008 Steve Menard

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
#ifndef _PLATFORM_LINUX_H_
#define _PLATFORM_LINUX_H_
//AT's comments on porting:
// this file should be better called jp_platform_unix.h
#if defined(_HPUX) && !defined(_IA64)
#include <dl.h>
#else
#include <dlfcn.h>
#endif // HPUX

class LinuxPlatformAdapter : public JPPlatformAdapter
{
private :
	void* jvmLibrary;

public :
	virtual void loadLibrary(const char* path)
	{
#if defined(_HPUX) && !defined(_IA64)
		jvmLibrary = shl_load(path, BIND_DEFERRED|BIND_VERBOSE, 0L);
#else
		jvmLibrary = dlopen(path, RTLD_LAZY|RTLD_GLOBAL);
#endif // HPUX

		if (jvmLibrary == NULL)
		{
			std::stringstream msg;
			msg << "Unable to load DLL [" << path << "], error = " << dlerror();
			RAISE(JPypeException, msg.str().c_str());
		}
	}

	virtual void* getSymbol(const char* name)
	{
		void* res = dlsym(jvmLibrary, name);
		if (res == NULL)
		{
			std::stringstream msg;
			msg << "Unable to load symbol [" << name << "], error = " << dlerror();
			RAISE(JPypeException, msg.str().c_str());
		}
		return res;
	}
};

#endif // _PLATFORM_LINUX_H_
