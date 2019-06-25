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

#include "jp_class.h"

class JPProxy
{
public:
	JPProxy(PyObject* inst, JPClass::ClassList& intf);

	virtual ~JPProxy();

	static void init();

	const JPClass::ClassList& getInterfaces() const
	{
		return m_InterfaceClasses;
	}

	jobject getProxy();

private:
	jobject	      m_Handler;
	PyObject*     m_Instance; // This is a PyJPProxy
	JPClass::ClassList m_InterfaceClasses;
	JPObjectRef   m_Interfaces;
} ;

// Special wrapper for round trip returns
class JPProxyType : public JPClass
{
public:
	JPProxyType();
	virtual~ JPProxyType();

public: // JPClass implementation
	virtual JPPyObject convertToPythonObject(jvalue val) override;
} ;


#endif // JPPROXY_H
