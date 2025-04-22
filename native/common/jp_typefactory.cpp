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
#include "jpype.h"
#include "pyjp.h"
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
#include "jp_functional.h"
#include "jp_proxy.h"

void JPTypeFactory_rethrow(JPJavaFrame& frame)
{
	try
	{
		throw;
	} catch (JPypeException& ex)
	{
		ex.toJava();
	} catch (...)  // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		frame.ThrowNew(frame.getContext()->m_RuntimeException.get(),
				"unknown error occurred");
		// GCOVR_EXCL_STOP
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

extern "C"
{

JNIEXPORT void JNICALL Java_org_jpype_manager_TypeFactoryNative_newWrapper(
		JNIEnv *env, jobject self, jlong contextPtr, jlong jcls)
{
	JPJavaFrame frame = JPJavaFrame::external(env);
	JP_JAVA_TRY("JPTypeFactory_newWrapper");
	JPPyCallAcquire callback;
	auto* cls = (JPClass*) jcls;
	PyJPClass_hook(frame, cls);
	JP_JAVA_CATCH();  // GCOVR_EXCL_LINE
}

JNIEXPORT void JNICALL Java_org_jpype_manager_TypeFactoryNative_destroy(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlongArray resources,
		jint sz)
{
	JPJavaFrame frame = JPJavaFrame::external(env);
	auto* context = frame.getContext();
	JP_JAVA_TRY("JPTypeFactory_destroy");
	JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, resources,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);
	jlong* values = accessor.get();
	for (int i = 0; i < sz; ++i)
	{
		context->m_Resources.push_back((JPResource*) values[i]);
	}
	return;
	JP_JAVA_CATCH();  // GCOVR_EXCL_LINE
}

JNIEXPORT jlong JNICALL Java_org_jpype_manager_TypeFactoryNative_defineMethodDispatch(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong clsPtr,
		jstring name,
		jlongArray overloadPtrs,
		jint modifiers)
{
	JPJavaFrame frame = JPJavaFrame::external(env);
	JP_JAVA_TRY("JPTypeFactory_defineMethodDispatch");
	auto* cls = (JPClass*) clsPtr;
	JPMethodList overloadList;
	convert(frame, overloadPtrs, overloadList);
	string cname = frame.toStringUTF8(name);
	JP_TRACE(cname);
	auto* dispatch = new JPMethodDispatch(cls, cname, overloadList, modifiers);
	return (jlong) dispatch;
	JP_JAVA_CATCH(0);  // GCOVR_EXCL_LINE
}

JNIEXPORT jlong JNICALL Java_org_jpype_manager_TypeFactoryNative_defineArrayClass(
		JNIEnv *env, jobject self, jlong contextPtr,
		jclass cls,
		jstring name,
		jlong superClass,
		jlong componentClass,
		jint modifiers)
{
	JPJavaFrame frame = JPJavaFrame::external(env);
	JP_JAVA_TRY("JPTypeFactory_defineArrayClass");
	string cname = frame.toStringUTF8(name);
	JP_TRACE(cname);
	auto* result = new JPArrayClass(frame, cls,
			cname,
			(JPClass*) superClass,
			(JPClass*) componentClass,
			modifiers);
	return (jlong) result;
	JP_JAVA_CATCH(0);  // GCOVR_EXCL_LINE
}

