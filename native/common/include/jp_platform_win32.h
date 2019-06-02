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
#ifndef _PLATFORM_WIN32_H_
#define _PLATFORM_WIN32_H_

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
			JP_RAISE_RUNTIME_ERROR( msg.str().c_str());
		}
		return res;
	}
} ;

#endif // _PLATFORM_WIN32_H_
