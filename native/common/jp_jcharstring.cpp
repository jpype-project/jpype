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

JCharString::JCharString(const jchar* c)
{
	m_Length = 0;
	while (c[m_Length] != 0)
	{
		m_Length ++;
	}
	
	m_Value = new jchar[m_Length+1];
	m_Value[m_Length] = 0;
	for (unsigned int i = 0; i < m_Length; i++)
	{
		m_Value[i] = c[i];
	}
}

JCharString::JCharString(const JCharString& c)
{
	m_Length = c.m_Length;
	m_Value = new jchar[m_Length+1];
	m_Value[m_Length] = 0;
	for (unsigned int i = 0; i < m_Length; i++)
	{
		m_Value[i] = c.m_Value[i];
	}
	
}

JCharString::JCharString(size_t len)
{
	m_Length = len;
	m_Value = new jchar[len+1];
	for (size_t i = 0; i <= len; i++)
	{
		m_Value[i] = 0;
	}
}

JCharString::~JCharString()
{
	if (m_Value != NULL)
	{
		delete[] m_Value;
	}
}
	
const jchar* JCharString::c_str()
{
	return m_Value;
}
 
