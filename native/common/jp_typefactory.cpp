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
#include "jpype.h"
#include "jp_primitive_accessor.h"
#include "jp_classloader.h"
#include "jp_boxedtype.h"
#include "jp_field.h"
#include "jp_objecttype.h"
#include "jp_numbertype.h"
#include "jp_classtype.h"
#include "jp_method.h"
#include "jp_methoddispatch.h"
#include "jp_buffertype.h"
#include "jp_typemanager.h"
#include "jp_arrayclass.h"
#include "jp_stringtype.h"
#include "jp_voidtype.h"
#include "jp_booleantype.h"
#include "jp_bytetype.h"
#include "jp_chartype.h"
#include "jp_shorttype.h"
#include "jp_inttype.h"
#include "jp_longtype.h"
#include "jp_floattype.h"
#include "jp_doubletype.h"

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
		frame.ThrowNew(frame.getContext()->m_RuntimeException.get(),
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

#ifdef JP_TRACING_ENABLE
#define JP_JAVA_TRY(...) \
  JPypeTracer _trace(__VA_ARGS__); \
  try {
#define JP_JAVA_CATCH(...) \
  } \
  catch(...) { \
  _trace.gotError(JP_STACKINFO()); \
  JPTypeFactory_rethrow(frame); } return __VA_ARGS__
#else
#define JP_JAVA_TRY(...)  try {
#define JP_JAVA_CATCH(...)  } catch(...) { JPTypeFactory_rethrow(frame); } return __VA_ARGS__
#endif

JNIEXPORT void JNICALL JPTypeFactory_destroy(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlongArray resources,
		jint sz)
{
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	JP_JAVA_TRY("JPTypeFactory_destroy");
	JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, resources,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);
	jlong* values = accessor.get();
	for (int i = 0; i < sz; ++i)
	{
		delete (JPResource*) values[i];
	}
	return;
	JP_JAVA_CATCH();
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineMethodDispatch(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong clsPtr,
		jstring name,
		jlongArray overloadPtrs,
		jint modifiers)
{
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	JP_JAVA_TRY("JPTypeFactory_defineMethodDispatch");
	JPClass* cls = (JPClass*) clsPtr;
	JPMethodList overloadList;
	convert(frame, overloadPtrs, overloadList);
	string cname = frame.toStringUTF8(name);
	JP_TRACE(cname);
	JPMethodDispatch* dispatch = new JPMethodDispatch(cls, cname, overloadList, modifiers);
	return (jlong) dispatch;
	JP_JAVA_CATCH(0);
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineArrayClass(
		JNIEnv *env, jobject self, jlong contextPtr,
		jclass cls,
		jstring name,
		jlong superClass,
		jlong componentClass,
		jint modifiers)
{
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	JP_JAVA_TRY("JPTypeFactory_defineArrayClass");
	string cname = frame.toStringUTF8(name);
	JP_TRACE(cname);
	JPArrayClass* result = new JPArrayClass(frame, cls,
			cname,
			(JPClass*) superClass,
			(JPClass*) componentClass,
			modifiers);
	return (jlong) result;
	JP_JAVA_CATCH(0);
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineObjectClass(
		JNIEnv *env, jobject self, jlong contextPtr,
		jclass cls,
		jstring name,
		jlong superClass,
		jlongArray interfacePtrs,
		jint modifiers)
{
	// All resources are created here are owned by Java and deleted by Java shutdown routine
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	JP_JAVA_TRY("JPTypeFactory_defineObjectClass");
	string className = frame.toStringUTF8(name);
	JP_TRACE(className);
	JPClassList interfaces;
	if (interfacePtrs != NULL)
		convert(frame, interfacePtrs, interfaces);
	JPClass* result = NULL;
	if (!JPModifier::isSpecial(modifiers))
	{
		// Create a normal class
		return (jlong) new JPClass(frame, cls, className, (JPClass*) superClass, interfaces, modifiers);
	}
	if (JPModifier::isBuffer(modifiers))
		return (jlong) new JPBufferType(frame, cls, className, (JPClass*) superClass, interfaces, modifiers);
	// Certain classes require special implementations
	if (className == "java.lang.Object")
		return (jlong) (context->_java_lang_Object
			= new JPObjectType(frame, cls, className, (JPClass*) superClass, interfaces, modifiers));
	if (className == "java.lang.Class")
		return (jlong) (context->_java_lang_Class
			= new JPClassType(frame, cls, className, (JPClass*) superClass, interfaces, modifiers));
	if (className == "java.lang.CharSequence")
		return (jlong) (new JPStringType(frame, cls, className,
			(JPClass*) superClass, interfaces, modifiers));
	if (className == "java.lang.String")
		return (jlong) (context->_java_lang_String
			= new JPStringType(frame, cls, className,
			(JPClass*) superClass, interfaces, modifiers));
	if (className == "java.lang.Throwable")
		return (jlong) (context->_java_lang_Throwable
			= new JPObjectType(frame, cls, className,
			(JPClass*) superClass, interfaces, modifiers));

	if (className == "java.lang.Number")
		return (jlong) new JPNumberType(frame, cls, className, (JPClass*) superClass, interfaces, modifiers);


	// Register the box types
	if (className == "java.lang.Void")
	{
		context->_void = new JPVoidType();
		return (jlong) (context->_java_lang_Void
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_void));
	}
	if (className == "java.lang.Boolean")
	{
		context->_boolean = new JPBooleanType();
		context->m_BooleanValueID = frame.GetMethodID(cls, "booleanValue", "()Z");
		return (jlong) (context->_java_lang_Boolean
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_boolean));
	}
	if (className == "java.lang.Byte")
	{
		context->_byte = new JPByteType();
		context->m_ByteValueID = frame.GetMethodID(cls, "byteValue", "()B");
		return (jlong) (context->_java_lang_Byte
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_byte));
	}
	if (className == "java.lang.Character")
	{
		context->_char = new JPCharType();
		context->m_CharValueID = frame.GetMethodID(cls, "charValue", "()C");
		return (jlong) (context->_java_lang_Character
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_char));
	}
	if (className == "java.lang.Short")
	{
		context->_short = new JPShortType();
		context->m_ShortValueID = frame.GetMethodID(cls, "shortValue", "()S");
		return (jlong) (context->_java_lang_Short
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_short));
	}
	if (className == "java.lang.Integer")
	{
		context->_int = new JPIntType();
		context->m_IntValueID = frame.GetMethodID(cls, "intValue", "()I");
		return (jlong) (context->_java_lang_Integer
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_int));
	}
	if (className == "java.lang.Long")
	{
		context->_long = new JPLongType();

		context->m_LongValueID = frame.GetMethodID(cls, "longValue", "()J");
		return (jlong) (context->_java_lang_Long
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_long));
	}
	if (className == "java.lang.Float")
	{
		context->_float = new JPFloatType();
		context->m_FloatValueID = frame.GetMethodID(cls, "floatValue", "()F");
		return (jlong) (context->_java_lang_Float
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_float));
	}
	if (className == "java.lang.Double")
	{
		context->_double = new JPDoubleType();
		context->m_DoubleValueID = frame.GetMethodID(cls, "doubleValue", "()D");
		return (jlong) (context->_java_lang_Double
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_double));
	}
	if (className == "org.jpype.proxy.JPypeProxy")
		return (jlong)
		new JPProxyType(frame, cls, className,
			(JPClass*) superClass, interfaces, modifiers);

	// Register reflection types for later use
	if (className == "java.lang.reflect.Method")
		return (jlong) (context->_java_lang_reflect_Method
			= new JPClass(frame, cls, className, (JPClass*) superClass, interfaces, modifiers));

	if (className == "java.lang.reflect.Field")
		return (jlong) (context->_java_lang_reflect_Field
			= new JPClass(frame, cls, className, (JPClass*) superClass, interfaces, modifiers));

	stringstream ss;
	ss << "Special class not defined for " << className;
	JP_RAISE(PyExc_RuntimeError, ss.str());
	return (jlong) result;
	JP_JAVA_CATCH(0);
}

