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
#ifndef _JPPROXY_H_
#define _JPPROXY_H_

class JPProxy
{
public:
	JPProxy(HostRef* inst, vector<jclass>& intf);

	virtual ~JPProxy()
	{
		if (m_Instance != NULL)
		{
			m_Instance->release();
		}
		JPEnv::getJava()->DeleteGlobalRef(m_Handler);

		for (unsigned int i = 0; i < m_InterfaceClasses.size(); i++)
		{
			JPEnv::getJava()->DeleteGlobalRef(m_InterfaceClasses[i]);
		}

	}

	static void init();

	vector<jclass> getInterfaces()
	{
		return m_InterfaceClasses;
	}

//	jobject getHandler()
//	{
//		return JPEnv::getJava()->NewGlobalRef(m_Handler);
//	}

	jobject getProxy();

private :

	vector<jclass> m_InterfaceClasses;
	jobjectArray   m_Interfaces;
	jobject	       m_Handler;
	HostRef*       m_Instance;
};

#endif // JPPROXY_H
