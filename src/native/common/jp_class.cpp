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

	for (vector<JPClass*>::iterator clit = m_SuperInterfaces.begin(); clit != m_SuperInterfaces.end(); clit ++)
	{
		delete (*clit);
	}

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
	// Is this an interface?
	m_IsInterface = JPJni::isInterface(m_Class);

	loadSuperClass();
	loadSuperInterfaces();
	loadFields();
	loadMethods();
	loadConstructors();
}

void JPClass::loadSuperClass() 
{	
	JPCleaner cleaner;

	// base class .. if any
	if (!m_IsInterface && m_Name.getSimpleName() != "java.lang.Object")
	{
		jclass baseClass = JPEnv::getJava()->GetSuperclass(m_Class);
		cleaner.addLocal(baseClass);

		if (baseClass != NULL) 
		{
			JPTypeName baseClassName = JPJni::getName(baseClass);		
			m_SuperClass = JPTypeManager::findClass(baseClassName);
		}
	}
}
	
void JPClass::loadSuperInterfaces() 
{	
	JPCleaner cleaner;
	// Super interfaces
	vector<jclass> intf = JPJni::getInterfaces(m_Class);

	cleaner.addAllLocal(intf);

	for (vector<jclass>::iterator it = intf.begin(); it != intf.end(); it++)
	{
		JPTypeName intfName = JPJni::getName(*it);
		JPClass* intf = JPTypeManager::findClass(intfName);
		m_SuperInterfaces.push_back(intf);
	}
}
	
void JPClass::loadFields() 
{	
	JPCleaner cleaner;
	// fields
	vector<jobject> fields = JPJni::getDeclaredFields(m_Class);
	cleaner.addAllLocal(fields);

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
}
	
void JPClass::loadMethods() 
{	
	JPCleaner cleaner;
	JPCleaner pcleaner;

	// methods
	vector<jobject> methods = JPJni::getDeclaredMethods(m_Class);
	cleaner.addAllLocal(methods);

	for (vector<jobject>::iterator it = methods.begin(); it != methods.end(); it++)
	{
		string name = JPJni::getMemberName(*it);

		if (JPJni::isMemberPublic(*it) && !JPJni::isMemberAbstract(*it) )
		{
			JPMethod* method = getMethod(name);
			if (method == NULL)
			{
				method = new JPMethod(m_Class, name, false);
				m_Methods[name] = method; 
			}
			
			method->addOverload(this, *it);			
		
		
		}

		
	}

	// add previous overloads to methods defined in THIS class
	if (m_SuperClass != NULL)
	{
		for (map<string, JPMethod*>::iterator cur = m_Methods.begin(); cur != m_Methods.end(); cur ++)
		{
			string name = cur->first;
			JPMethod* superMethod = m_SuperClass->getMethod(name);
			if (superMethod != NULL)
			{
				cur->second->addOverloads(superMethod);
			}
		}
	}		
}

