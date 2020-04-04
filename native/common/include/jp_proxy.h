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

class JPProxy;

class JPProxyFactory
{
	friend class JPProxy;
public:
	explicit JPProxyFactory(JPJavaFrame& frame);
	JPProxy* newProxy(PyObject* inst, JPClassList& intf);

private:
	JPContext* m_Context;
	JPClassRef m_ProxyClass;
	jmethodID m_NewProxyID;
	jmethodID m_NewInstanceID;
} ;

class JPProxy
{
	friend class JPProxyFactory;
	JPProxy(JPProxyFactory* factory, PyObject* inst, JPClassList& intf);

public:
	virtual ~JPProxy();

	const JPClassList& getInterfaces() const
	{
		return m_InterfaceClasses;
	}

	jvalue getProxy();

	JPContext* getContext()
	{
		return m_Factory->m_Context;
	}

private:
	JPProxyFactory* m_Factory;
	PyObject*     m_Instance; // This is a PyJPProxy
	JPObjectRef   m_Proxy;
	JPClassList   m_InterfaceClasses;
	jweak         m_Ref;
} ;

/** Special wrapper for round trip returns
 */
class JPProxyType : public JPClass
{
public:
	JPProxyType(JPJavaFrame& frame,
			jclass clss,
			const string& name,
			JPClass* super,
			JPClassList& interfaces,
			jint modifiers);
	virtual~ JPProxyType();

public: // JPClass implementation
	virtual JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) override;

private:
	JPClassRef m_ProxyClass;
	jmethodID  m_GetInvocationHandlerID;
	jfieldID   m_InstanceID;
} ;

#endif // JPPROXY_H