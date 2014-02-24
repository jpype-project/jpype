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
#ifndef _JPENV_H_
#define _JPENV_H_

/**
 * Simple tample class for managing local java references.
 */
class JPCleaner
{
public :
	JPCleaner();	
	virtual ~JPCleaner();
	
	void addGlobal(jobject r);
	void removeGlobal(jobject r);
	void addAllGlobal(vector<jobject>& r);
	void addAllGlobal(vector<jclass>& r);
	void removeAllGlobal(vector<jobject>& r);

	void addLocal(jobject r);
	void removeLocal(jobject r);
	void addAllLocal(vector<jobject>& r);
	void addAllLocal(vector<jclass>& r);
	void removeAllLocal(vector<jobject>& r);


	void add(HostRef* r);
	void addAll(vector<HostRef*>& r);
	void remove(HostRef* r);
	void removeAll(vector<HostRef*>& r);
	
private :
	vector<jobject>  m_GlobalJavaObjects;
	vector<jobject>  m_LocalJavaObjects;
	vector<HostRef*> m_HostObjects;
};

template<typename T>
class JPMallocCleaner
{
public :
	JPMallocCleaner(size_t size)
	{
		mData = (T*)malloc(sizeof(T)*size);
	}
	
	~JPMallocCleaner()
	{
		free(mData);
	}
	
	T& operator[](size_t ndx)
	{
		return mData[ndx];
	}
	
	T* borrow()
	{
		return mData;
	}
	
private :
	T* mData;
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

	void attachCurrentThread();
	void attachCurrentThreadAsDaemon();
	bool isThreadAttached();

	JPJavaEnv*       getJava();
	HostEnvironment* getHost();

	/**
	* Check if the JPype environment has been initialized
	*/
	bool isInitialized();
	
	void registerRef(HostRef*, HostRef* targetRef);
}

#endif // _JPENV_H_