JNIEXPORT jlong JNICALL JPTypeFactory_definePrimitive(
		JNIEnv *env, jobject self, jlong contextPtr,
		jstring name,
		jclass cls,
		jlong boxedPtr,
		jint modifiers)
{
	// These resources are created by the boxed types
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	JP_JAVA_TRY("JPTypeFactory_definePrimitive");
	string cname = frame.toStringUTF8(name);
	JP_TRACE(cname);
	if (cname == "void")
	{
		context->_void->setClass(frame, cls);
		return (jlong) (context->_void);
	}
	if (cname == "byte")
	{
		context->_byte->setClass(frame, cls);
		return (jlong) (context->_byte);
	}
	if (cname == "boolean")
	{
		context->_boolean->setClass(frame, cls);
		return (jlong) (context->_boolean);
	}
	if (cname == "char")
	{
		context->_char->setClass(frame, cls);
		return (jlong) (context->_char);
	}
	if (cname == "short")
	{
		context->_short->setClass(frame, cls);
		return (jlong) (context->_short);
	}
	if (cname == "int")
	{
		context->_int->setClass(frame, cls);
		return (jlong) (context->_int);
	}
	if (cname == "long")
	{
		context->_long->setClass(frame, cls);
		return (jlong) (context->_long);
	}
	if (cname == "float")
	{
		context->_float->setClass(frame, cls);
		return (jlong) (context->_float);
	}
	if (cname == "double")
	{
		context->_double->setClass(frame, cls);
		return (jlong) (context->_double);
	}
	return 0;
	JP_JAVA_CATCH(0);
}

