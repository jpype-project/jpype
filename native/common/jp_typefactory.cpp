/*****************************************************************************
   Copyright 2019 Karl Einar Nelson

   Licensed under the Apache License, Version 2.0 (the "License
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
#include <jp_primitive_common.h>

JPVoidType *_void = new JPVoidType();
JPByteType *_byte = new JPByteType();
JPBooleanType *_boolean = new JPBooleanType();
JPCharType *_char = new JPCharType();
JPShortType *_short = new JPShortType();
JPIntType *_int = new JPIntType();
JPLongType *_long = new JPLongType();
JPFloatType *_float = new JPFloatType();
JPDoubleType *_double = new JPDoubleType();

void JPTypeFactory_rethrow(JPJavaFrame& frame)
{
	try
	{
		throw;
	} catch (JPypeException& ex)
	{
		ex.toJava(frame.getContext());
	} catch (...)
	{
		frame.ThrowNew(frame.getContext()->_java_lang_RuntimeException.get(),
				"unknown error occurred");
	}
}

template <class T> void convert(JPJavaFrame& frame, jlongArray array, vector<T>& out)
{
	JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, array,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);
	jlong* values = accessor.get();
	jsize sz = frame.GetArrayLength(array);
	out.resize(sz);
	for (int i = 0; i < sz; ++i)
	{
		out[i] = (T) values[i];
	}
	return;
}

JNIEXPORT void JNICALL JPTypeFactory_destroy(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlongArray resources,
		jint sz)
{
	JP_TRACE_IN_C("JPTypeFactory_destroy");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	try
	{
		JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, resources,
				&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);
		jlong* values = accessor.get();
		for (int i = 0; i < sz; ++i)
		{
			// Do not delete the shared primitive types.
			if (dynamic_cast<JPPrimitiveType*> ((JPResource*) values[i]) != 0)
				continue;
			delete (JPResource*) values[i];
		}
		return;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return;
	JP_TRACE_OUT_C;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineMethodDispatch(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong clsPtr,
		jstring name,
		jlongArray overloadPtrs,
		jint modifiers)
{
	JP_TRACE_IN_C("JPTypeFactory_defineMethodDispatch");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	try
	{
		JPClass* cls = (JPClass*) clsPtr;
		JPMethodList overloadList;
		convert(frame, overloadPtrs, overloadList);
		string cname = context->toStringUTF8(name);
		JP_TRACE(cname);
		JPMethodDispatch* dispatch = new JPMethodDispatch(cls, cname, overloadList, modifiers);
		return (jlong) dispatch;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT_C;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineArrayClass(
		JNIEnv *env, jobject self, jlong contextPtr,
		jclass cls,
		jstring name,
		jlong superClass,
		jlong componentClass,
		jint modifiers)
{
	JP_TRACE_IN_C("JPTypeFactory_defineArrayClass");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	try
	{
		string cname = context->toStringUTF8(name);
		JP_TRACE(cname);
		JPArrayClass* result = new JPArrayClass(context, cls,
				cname,
				(JPClass*) superClass,
				(JPClass*) componentClass,
				modifiers);
		return (jlong) result;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT_C;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineObjectClass(
		JNIEnv *env, jobject self, jlong contextPtr,
		jclass cls,
		jstring name,
		jlong superClass,
		jlongArray interfacePtrs,
		jint modifiers)
{
	JP_TRACE_IN_C("JPTypeFactory_defineObjectClass");
	JPContext* context = (JPContext*) contextPtr;
	JP_TRACE("got context", context);
	JPJavaFrame frame(context, env);
	try
	{
		string className = context->toStringUTF8(name);
		JP_TRACE(className);
		JPClassList interfaces;
		if (interfacePtrs != NULL)
			convert(frame, interfacePtrs, interfaces);
		JPClass* result = NULL;
		if (JPModifier::isSpecial(modifiers))
		{
			// Certain classes require special implementations
			if (className == "java.lang.Object")
				return (jlong) (context->_java_lang_Object
					= new JPObjectType(context, cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Class")
				return (jlong) (context->_java_lang_Class
					= new JPClassType(context, cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.String")
				return (jlong) (context->_java_lang_String
					= new JPStringType(context, cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Throwable")
				return (jlong) (context->_java_lang_Throwable
					= new JPObjectType(context, cls, className, (JPClass*) superClass, interfaces, modifiers));

			// Register the box types
			if (className == "java.lang.Void")
				return (jlong) (context->_java_lang_Void
					= new JPBoxedType(context, cls, className, (JPClass*) superClass, interfaces, modifiers, _void));
			if (className == "java.lang.Boolean")
			{
				context->m_BooleanValueID = frame.GetMethodID(cls, "booleanValue", "()Z");
				return (jlong) (context->_java_lang_Boolean
						= new JPBoxedType(context, cls, className, (JPClass*) superClass, interfaces, modifiers, _boolean));
			}
			if (className == "java.lang.Byte")
			{
				context->m_ByteValueID = frame.GetMethodID(cls, "byteValue", "()B");
				return (jlong) (context->_java_lang_Byte
						= new JPBoxedType(context, cls, className, (JPClass*) superClass, interfaces, modifiers, _byte));
			}
			if (className == "java.lang.Character")
			{
				context->m_CharValueID = frame.GetMethodID(cls, "charValue", "()C");
				return (jlong) (context->_java_lang_Char
						= new JPBoxedType(context, cls, className, (JPClass*) superClass, interfaces, modifiers, _char));
			}
			if (className == "java.lang.Short")
			{
				context->m_ShortValueID = frame.GetMethodID(cls, "shortValue", "()S");
				return (jlong) (context->_java_lang_Short
						= new JPBoxedType(context, cls, className, (JPClass*) superClass, interfaces, modifiers, _short));
			}
			if (className == "java.lang.Integer")
			{
				context->m_IntValueID = frame.GetMethodID(cls, "intValue", "()I");
				return (jlong) (context->_java_lang_Integer
						= new JPBoxedType(context, cls, className, (JPClass*) superClass, interfaces, modifiers, _int));
			}
			if (className == "java.lang.Long")
			{
				context->m_LongValueID = frame.GetMethodID(cls, "longValue", "()J");
				return (jlong) (context->_java_lang_Long
						= new JPBoxedType(context, cls, className, (JPClass*) superClass, interfaces, modifiers, _long));
			}
			if (className == "java.lang.Float")
			{
				context->m_FloatValueID = frame.GetMethodID(cls, "floatValue", "()F");
				return (jlong) (context->_java_lang_Float
						= new JPBoxedType(context, cls, className, (JPClass*) superClass, interfaces, modifiers, _float));
			}
			if (className == "java.lang.Double")
			{
				context->m_DoubleValueID = frame.GetMethodID(cls, "doubleValue", "()D");
				return (jlong) (context->_java_lang_Double
						= new JPBoxedType(context, cls, className, (JPClass*) superClass, interfaces, modifiers, _double));
			}
			if (className == "org.jpype.proxy.JPypeProxy")
				return (jlong)
				new JPProxyType(context, cls, className, (JPClass*) superClass, interfaces, modifiers);

			// Register reflection types for later use
			if (className == "java.lang.reflect.Method")
				return (jlong) (context->_java_lang_reflect_Method = new JPClass(context, cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.reflect.Field")
				return (jlong) (context->_java_lang_reflect_Field = new JPClass(context, cls, className, (JPClass*) superClass, interfaces, modifiers));

			stringstream ss;
			ss << "Special class not defined for " << className;
			JP_RAISE_RUNTIME_ERROR(ss.str());
		} else
			// Otherwise create a normal class
			result = new JPClass(context, cls, className, (JPClass*) superClass, interfaces, modifiers);
		return (jlong) result;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT_C;
}

JNIEXPORT jlong JNICALL JPTypeFactory_definePrimitive(
		JNIEnv *env, jobject self, jlong contextPtr,
		jstring name,
		jclass cls,
		jlong boxedPtr,
		jint modifiers)
{
	JP_TRACE_IN_C("JPTypeFactory_definePrimitive");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	try
	{
		string cname = context->toStringUTF8(name);
		JP_TRACE(cname);
		if (cname == "void")
			return (jlong) (context->_void = _void);
		if (cname == "byte")
			return (jlong) (context->_byte = _byte);
		if (cname == "boolean")
			return (jlong) (context->_boolean = _boolean);
		if (cname == "char")
			return (jlong) (context->_char = _char);
		if (cname == "short")
			return (jlong) (context->_short = _short);
		if (cname == "int")
			return (jlong) (context->_int = _int);
		if (cname == "long")
			return (jlong) (context->_long = _long);
		if (cname == "float")
			return (jlong) (context->_float = _float);
		if (cname == "double")
			return (jlong) (context->_double = _double);
		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT_C;
}

JNIEXPORT void JNICALL JPTypeFactory_assignMembers(JNIEnv *env, jobject self,
		jlong contextPtr,
		jlong clsPtr,
		jlong ctorMethod,
		jlongArray methodPtrs,
		jlongArray fieldPtrs)
{
	JP_TRACE_IN_C("JPTypeFactory_assignMembers");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	try
	{
		JPClass* cls = (JPClass*) clsPtr;
		JPMethodDispatchList methodList;
		convert(frame, methodPtrs, methodList);

		JPFieldList fieldList;
		convert(frame, fieldPtrs, fieldList);
		cls->assignMembers(
				(JPMethodDispatch*) ctorMethod,
				methodList,
				fieldList);
		return;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return;
	JP_TRACE_OUT_C;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineField(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong cls,
		jstring name,
		jobject field,
		jlong fieldType,
		jint modifiers)
{
	JP_TRACE_IN_C("JPTypeFactory_defineField");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	try
	{
		string cname = context->toStringUTF8(name);
		JP_TRACE("class", cls);
		JP_TRACE(cname);
		jfieldID fid = frame.FromReflectedField(field);
		return (jlong) (new JPField(
				(JPClass*) cls,
				cname,
				field, fid,
				(JPClass*) fieldType,
				modifiers));
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT_C;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineMethod(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong cls, jstring name,
		jobject method,
		jlongArray overloadList, jint modifiers)
{
	JP_TRACE_IN_C("JPTypeFactory_defineMethod");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	try
	{
		jmethodID mid = frame.FromReflectedMethod(method);
		JPMethodList cover;
		convert(frame, overloadList, cover);
		string cname = context->toStringUTF8(name);
		JP_TRACE(cname);
		return (jlong) (new JPMethod(
				(JPClass*) cls,
				cname,
				method, mid,
				cover,
				modifiers));
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT_C;
}

JNIEXPORT jlong JNICALL JPTypeFactory_populateMethod(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong method,
		jlong returnType,
		jlongArray argumentTypes
		)
{
	JP_TRACE_IN_C("JPTypeFactory_populateMethod");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	try
	{
		JPClassList cargs;
		convert(frame, argumentTypes, cargs);
		JPMethod *methodPtr = (JPMethod*) method;
		methodPtr->setParameters((JPClass*) returnType, cargs);
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT_C;
}

JPTypeFactory::~JPTypeFactory()
{
}

JPTypeFactory::JPTypeFactory(JPContext* context)
{
	JPJavaFrame frame(context, 32);
	JP_TRACE_IN("JPTypeFactory::init");

	jclass cls = context->getClassLoader()->findClass("org.jpype.manager.TypeFactoryNative");

	JNINativeMethod method[9];

	method[0].name = (char*) "destroy";
	method[0].signature = (char*) "(J[JI)V";
	method[0].fnPtr = (void*) &JPTypeFactory_destroy;

	method[1].name = (char*) "defineMethodDispatch";
	method[1].signature = (char*) "(JJLjava/lang/String;[JI)J";
	method[1].fnPtr = (void*) &JPTypeFactory_defineMethodDispatch;

	method[2].name = (char*) "defineArrayClass";
	method[2].signature = (char*) "(JLjava/lang/Class;Ljava/lang/String;JJI)J";
	method[2].fnPtr = (void*) &JPTypeFactory_defineArrayClass;

	method[3].name = (char*) "defineObjectClass";
	method[3].signature = (char*) "(JLjava/lang/Class;Ljava/lang/String;J[JI)J";
	method[3].fnPtr = (void*) &JPTypeFactory_defineObjectClass;

	method[4].name = (char*) "definePrimitive";
	method[4].signature = (char*) "(JLjava/lang/String;Ljava/lang/Class;JI)J";
	method[4].fnPtr = (void*) &JPTypeFactory_definePrimitive;

	method[5].name = (char*) "assignMembers";
	method[5].signature = (char*) "(JJJ[J[J)V";
	method[5].fnPtr = (void*) &JPTypeFactory_assignMembers;

	method[6].name = (char*) "defineField";
	method[6].signature = (char*) "(JJLjava/lang/String;Ljava/lang/reflect/Field;JI)J";
	method[6].fnPtr = (void*) &JPTypeFactory_defineField;

	method[7].name = (char*) "defineMethod";
	method[7].signature = (char*) "(JJLjava/lang/String;Ljava/lang/reflect/Executable;[JI)J";
	method[7].fnPtr = (void*) &JPTypeFactory_defineMethod;

	method[8].name = (char*) "populateMethod";
	method[8].signature = (char*) "(JJJ[J)V";
	method[8].fnPtr = (void*) &JPTypeFactory_populateMethod;

	frame.GetMethodID(cls, "<init>", "()V");
	frame.RegisterNatives(cls, method, 9);
	JP_TRACE_OUT;
}