void JPClass::loadConstructors() 
{
	JPCleaner cleaner;

	m_Constructors = new JPMethod(m_Class, "[init", true); 

	if (JPJni::isAbstract(m_Class))
	{
		// NO constructor ...
		return;
	}

	
	vector<jobject> methods = JPJni::getDeclaredConstructors(m_Class);
	cleaner.addAllLocal(methods);

	for (vector<jobject>::iterator it = methods.begin(); it != methods.end(); it++)
	{
		if (JPJni::isMemberPublic(*it))
		{
			m_Constructors->addOverload(this, *it);
		}
	}
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

HostRef* JPClass::getStaticAttribute(string name) 
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
	TRACE_IN("JPClass::asPyObject");
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

EMatchType JPClass::canConvertToJava(HostRef* obj)
{
	if (JPEnv::getHost()->isNone(obj))
	{
		return _implicit;
	}

	JPCleaner cleaner;

	string simpleName = m_Name.getSimpleName();	
	
	if (simpleName == "java.lang.Byte" || simpleName == "java.lang.Short" ||
	    simpleName == "java.lang.Integer")
	{
		if (JPEnv::getHost()->isInt(obj))
		{
			return _explicit;
		}
	}    	
	
	if (simpleName == "java.lang.Long" && JPEnv::getHost()->isLong(obj))
	{
		return _explicit;
	}
	
	if (simpleName == "java.lang.Float" || simpleName == "java.lang.Double")
	{
		if (JPEnv::getHost()->isFloat(obj))
		{
			return _explicit;
		}
	}    	
		
	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* o = JPEnv::getHost()->asObject(obj);

		JPClass* oc = o->getClass();

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

	if (JPEnv::getHost()->isProxy(obj))
	{
		JPProxy* proxy = JPEnv::getHost()->asProxy(obj);
		// Check if any of the interfaces matches ...
		vector<jclass> itf = proxy->getInterfaces();
		for (unsigned int i = 0; i < itf.size(); i++)
		{
			if (JPEnv::getJava()->IsAssignableFrom(itf[i], m_Class))
			{
				return _implicit;
			}
		}
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName o = JPEnv::getHost()->getWrapperTypeName(obj);

		if (o.getSimpleName() == m_Name.getSimpleName())
		{
			return _exact;
		}
	}

	if (m_Name.getSimpleName() == "java.lang.Object")
	{
		// arrays are objects
		if (JPEnv::getHost()->isArray(obj))
		{
			return _implicit;
		}
		
		// Strings are objects too
		if (JPEnv::getHost()->isString(obj))
		{
			return _implicit;
		}
		
		// Class are objects too
		if (JPEnv::getHost()->isClass(obj) || JPEnv::getHost()->isArrayClass(obj))
		{
			return _implicit;
		}

		// Let'a allow primitives (int, long, float and boolean) to convert implicitly too ...
		if (JPEnv::getHost()->isInt(obj))
		{
			return _implicit;
		}
		
		if (JPEnv::getHost()->isLong(obj))
		{
			return _implicit;
		}
		
		if (JPEnv::getHost()->isFloat(obj))
		{
			return _implicit;
		}

		if (JPEnv::getHost()->isBoolean(obj))
		{
			return _implicit;
		}
	}

	return _none;
}

jvalue JPClass::buildObjectWrapper(HostRef* obj)
{
	jvalue res;
	
	JPCleaner cleaner;
	
	vector<HostRef*> args(1);
	args.push_back(obj);

	JPObject* pobj = newInstance(args);

	res.l = pobj->getObject();
	delete pobj;

	return res;
}

jvalue JPClass::convertToJava(HostRef* obj)
{
	jvalue res;
	JPCleaner cleaner;

	res.l = NULL;

	// assume it is convertible;
	if (JPEnv::getHost()->isNone(obj))
	{
		res.l = NULL;
	}

	string simpleName = m_Name.getSimpleName();	
	if (JPEnv::getHost()->isInt(obj) && (simpleName == "java.lang.Byte" || simpleName == "java.lang.Short" ||
	    simpleName == "java.lang.Integer"))
	{
		return buildObjectWrapper(obj);
	}    	
	
	if ((JPEnv::getHost()->isInt(obj) || JPEnv::getHost()->isLong(obj)) && simpleName == "java.lang.Long" && JPEnv::getHost()->isLong(obj))
	{
		return buildObjectWrapper(obj);
	}
	
	if (JPEnv::getHost()->isFloat(obj) && (simpleName == "java.lang.Float" || simpleName == "java.lang.Double"))
	{
		if (JPEnv::getHost()->isFloat(obj))
		{
			return buildObjectWrapper(obj);
		}
	}    	

	if (JPEnv::getHost()->isString(obj))
	{
		JPTypeName name = JPTypeName::fromSimple("java.lang.String");
		JPType* type = JPTypeManager::getType(name);
		return type->convertToJava(obj);
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* ref = JPEnv::getHost()->asObject(obj);
		res.l = ref->getObject();
	}

	if (JPEnv::getHost()->isProxy(obj))
	{
		JPProxy* proxy = JPEnv::getHost()->asProxy(obj);
		res.l = proxy->getProxy();
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		res = JPEnv::getHost()->getWrapperValue(obj);
	}

	if (JPEnv::getHost()->isInt(obj))
	{
		JPTypeName tname = JPTypeName::fromType(JPTypeName::_int);
		JPType* t = JPTypeManager::getType(tname);
		res.l = t->convertToJavaObject(obj);
	}
	
	if (JPEnv::getHost()->isLong(obj))
	{
		JPTypeName tname = JPTypeName::fromType(JPTypeName::_long);
		JPType* t = JPTypeManager::getType(tname);
		res.l = t->convertToJavaObject(obj);
	}
	
	if (JPEnv::getHost()->isFloat(obj))
	{
		JPTypeName tname = JPTypeName::fromType(JPTypeName::_double);
		JPType* t = JPTypeManager::getType(tname);
		res.l = t->convertToJavaObject(obj);
	}

	if (JPEnv::getHost()->isBoolean(obj))
	{
		JPTypeName tname = JPTypeName::fromType(JPTypeName::_boolean);
		JPType* t = JPTypeManager::getType(tname);
		res.l = t->convertToJavaObject(obj);
	}

	if (JPEnv::getHost()->isArray(obj) && simpleName == "java.lang.Object")
	{
		JPArray* a = JPEnv::getHost()->asArray(obj);
		res = a->getValue();		
	}

	return res;
}

JPObject* JPClass::newInstance(vector<HostRef*>& args)
{
	return m_Constructors->invokeConstructor(args);
}

void JPClass::setStaticAttribute(string name, HostRef* val) 
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
	stringstream out;
	JPCleaner cleaner;
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

vector<JPClass*> JPClass::getInterfaces()
{
	vector<JPClass*> res;

	for (vector<JPClass*>::iterator cur = m_SuperInterfaces.begin(); cur != m_SuperInterfaces.end(); cur++)
	{
		JPClass* c = *cur;
		res.push_back(c);
	}
	return res;
}

bool JPClass::isSubclass(JPClass* o)
{
	JPCleaner cleaner;

	jclass jo = o->getClass();
	cleaner.addLocal(jo);

	if (JPEnv::getJava()->IsAssignableFrom(m_Class, jo))
	{
		return true;
	}
	return false;
}