JNIEXPORT void JNICALL JPTypeFactory_assignMembers(JNIEnv *env, jobject self,
		jlong contextPtr,
		jlong clsPtr,
		jlong ctorMethod,
		jlongArray methodPtrs,
		jlongArray fieldPtrs)
{
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	JP_JAVA_TRY("JPTypeFactory_assignMembers");
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
	JP_JAVA_CATCH();
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineField(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong cls,
		jstring name,
		jobject field,
		jlong fieldType,
		jint modifiers)
{
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	JP_JAVA_TRY("JPTypeFactory_defineField");
	string cname = frame.toStringUTF8(name);
	JP_TRACE("class", cls);
	JP_TRACE(cname);
	jfieldID fid = frame.FromReflectedField(field);
	return (jlong) (new JPField(
			frame,
			(JPClass*) cls,
			cname,
			field, fid,
			(JPClass*) fieldType,
			modifiers));
	JP_JAVA_CATCH(0);
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineMethod(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong cls, jstring name,
		jobject method,
		jlongArray overloadList, jint modifiers)
{
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	JP_JAVA_TRY("JPTypeFactory_defineMethod");
	jmethodID mid = frame.FromReflectedMethod(method);
	JPMethodList cover;
	convert(frame, overloadList, cover);
	string cname = frame.toStringUTF8(name);
	JP_TRACE(cname);
	return (jlong) (new JPMethod(
			frame,
			(JPClass*) cls,
			cname,
			method, mid,
			cover,
			modifiers));
	JP_JAVA_CATCH(0);
}

JNIEXPORT jlong JNICALL JPTypeFactory_populateMethod(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong method,
		jlong returnType,
		jlongArray argumentTypes
		)
{
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);
	JP_JAVA_TRY("JPTypeFactory_populateMethod");
	JPClassList cargs;
	convert(frame, argumentTypes, cargs);
	JPMethod *methodPtr = (JPMethod*) method;
	methodPtr->setParameters((JPClass*) returnType, cargs);
	JP_JAVA_CATCH(0);
}

JPTypeFactory::~JPTypeFactory()
{
}

JPTypeFactory::JPTypeFactory(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPTypeFactory::init");
	JPContext* context = frame.getContext();
	jclass cls = context->getClassLoader()->findClass(frame, "org.jpype.manager.TypeFactoryNative");

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
