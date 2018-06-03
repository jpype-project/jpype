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
#include <Python.h>
#include <jpype.h>

/*
 * FIXME: use a less coupled way to call PyGILState_Ensure/Release()
 * in JPCleaner::~JPCleaner() if we wan't to target a non Python
 * implementation.
 */

JPCleaner::JPCleaner()
{
}

JPCleaner::~JPCleaner()
{
	// FIXME use an exception safe enclosure would be better here
	PyGILState_STATE state = PyGILState_Ensure();

//AT's comments on porting:
// A variety of Unix compilers do not allow redefinition of the same variable in "for" cycles
	for (vector<HostRef*>::iterator cur2 = m_HostObjects.begin(); cur2 != m_HostObjects.end(); cur2++)
	{
		(*cur2)->release();
	}

	PyGILState_Release(state);
}

void JPCleaner::add(HostRef* obj)
{
	m_HostObjects.push_back(obj);
}

void JPCleaner::remove(HostRef* obj)
{
	for (vector<HostRef*>::iterator cur2 = m_HostObjects.begin(); cur2 != m_HostObjects.end(); cur2++)
	{
		if (*cur2 == obj)
		{
			m_HostObjects.erase(cur2);
			return;
		}
	}
}

void JPCleaner::addAll(vector<HostRef*>& r) 
{
	m_HostObjects.insert(m_HostObjects.end(), r.begin(), r.end());
}

void JPCleaner::removeAll(vector<HostRef*>& r)
{
	for (vector<HostRef*>::iterator cur = r.begin(); cur != r.end(); cur++)
	{
		remove(*cur);
	}
}