JNIEXPORT jlong JNICALL Java_org_jpype_manager_TypeFactoryNative_defineObjectClass(
		JNIEnv *env, jobject self, jlong contextPtr,
		jclass cls,
		jstring name,
		jlong superClass,
		jlongArray interfacePtrs,
		jint modifiers)
{
	// All resources are created here are owned by Java and deleted by Java shutdown routine
	JPJavaFrame frame = JPJavaFrame::external(env);
	auto* context = frame.getContext();
	JP_JAVA_TRY("JPTypeFactory_defineObjectClass");
	string className = frame.toStringUTF8(name);
	JP_TRACE(className);
	JPClassList interfaces;
	if (interfacePtrs != nullptr)
		convert(frame, interfacePtrs, interfaces);
	JPClass* result = nullptr;
	if (!JPModifier::isSpecial(modifiers))
	{
		// Create a normal class
		return (jlong) new JPClass(frame, cls, className, (JPClass*) superClass, interfaces, modifiers);
	}

	if (JPModifier::isFunctional(modifiers))
		return (jlong) new JPFunctional(frame, cls, className, (JPClass*) superClass, interfaces, modifiers);
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
			= new JPClassType(frame, cls, className,
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
		return (jlong) (context->_java_lang_Boolean
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_boolean));
	}
	if (className == "java.lang.Byte")
	{
		context->_byte = new JPByteType();
		return (jlong) (context->_java_lang_Byte
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_byte));
	}
	if (className == "java.lang.Character")
	{
		context->_char = new JPCharType();
		return (jlong) (context->_java_lang_Character
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_char));
	}
	if (className == "java.lang.Short")
	{
		context->_short = new JPShortType();
		return (jlong) (context->_java_lang_Short
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_short));
	}
	if (className == "java.lang.Integer")
	{
		context->_int = new JPIntType();
		return (jlong) (context->_java_lang_Integer
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_int));
	}
	if (className == "java.lang.Long")
	{
		context->_long = new JPLongType();
		return (jlong) (context->_java_lang_Long
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_long));
	}
	if (className == "java.lang.Float")
	{
		context->_float = new JPFloatType();
		return (jlong) (context->_java_lang_Float
				= new JPBoxedType(frame, cls, className,
				(JPClass*) superClass, interfaces, modifiers, context->_float));
	}
	if (className == "java.lang.Double")
	{
		context->_double = new JPDoubleType();
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

	std::stringstream ss;
	ss << "Special class not defined for " << className;
	JP_RAISE(PyExc_RuntimeError, ss.str());
	return (jlong) result;
	JP_JAVA_CATCH(0);  // GCOVR_EXCL_LINE
}

JNIEXPORT jlong JNICALL Java_org_jpype_manager_TypeFactoryNative_definePrimitive(
		JNIEnv *env, jobject self, jlong contextPtr,
		jstring name,
		jclass cls,
		jlong boxedPtr,
		jint modifiers)
{
	// These resources are created by the boxed types
	JPJavaFrame frame = JPJavaFrame::external(env);
	auto* context = frame.getContext();
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
	JP_JAVA_CATCH(0);  // GCOVR_EXCL_LINE
}

JNIEXPORT void JNICALL Java_org_jpype_manager_TypeFactoryNative_assignMembers(
		JNIEnv *env, jobject self,
		jlong contextPtr,
		jlong clsPtr,
		jlong ctorMethod,
		jlongArray methodPtrs,
		jlongArray fieldPtrs)
{
	JPJavaFrame frame = JPJavaFrame::external(env);
	JP_JAVA_TRY("JPTypeFactory_assignMembers");
	auto* cls = (JPClass*) clsPtr;
	JPMethodDispatchList methodList;
	convert(frame, methodPtrs, methodList);

	JPFieldList fieldList;
	convert(frame, fieldPtrs, fieldList);
	cls->assignMembers(
			(JPMethodDispatch*) ctorMethod,
			methodList,
			fieldList);
	return;
	JP_JAVA_CATCH();  // GCOVR_EXCL_LINE
}

JNIEXPORT jlong JNICALL Java_org_jpype_manager_TypeFactoryNative_defineField(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong cls,
		jstring name,
		jobject field,
		jlong fieldType,
		jint modifiers)
{
	JPJavaFrame frame = JPJavaFrame::external(env);
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
	JP_JAVA_CATCH(0);  // GCOVR_EXCL_LINE
}

JNIEXPORT jlong JNICALL Java_org_jpype_manager_TypeFactoryNative_defineMethod(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong cls, jstring name,
		jobject method,
		jlongArray overloadList, jint modifiers)
{
	JPJavaFrame frame = JPJavaFrame::external(env);
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

JNIEXPORT void JNICALL Java_org_jpype_manager_TypeFactoryNative_populateMethod(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong method,
		jlong returnType,
		jlongArray argumentTypes
		)
{
	JPJavaFrame frame = JPJavaFrame::external(env);
	JP_JAVA_TRY("JPTypeFactory_populateMethod");
	JPClassList cargs;
	convert(frame, argumentTypes, cargs);
	auto *methodPtr = (JPMethod*) method;
	methodPtr->setParameters((JPClass*) returnType, std::move(cargs));
	JP_JAVA_CATCH();  // GCOVR_EXCL_LINE
}

} // extern "C"

