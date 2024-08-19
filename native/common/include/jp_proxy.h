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
#ifndef _JPPROXY_H_
#define _JPPROXY_H_

struct PyJPProxy;
class JPProxy;
class JPFunctional;

class JPProxy
{
public:
	friend class JPProxyType;
	JPProxy(JPContext* context, PyJPProxy* inst, JPClassList& intf);
	virtual ~JPProxy();

	const JPClassList& getInterfaces() const
	{
		return m_InterfaceClasses;
	}

	jvalue getProxy();

	JPContext* getContext()
	{
		return m_Context;
	}

	virtual JPPyObject getCallable(const string& cname) = 0;
	static void releaseProxyPython(void* host);

protected:
	JPContext*    m_Context;
	PyJPProxy*    m_Instance;
	JPObjectRef   m_Proxy;
	JPClassList   m_InterfaceClasses;
	jweak         m_Ref;
} ;

class JPProxyDirect : public JPProxy
{
public:
	JPProxyDirect(JPContext* context, PyJPProxy* inst, JPClassList& intf);
	~JPProxyDirect() override;
	JPPyObject getCallable(const string& cname) override;
} ;

class JPProxyIndirect : public JPProxy
{
public:
	JPProxyIndirect(JPContext* context, PyJPProxy* inst, JPClassList& intf);
	~JPProxyIndirect() override;
	JPPyObject getCallable(const string& cname) override;
} ;

class JPProxyFunctional : public JPProxy
{
public:
	JPProxyFunctional(JPContext* context, PyJPProxy* inst, JPClassList& intf);
	~JPProxyFunctional() override;
	JPPyObject getCallable(const string& cname) override;
private:
	JPFunctional *m_Functional;
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
	~ JPProxyType() override;

public: // JPClass implementation
	JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) override;

private:
	JPClassRef m_ProxyClass;
	jmethodID  m_GetInvocationHandlerID;
	jfieldID   m_InstanceID;
} ;

#endif // JPPROXY_H