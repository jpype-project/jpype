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
	friend class JPProxyInstance;
	JPProxy(JPJavaFrame& frame, PyJPProxy* inst, JPClassList& intf, bool convert);
	virtual ~JPProxy();

	const JPClassList& getInterfaces() const
	{
		return m_InterfaceClasses;
	}

	jvalue getProxy(JPJavaFrame& frame);

	virtual JPPyObject getCallable(JPContext* context, PyObject* name, int& addSelf) = 0;
	static void releaseProxyPython(void* host);

	PyJPProxy* getInstance()
	{
		return m_Instance;
	}

	PyJPProxy*    m_Instance;
	jref          m_ProxyType;
	JPClassList   m_InterfaceClasses;
	jweak         m_Ref;
	JPContext*    m_Context;
} ;

class JPProxyDirect : public JPProxy
{
public:
	JPProxyDirect(JPJavaFrame& frame, PyJPProxy* inst, JPClassList& intf, bool convert)
		: JPProxy(frame, inst, intf, convert) {}
	~JPProxyDirect() override;
	JPPyObject getCallable(JPContext* context, PyObject* name, int& addSelf) override;
} ;

class JPProxyIndirectAttr : public JPProxy
{
public:
	JPProxyIndirectAttr(JPJavaFrame& frame, PyJPProxy* inst, JPClassList& intf, bool convert)
		: JPProxy(frame, inst, intf, convert) {}
	~JPProxyIndirectAttr() override;
	JPPyObject getCallable(JPContext* context, PyObject* name, int& addSelf) override;
} ;

class JPProxyIndirectDict : public JPProxy
{
public:
	JPProxyIndirectDict(JPJavaFrame& frame, PyJPProxy* inst, JPClassList& intf, bool convert)
		: JPProxy(frame, inst, intf, convert) {}
	~JPProxyIndirectDict() override;
	JPPyObject getCallable(JPContext* context, PyObject* name, int& addSelf) override;
} ;

class JPProxyFunctional : public JPProxy
{
public:
	JPProxyFunctional(JPJavaFrame& frame, PyJPProxy* inst, JPClassList& intf);
	~JPProxyFunctional() override;
	JPPyObject getCallable(JPContext* context, PyObject* name, int& addSelf) override;
private:
	JPFunctional *m_Functional;
} ;

/** Special wrapper for round trip returns
 */
class JPProxyInstance : public JPClass
{
public:
	JPProxyInstance(JPJavaFrame& frame,
			jclass clss,
			const string& name,
			JPClass* super,
			JPClassList& interfaces,
			jint modifiers);
	~JPProxyInstance() override;

public: // JPClass implementation
//	JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) override;

private:
	jclass m_ProxyClass; // FIXME I think this goes away
} ;

#endif // JPPROXY_H
