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
#include <jpype.h>

JPClass::JPClass(const JPTypeName& n, jclass c) :
	JPClassBase(n, c),
	m_SuperClass(NULL),
	m_Constructors(NULL)
{
}

JPClass::~JPClass()
{
	delete m_Constructors;

	// interfaces of this cannot be simply deleted here, since they may be also
	// super types of other classes, which would be invalidated by doing so.

	for (map<string, JPMethod*>::iterator mthit = m_Methods.begin(); mthit != m_Methods.end(); mthit ++)
	{
		delete mthit->second;
	}

	for (map<string, JPField*>::iterator fldit = m_InstanceFields.begin(); fldit != m_InstanceFields.end(); fldit++)
	{
		delete fldit->second;
	}

	for (map<string, JPField*>::iterator fldit2 = m_StaticFields.begin(); fldit2 != m_StaticFields.end(); fldit2++)
	{
		delete fldit2->second;
	}

}

void JPClass::postLoad()
{
	TRACE_IN("JPClass::postLoad");
	// Is this an interface?
	m_IsInterface = JPJni::isInterface(m_Class);

	loadSuperClass();
	loadSuperInterfaces();
	loadFields();
	loadMethods();
	loadConstructors();
	TRACE_OUT;
}

void JPClass::loadSuperClass()
{
	TRACE_IN("JPClass::loadSuperClass");
	JPLocalFrame frame;

	// base class .. if any
	if (!m_IsInterface && m_Name.getSimpleName() != "java.lang.Object")
	{
		jclass baseClass = JPEnv::getJava()->GetSuperclass(m_Class);

		if (baseClass != NULL)
		{
			JPTypeName baseClassName = JPJni::getName(baseClass);
			m_SuperClass = JPTypeManager::findClass(baseClassName);
		}
	}
	TRACE_OUT;
}

void JPClass::loadSuperInterfaces()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPClass::loadSuperInterfaces");
	// Super interfaces
	vector<jclass> intf = JPJni::getInterfaces(frame, m_Class);

	for (vector<jclass>::iterator it = intf.begin(); it != intf.end(); it++)
	{
		JPTypeName intfName = JPJni::getName(*it);
		JPClass* interface = JPTypeManager::findClass(intfName);
		m_SuperInterfaces.push_back(interface);
	}
	TRACE_OUT;
}

void JPClass::loadFields()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPClass::loadFields");
	// fields
	vector<jobject> fields = JPJni::getDeclaredFields(frame, m_Class);

	for (vector<jobject>::iterator it = fields.begin(); it != fields.end(); it++)
	{
		JPField* field = new JPField(this, *it);
		if (field->isStatic())
		{
			m_StaticFields[field->getName()] = field;
		}
		else
		{
			m_InstanceFields[field->getName()] = field;
		}
	}
	TRACE_OUT;
}

void JPClass::loadMethods()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPClass::loadMethods");

	// methods
	vector<jobject> methods = JPJni::getMethods(frame, m_Class);

	for (vector<jobject>::iterator it = methods.begin(); it != methods.end(); it++)
	{
		const string& name = JPJni::getMemberName(*it);
		JPMethod* method = getMethod(name);
		if (method == NULL)
		{
			method = new JPMethod(m_Class, name, false);
			m_Methods[name] = method;
		}

		method->addOverload(this, *it);
	}
	TRACE_OUT;
}

void JPClass::loadConstructors()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPClass::loadMethods");
	m_Constructors = new JPMethod(m_Class, "[init", true);

	if (JPJni::isAbstract(m_Class))
	{
		// NO constructor ...
		return;
	}


	vector<jobject> methods = JPJni::getDeclaredConstructors(frame, m_Class);

	for (vector<jobject>::iterator it = methods.begin(); it != methods.end(); it++)
	{
		if (JPJni::isMemberPublic(*it))
		{
			m_Constructors->addOverload(this, *it);
		}
	}
	TRACE_OUT;
}

JPField* JPClass::getInstanceField(const string& name)
{
	map<string, JPField*>::iterator it = m_InstanceFields.find(name);
	if (it == m_InstanceFields.end())
	{
		return NULL;
	}
	return it->second;
}

