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
#ifndef _JPENV_H_
#define _JPENV_H_

/**
 *  the platform adapter's implementation is chosen by the JPYPE_??? macros
 */
class JPPlatformAdapter
{
public :
	virtual ~JPPlatformAdapter() {};
	virtual void loadLibrary(const char* path) = 0;
	virtual void unloadLibrary() = 0;
	virtual void* getSymbol(const char* name)= 0;
};

/**
 * Simple tample class for managing host references.
 */
class JPCleaner
{
public :
	JPCleaner();	
	virtual ~JPCleaner();
	
	void add(HostRef* r);
	void addAll(vector<HostRef*>& r);
	void remove(HostRef* r);
	void removeAll(vector<HostRef*>& r);
	
private :
	vector<HostRef*> m_HostObjects;
};

namespace JPEnv
{	
	/**
	* Initialize the JPype subs-system. Does NOT load the JVM
	*/
	void init(HostEnvironment* hostEnv);
	
	/**
	* Load the JVM
	* TODO : add the non-string parameters, for possible callbacks
	*/
	void loadJVM(const string& vmPath, char ignoreUnrecognized, const StringVector& args);

	void attachJVM(const string& vmPath);

	void detachCurrentThread();
	void attachCurrentThread();
	void attachCurrentThreadAsDaemon();
	bool isThreadAttached();
	void assertJVMRunning(const char* function, const char* file, int line);

	HostEnvironment* getHost();

	void setConvertStringObjects(bool flag);
	bool getConvertStringObjects();

	void shutdown();

	/**
	* Check if the JPype environment has been initialized
	*/
	bool isInitialized();

	void CreateJavaVM(void* arg);
	void GetCreatedJavaVM();
}

#endif // _JPENV_H_
