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
#ifndef _JPENV_H_
#define _JPENV_H_

/**
 *  the platform adapter's implementation is chosen by the JPYPE_??? macros
 */
class JPPlatformAdapter
{
public:
	virtual ~JPPlatformAdapter();
	virtual void loadLibrary(const char* path) = 0;
	virtual void unloadLibrary() = 0;
	virtual void* getSymbol(const char* name) = 0;

	static JPPlatformAdapter* getAdapter();
} ;

#endif // _JPENV_H_