JPField* JPClass::getStaticField(const string& name)
{
	map<string, JPField*>::iterator it = m_StaticFields.find(name);
	if (it == m_StaticFields.end())
	{
		return NULL;
	}
	return it->second;
}


JPMethod* JPClass::getMethod(const string& name)
{
	map<string, JPMethod*>::iterator it = m_Methods.find(name);
	if (it == m_Methods.end())
	{
		return NULL;
	}

	return it->second;
}

HostRef* JPClass::getStaticAttribute(const string& name)
{
	// static fields
	map<string, JPField*>::iterator fld = m_StaticFields.find(name);
	if (fld != m_StaticFields.end())
	{
		return fld->second->getStaticAttribute();
	}

	JPEnv::getHost()->setAttributeError(name.c_str());
	JPEnv::getHost()->raise("getAttribute");
	return NULL; // never reached
}

HostRef* JPClass::asHostObject(jvalue obj)
{
	TRACE_IN("JPClass::asHostObject");
	if (obj.l == NULL)
	{
		return JPEnv::getHost()->getNone();
	}

	JPTypeName name = JPJni::getClassName(obj.l);
	if (name.getType() ==JPTypeName::_array)
	{
		JPType* arrayType = JPTypeManager::getType(name);
		return arrayType->asHostObject(obj);
	}

	return JPEnv::getHost()->newObject(new JPObject(name, obj.l));
	TRACE_OUT;
}

const char* java_lang = "java.lang.";
EMatchType JPClass::canConvertToJava(HostRef* obj)
{
	JPLocalFrame frame;
	TRACE_IN("JPClass::canConvertToJava");
	if (JPEnv::getHost()->isNone(obj))
	{
		return _implicit;
	}

	const string& simpleName = m_Name.getSimpleName();
	TRACE2("Simple name", simpleName);

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* o = JPEnv::getHost()->asObject(obj);
		JPClass* oc = o->getClass(); 
		TRACE2("Match name", oc->m_Name.getSimpleName());

		if (oc == this)
		{
			// hey, this is me! :)
			return _exact;
		}

		if (JPEnv::getJava()->IsAssignableFrom(oc->m_Class, m_Class))
		{
			return _implicit;
		}
	}

	if (simpleName.compare(0, 10, java_lang)==0)
	{
		if (simpleName == "java.lang.Byte" || simpleName == "java.lang.Short" ||
		    simpleName == "java.lang.Integer")
		{
			if (JPEnv::getHost()->isInt(obj))
			{
				TRACE1("explicit int");
				return _explicit;
			}
		}
	
		if (simpleName == "java.lang.Long" && JPEnv::getHost()->isLong(obj))
		{
			TRACE1("explicit long");
			return _explicit;
		}
	
		if (simpleName == "java.lang.Float" || simpleName == "java.lang.Double")
		{
			if (JPEnv::getHost()->isFloat(obj))
			{
				TRACE1("explicit float");
				return _explicit;
			}
		}

		// Handle a Python class which wraps java class 
		if (simpleName == "java.lang.Class")
		{
			if (JPEnv::getHost()->isClass(obj))
			{
				return _exact;
			}
		}
	
		if (simpleName == "java.lang.Object")
		{
			// arrays are objects
			if (JPEnv::getHost()->isArray(obj))
			{
				TRACE1("From array");
				return _implicit;
			}
	
			// Strings are objects too
			if (JPEnv::getHost()->isString(obj))
			{
				TRACE1("From string");
				return _implicit;
			}
	
			// Class are objects too
			if (JPEnv::getHost()->isClass(obj) || JPEnv::getHost()->isArrayClass(obj))
			{
				TRACE1("implicit array class");
				return _implicit;
			}
	
			// Let'a allow primitives (int, long, float and boolean) to convert implicitly too ...
			if (JPEnv::getHost()->isInt(obj))
			{
				TRACE1("implicit int");
				return _implicit;
			}
	
			if (JPEnv::getHost()->isLong(obj))
			{
				TRACE1("implicit long");
				return _implicit;
			}
	
			if (JPEnv::getHost()->isFloat(obj))
			{
				TRACE1("implicit float");
				return _implicit;
			}
	
			if (JPEnv::getHost()->isBoolean(obj))
			{
				TRACE1("implicit boolean");
				return _implicit;
			}
		}
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName o = JPEnv::getHost()->getWrapperTypeName(obj);

		if (o.getSimpleName() == m_Name.getSimpleName())
		{
			TRACE1("exact wrapper");
			return _exact;
		}
	}

	if (JPEnv::getHost()->isProxy(obj))
	{
		JPProxy* proxy = JPEnv::getHost()->asProxy(obj);
		// Check if any of the interfaces matches ...
		vector<jclass> itf = proxy->getInterfaces();
		for (unsigned int i = 0; i < itf.size(); i++)
		{
			if (JPEnv::getJava()->IsAssignableFrom(itf[i], m_Class))
			{
				TRACE1("implicit proxy");
				return _implicit;
			}
		}
	}

	return _none;
	TRACE_OUT;
}

