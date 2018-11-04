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
#include <jpype.h>

HostRef::HostRef(void* data, bool acquire)
{
	if (acquire)
	{
		m_HostData = JPEnv::getHost()->acquireRef(data);
	}
	else
	{
		m_HostData = data;
	}
}

HostRef::HostRef(void* data)
{
	m_HostData = JPEnv::getHost()->acquireRef(data);
}

HostRef::~HostRef()
{
	JPEnv::getHost()->releaseRef(m_HostData);
}

HostRef::HostRef(const HostRef& h)
{
	m_HostData = JPEnv::getHost()->acquireRef(h.m_HostData);
}

HostRef& HostRef::operator=(const HostRef& h) {
	m_HostData = JPEnv::getHost()->acquireRef(h.m_HostData);
	return *this;
}

	
HostRef* HostRef::copy()
{
	return new HostRef(m_HostData);
}

void HostRef::release()
{
	delete this;
}

bool HostRef::isNull()
{
	return JPEnv::getHost()->isRefNull(m_HostData);
}

void* HostRef::data()
{
	return m_HostData;
}

