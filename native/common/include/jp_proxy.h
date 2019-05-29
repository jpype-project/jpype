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

class JPProxyFactory
{
public:
	void init(JPContext* context);
	JPProxy* newProxy(PyObject* inst, JPClassList& intf);
	
private:
	JPContext* m_Context;
	jclass handlerClass;
	jmethodID invocationHandlerConstructorID;
	jfieldID hostObjectID;
	jfieldID contextID;
};
	
class JPProxy
{
	friend class JPProxyFactory;
	JPProxy(JPContext* context, PyObject* inst, JPClassList& intf);
	
public:
	virtual ~JPProxy();

	const JPClassList& getInterfaces() const
	{
		return m_InterfaceClasses;
	}

	jobject getProxy();

	JPContext* getContext()
	{
		return m_Context;
	}

private:
	JPContext*    m_Context;
	jobject	      m_Handler;
	PyObject*     m_Instance; // This is a PyJPProxy
	JPClassList   m_InterfaceClasses;
	JPObjectRef   m_Interfaces;
} ;

#endif // JPPROXY_H