jobject JPClass::buildObjectWrapper(HostRef* obj)
{
	JPLocalFrame frame;

	vector<HostRef*> args(1);
	args.push_back(obj);

	JPObject* pobj = newInstance(args);
	jobject out = pobj->getObject();
	delete pobj;

	return frame.keep(out);
}

jvalue JPClass::convertToJava(HostRef* obj)
{
	TRACE_IN("JPClass::convertToJava");
	JPLocalFrame frame;
	jvalue res;

	res.l = NULL;
	const string& simpleName = m_Name.getSimpleName();

	// assume it is convertible;
	if (JPEnv::getHost()->isNone(obj))
	{
		res.l = NULL;
		return res;
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* ref = JPEnv::getHost()->asObject(obj);
		res.l = frame.keep(ref->getObject());
		return res;
	}

	if (simpleName.compare(0, 10, java_lang)==0)
	{
		if ( 
				   (simpleName == "java.lang.Byte" && JPEnv::getHost()->isInt(obj) )
				|| (simpleName == "java.lang.Short" && JPEnv::getHost()->isInt(obj))
				|| (simpleName == "java.lang.Integer" && JPEnv::getHost()->isInt(obj))
				|| (simpleName == "java.lang.Long" && 
						(JPEnv::getHost()->isInt(obj) || JPEnv::getHost()->isLong(obj) ))
				|| (simpleName == "java.lang.Float" && (JPEnv::getHost()->isFloat(obj) 
						|| JPEnv::getHost()->isInt(obj) 
						|| JPEnv::getHost()->isLong(obj) ))
				|| (simpleName == "java.lang.Double" && (JPEnv::getHost()->isFloat(obj) 
						|| JPEnv::getHost()->isInt(obj) 
						|| JPEnv::getHost()->isLong(obj) ))
			 )
		{
			res.l = frame.keep(buildObjectWrapper(obj));
			return res;
		}
	
		if (JPEnv::getHost()->isString(obj))
		{
			JPTypeName name = JPTypeName::fromSimple("java.lang.String");
			JPType* type = JPTypeManager::getType(name);
			res = type->convertToJava(obj);
			res.l = frame.keep(res.l);
			return res;
		}

		if (simpleName == "java.lang.Class")
		{
			JPClass* w = JPEnv::getHost()->asClass(obj);
		  jclass lr = w->getClass();
		  res.l = lr;
		}

		if (simpleName == "java.lang.Object")
		{
			if (JPEnv::getHost()->isInt(obj))
			{
				JPTypeName tname = JPTypeName::fromType(JPTypeName::_int);
				JPType* t = JPTypeManager::getType(tname);
				res.l = frame.keep(t->convertToJavaObject(obj));
				return res;
			}

			else if (JPEnv::getHost()->isLong(obj))
			{
				JPTypeName tname = JPTypeName::fromType(JPTypeName::_long);
				JPType* t = JPTypeManager::getType(tname);
				res.l = frame.keep(t->convertToJavaObject(obj));
				return res;
			}

			else if (JPEnv::getHost()->isFloat(obj))
			{
				JPTypeName tname = JPTypeName::fromType(JPTypeName::_double);
				JPType* t = JPTypeManager::getType(tname);
				res.l = frame.keep(t->convertToJavaObject(obj));
				return res;
			}

			else if (JPEnv::getHost()->isBoolean(obj))
			{
				JPTypeName tname = JPTypeName::fromType(JPTypeName::_boolean);
				JPType* t = JPTypeManager::getType(tname);
				res.l = frame.keep(t->convertToJavaObject(obj));
				return res;
			}

			else if (JPEnv::getHost()->isArray(obj) && simpleName == "java.lang.Object")
			{
				JPArray* a = JPEnv::getHost()->asArray(obj);
				res = a->getValue();
				res.l = frame.keep(res.l);
				return res;
			}

			else if (JPEnv::getHost()->isClass(obj))
			{
				JPTypeName name = JPTypeName::fromSimple("java.lang.Class");
				JPType* type = JPTypeManager::getType(name);
				res.l = frame.keep(type->convertToJavaObject(obj));
				return res;
			}
		}
	}

	if (JPEnv::getHost()->isProxy(obj))
	{
		JPProxy* proxy = JPEnv::getHost()->asProxy(obj);
		res.l = frame.keep(proxy->getProxy());
		return res;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		res = JPEnv::getHost()->getWrapperValue(obj); // FIXME isn't this one global already
		res.l = frame.keep(res.l);
		return res;
	}

	return res;
	TRACE_OUT;
}

JPObject* JPClass::newInstance(vector<HostRef*>& args)
{
	return m_Constructors->invokeConstructor(args);
}

void JPClass::setStaticAttribute(const string& name, HostRef* val)
{
	map<string, JPField*>::iterator it = m_StaticFields.find(name);
	if (it == m_StaticFields.end())
	{
		JPEnv::getHost()->setAttributeError(name.c_str());
		JPEnv::getHost()->raise("__setattr__");
	}

	it->second->setStaticAttribute(val);
}

string JPClass::describe()
{
	JPLocalFrame frame;
	stringstream out;
	out << "public ";
	if (isAbstract())
	{
		out << "abstract ";
	}
	if (isFinal())
	{
		out << "final ";
	}

	out << "class " << m_Name.getSimpleName();
	if (m_SuperClass != NULL)
	{
		out << " extends " << m_SuperClass->getName().getSimpleName();
	}

	if (m_SuperInterfaces.size() > 0)
	{
		out << " implements";
		bool first = true;
		for (vector<JPClass*>::iterator itf = m_SuperInterfaces.begin(); itf != m_SuperInterfaces.end(); itf++)
		{
			if (!first)
			{
				out << ",";
			}
			else
			{
				first = false;
			}
			JPClass* pc = *itf;
			out << " " << pc->getName().getSimpleName();
		}
	}
	out << endl << "{" << endl;

	// Fields
	out << "  // Accessible Static Fields" << endl;
	for (map<string, JPField*>::iterator curField = m_StaticFields.begin(); curField != m_StaticFields.end(); curField++)
	{
		JPField* f = curField->second;
		out << "  public static ";
		if (f->isFinal())
		{
			out << "final ";
		}
		out << f->getType().getSimpleName() << " " << f->getName() << ";" << endl;
	}
	out << endl;
	out << "  // Accessible Instance Fields" << endl;
	for (map<string, JPField*>::iterator curInstField = m_InstanceFields.begin(); curInstField != m_InstanceFields.end(); curInstField++)
	{
		JPField* f = curInstField->second;
		out << "  public ";
		if (f->isFinal())
		{
			out << "final ";
		}
		out << f->getType().getSimpleName() << " " << f->getName() << ";" << endl;
	}
	out << endl;

	// Constructors
	out << "  // Accessible Constructors" << endl;
	out << m_Constructors->describe("  ") << endl;

	out << "  // Accessible Methods" << endl;
	for (map<string, JPMethod*>::iterator curMethod = m_Methods.begin(); curMethod != m_Methods.end(); curMethod++)
	{
		JPMethod* f = curMethod->second;
		out << f->describe("  ");
		out << endl;
	}
	out << "}";

	return out.str();
}

bool JPClass::isAbstract()
{
	return JPJni::isAbstract(m_Class);
}

bool JPClass::isFinal()
{
	return JPJni::isFinal(m_Class);
}

JPClass* JPClass::getSuperClass()
{
	return m_SuperClass;
}

const vector<JPClass*>& JPClass::getInterfaces() const
{
	return m_SuperInterfaces;
}

bool JPClass::isSubclass(JPClass* o)
{
	JPLocalFrame frame;

	jclass jo = o->getClass();

	return JPEnv::getJava()->IsAssignableFrom(m_Class, jo);
}
