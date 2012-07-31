
/*****************************************************************************
   Copyright 2004-2008 Steve Menard
   
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

// This code has been automatically generated ... No not edit

#include <jpype.h>
#define JAVA_CHECK(msg) \
if (JPEnv::getJava()->ExceptionCheck()) \
{ \
    RAISE(JavaException, msg); \
} \



jbyte JPJavaEnv::GetStaticByteField(jclass clazz, jfieldID fid) 
{
    JNIEnv* env = getJNIEnv(); 
    jbyte res = env->functions->GetStaticByteField(env, clazz, fid);
    JAVA_CHECK("GetStaticByteField");
    return res;
}

jbyte JPJavaEnv::GetByteField(jobject clazz, jfieldID fid)
{
    JNIEnv* env = getJNIEnv(); 
    jbyte res = env->functions->GetByteField(env, clazz, fid);
    JAVA_CHECK("GetByteField");
    return res;
}
void JPJavaEnv::SetStaticByteField(jclass clazz, jfieldID fid, jbyte val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetStaticByteField(env, clazz, fid, val);
    JAVA_CHECK("SetStaticByteField");
}

void JPJavaEnv::SetByteField(jobject clazz, jfieldID fid, jbyte val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetByteField(env, clazz, fid, val);
    JAVA_CHECK("SetByteField"); 
}

jbyte JPJavaEnv::CallStaticByteMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
        jbyte res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticByteMethodA(env, clazz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Byte");
    return res;
 
}

jbyte JPJavaEnv::CallStaticByteMethod(jclass clazz, jmethodID mid)
{
        jbyte res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticByteMethod(env, clazz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Byte");
    return res;
 
}

jbyte JPJavaEnv::CallByteMethodA(jobject obj, jmethodID mid, jvalue* val)
{
        jbyte res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallByteMethodA(env, obj, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Byte");
    return res;
 
}

jbyte JPJavaEnv::CallByteMethod(jobject obj, jmethodID mid)
{
        jbyte res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallByteMethod(env, obj, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Byte");
    return res;
 
}

jbyte JPJavaEnv::CallNonvirtualByteMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
        jbyte res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualByteMethodA(env, obj, claz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Byte");
    return res;
 
}

jbyte JPJavaEnv::CallNonvirtualByteMethod(jobject obj, jclass claz, jmethodID mid)
{
        jbyte res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualByteMethod(env, obj, claz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Byte");
    return res;
 
}

jshort JPJavaEnv::GetStaticShortField(jclass clazz, jfieldID fid) 
{
    JNIEnv* env = getJNIEnv(); 
    jshort res = env->functions->GetStaticShortField(env, clazz, fid);
    JAVA_CHECK("GetStaticShortField");
    return res;
}

jshort JPJavaEnv::GetShortField(jobject clazz, jfieldID fid)
{
    JNIEnv* env = getJNIEnv(); 
    jshort res = env->functions->GetShortField(env, clazz, fid);
    JAVA_CHECK("GetShortField");
    return res;
}
void JPJavaEnv::SetStaticShortField(jclass clazz, jfieldID fid, jshort val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetStaticShortField(env, clazz, fid, val);
    JAVA_CHECK("SetStaticShortField");
}

void JPJavaEnv::SetShortField(jobject clazz, jfieldID fid, jshort val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetShortField(env, clazz, fid, val);
    JAVA_CHECK("SetShortField"); 
}

jshort JPJavaEnv::CallStaticShortMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
        jshort res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticShortMethodA(env, clazz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Short");
    return res;
 
}

jshort JPJavaEnv::CallStaticShortMethod(jclass clazz, jmethodID mid)
{
        jshort res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticShortMethod(env, clazz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Short");
    return res;
 
}

jshort JPJavaEnv::CallShortMethodA(jobject obj, jmethodID mid, jvalue* val)
{
        jshort res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallShortMethodA(env, obj, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Short");
    return res;
 
}

jshort JPJavaEnv::CallShortMethod(jobject obj, jmethodID mid)
{
        jshort res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallShortMethod(env, obj, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Short");
    return res;
 
}

jshort JPJavaEnv::CallNonvirtualShortMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
        jshort res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualShortMethodA(env, obj, claz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Short");
    return res;
 
}

jshort JPJavaEnv::CallNonvirtualShortMethod(jobject obj, jclass claz, jmethodID mid)
{
        jshort res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualShortMethod(env, obj, claz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Short");
    return res;
 
}

jint JPJavaEnv::GetStaticIntField(jclass clazz, jfieldID fid) 
{
    JNIEnv* env = getJNIEnv(); 
    jint res = env->functions->GetStaticIntField(env, clazz, fid);
    JAVA_CHECK("GetStaticIntField");
    return res;
}

jint JPJavaEnv::GetIntField(jobject clazz, jfieldID fid)
{
    JNIEnv* env = getJNIEnv(); 
    jint res = env->functions->GetIntField(env, clazz, fid);
    JAVA_CHECK("GetIntField");
    return res;
}
void JPJavaEnv::SetStaticIntField(jclass clazz, jfieldID fid, jint val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetStaticIntField(env, clazz, fid, val);
    JAVA_CHECK("SetStaticIntField");
}

void JPJavaEnv::SetIntField(jobject clazz, jfieldID fid, jint val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetIntField(env, clazz, fid, val);
    JAVA_CHECK("SetIntField"); 
}

jint JPJavaEnv::CallStaticIntMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
        jint res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticIntMethodA(env, clazz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Int");
    return res;
 
}

jint JPJavaEnv::CallStaticIntMethod(jclass clazz, jmethodID mid)
{
        jint res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticIntMethod(env, clazz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Int");
    return res;
 
}

jint JPJavaEnv::CallIntMethodA(jobject obj, jmethodID mid, jvalue* val)
{
        jint res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallIntMethodA(env, obj, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Int");
    return res;
 
}

jint JPJavaEnv::CallIntMethod(jobject obj, jmethodID mid)
{
        jint res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallIntMethod(env, obj, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Int");
    return res;
 
}

jint JPJavaEnv::CallNonvirtualIntMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
        jint res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualIntMethodA(env, obj, claz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Int");
    return res;
 
}

jint JPJavaEnv::CallNonvirtualIntMethod(jobject obj, jclass claz, jmethodID mid)
{
        jint res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualIntMethod(env, obj, claz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Int");
    return res;
 
}

jlong JPJavaEnv::GetStaticLongField(jclass clazz, jfieldID fid) 
{
    JNIEnv* env = getJNIEnv(); 
    jlong res = env->functions->GetStaticLongField(env, clazz, fid);
    JAVA_CHECK("GetStaticLongField");
    return res;
}

jlong JPJavaEnv::GetLongField(jobject clazz, jfieldID fid)
{
    JNIEnv* env = getJNIEnv(); 
    jlong res = env->functions->GetLongField(env, clazz, fid);
    JAVA_CHECK("GetLongField");
    return res;
}
void JPJavaEnv::SetStaticLongField(jclass clazz, jfieldID fid, jlong val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetStaticLongField(env, clazz, fid, val);
    JAVA_CHECK("SetStaticLongField");
}

void JPJavaEnv::SetLongField(jobject clazz, jfieldID fid, jlong val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetLongField(env, clazz, fid, val);
    JAVA_CHECK("SetLongField"); 
}

jlong JPJavaEnv::CallStaticLongMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
        jlong res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticLongMethodA(env, clazz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Long");
    return res;
 
}

jlong JPJavaEnv::CallStaticLongMethod(jclass clazz, jmethodID mid)
{
        jlong res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticLongMethod(env, clazz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Long");
    return res;
 
}

jlong JPJavaEnv::CallLongMethodA(jobject obj, jmethodID mid, jvalue* val)
{
        jlong res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallLongMethodA(env, obj, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Long");
    return res;
 
}

jlong JPJavaEnv::CallLongMethod(jobject obj, jmethodID mid)
{
        jlong res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallLongMethod(env, obj, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Long");
    return res;
 
}

jlong JPJavaEnv::CallNonvirtualLongMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
        jlong res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualLongMethodA(env, obj, claz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Long");
    return res;
 
}

jlong JPJavaEnv::CallNonvirtualLongMethod(jobject obj, jclass claz, jmethodID mid)
{
        jlong res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualLongMethod(env, obj, claz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Long");
    return res;
 
}

jfloat JPJavaEnv::GetStaticFloatField(jclass clazz, jfieldID fid) 
{
    JNIEnv* env = getJNIEnv(); 
    jfloat res = env->functions->GetStaticFloatField(env, clazz, fid);
    JAVA_CHECK("GetStaticFloatField");
    return res;
}

jfloat JPJavaEnv::GetFloatField(jobject clazz, jfieldID fid)
{
    JNIEnv* env = getJNIEnv(); 
    jfloat res = env->functions->GetFloatField(env, clazz, fid);
    JAVA_CHECK("GetFloatField");
    return res;
}
void JPJavaEnv::SetStaticFloatField(jclass clazz, jfieldID fid, jfloat val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetStaticFloatField(env, clazz, fid, val);
    JAVA_CHECK("SetStaticFloatField");
}

void JPJavaEnv::SetFloatField(jobject clazz, jfieldID fid, jfloat val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetFloatField(env, clazz, fid, val);
    JAVA_CHECK("SetFloatField"); 
}

jfloat JPJavaEnv::CallStaticFloatMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
        jfloat res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticFloatMethodA(env, clazz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Float");
    return res;
 
}

jfloat JPJavaEnv::CallStaticFloatMethod(jclass clazz, jmethodID mid)
{
        jfloat res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticFloatMethod(env, clazz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Float");
    return res;
 
}

jfloat JPJavaEnv::CallFloatMethodA(jobject obj, jmethodID mid, jvalue* val)
{
        jfloat res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallFloatMethodA(env, obj, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Float");
    return res;
 
}

jfloat JPJavaEnv::CallFloatMethod(jobject obj, jmethodID mid)
{
        jfloat res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallFloatMethod(env, obj, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Float");
    return res;
 
}

jfloat JPJavaEnv::CallNonvirtualFloatMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
        jfloat res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualFloatMethodA(env, obj, claz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Float");
    return res;
 
}

jfloat JPJavaEnv::CallNonvirtualFloatMethod(jobject obj, jclass claz, jmethodID mid)
{
        jfloat res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualFloatMethod(env, obj, claz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Float");
    return res;
 
}

jdouble JPJavaEnv::GetStaticDoubleField(jclass clazz, jfieldID fid) 
{
    JNIEnv* env = getJNIEnv(); 
    jdouble res = env->functions->GetStaticDoubleField(env, clazz, fid);
    JAVA_CHECK("GetStaticDoubleField");
    return res;
}

jdouble JPJavaEnv::GetDoubleField(jobject clazz, jfieldID fid)
{
    JNIEnv* env = getJNIEnv(); 
    jdouble res = env->functions->GetDoubleField(env, clazz, fid);
    JAVA_CHECK("GetDoubleField");
    return res;
}
void JPJavaEnv::SetStaticDoubleField(jclass clazz, jfieldID fid, jdouble val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetStaticDoubleField(env, clazz, fid, val);
    JAVA_CHECK("SetStaticDoubleField");
}

void JPJavaEnv::SetDoubleField(jobject clazz, jfieldID fid, jdouble val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetDoubleField(env, clazz, fid, val);
    JAVA_CHECK("SetDoubleField"); 
}

jdouble JPJavaEnv::CallStaticDoubleMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
        jdouble res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticDoubleMethodA(env, clazz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Double");
    return res;
 
}

jdouble JPJavaEnv::CallStaticDoubleMethod(jclass clazz, jmethodID mid)
{
        jdouble res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticDoubleMethod(env, clazz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Double");
    return res;
 
}

jdouble JPJavaEnv::CallDoubleMethodA(jobject obj, jmethodID mid, jvalue* val)
{
        jdouble res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallDoubleMethodA(env, obj, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Double");
    return res;
 
}

jdouble JPJavaEnv::CallDoubleMethod(jobject obj, jmethodID mid)
{
        jdouble res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallDoubleMethod(env, obj, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Double");
    return res;
 
}

jdouble JPJavaEnv::CallNonvirtualDoubleMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
        jdouble res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualDoubleMethodA(env, obj, claz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Double");
    return res;
 
}

jdouble JPJavaEnv::CallNonvirtualDoubleMethod(jobject obj, jclass claz, jmethodID mid)
{
        jdouble res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualDoubleMethod(env, obj, claz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Double");
    return res;
 
}

jchar JPJavaEnv::GetStaticCharField(jclass clazz, jfieldID fid) 
{
    JNIEnv* env = getJNIEnv(); 
    jchar res = env->functions->GetStaticCharField(env, clazz, fid);
    JAVA_CHECK("GetStaticCharField");
    return res;
}

jchar JPJavaEnv::GetCharField(jobject clazz, jfieldID fid)
{
    JNIEnv* env = getJNIEnv(); 
    jchar res = env->functions->GetCharField(env, clazz, fid);
    JAVA_CHECK("GetCharField");
    return res;
}
void JPJavaEnv::SetStaticCharField(jclass clazz, jfieldID fid, jchar val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetStaticCharField(env, clazz, fid, val);
    JAVA_CHECK("SetStaticCharField");
}

void JPJavaEnv::SetCharField(jobject clazz, jfieldID fid, jchar val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetCharField(env, clazz, fid, val);
    JAVA_CHECK("SetCharField"); 
}

jchar JPJavaEnv::CallStaticCharMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
        jchar res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticCharMethodA(env, clazz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Char");
    return res;
 
}

jchar JPJavaEnv::CallStaticCharMethod(jclass clazz, jmethodID mid)
{
        jchar res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticCharMethod(env, clazz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Char");
    return res;
 
}

jchar JPJavaEnv::CallCharMethodA(jobject obj, jmethodID mid, jvalue* val)
{
        jchar res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallCharMethodA(env, obj, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Char");
    return res;
 
}

jchar JPJavaEnv::CallCharMethod(jobject obj, jmethodID mid)
{
        jchar res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallCharMethod(env, obj, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Char");
    return res;
 
}

jchar JPJavaEnv::CallNonvirtualCharMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
        jchar res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualCharMethodA(env, obj, claz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Char");
    return res;
 
}

jchar JPJavaEnv::CallNonvirtualCharMethod(jobject obj, jclass claz, jmethodID mid)
{
        jchar res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualCharMethod(env, obj, claz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Char");
    return res;
 
}

jboolean JPJavaEnv::GetStaticBooleanField(jclass clazz, jfieldID fid) 
{
    JNIEnv* env = getJNIEnv(); 
    jboolean res = env->functions->GetStaticBooleanField(env, clazz, fid);
    JAVA_CHECK("GetStaticBooleanField");
    return res;
}

jboolean JPJavaEnv::GetBooleanField(jobject clazz, jfieldID fid)
{
    JNIEnv* env = getJNIEnv(); 
    jboolean res = env->functions->GetBooleanField(env, clazz, fid);
    JAVA_CHECK("GetBooleanField");
    return res;
}
void JPJavaEnv::SetStaticBooleanField(jclass clazz, jfieldID fid, jboolean val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetStaticBooleanField(env, clazz, fid, val);
    JAVA_CHECK("SetStaticBooleanField");
}

void JPJavaEnv::SetBooleanField(jobject clazz, jfieldID fid, jboolean val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetBooleanField(env, clazz, fid, val);
    JAVA_CHECK("SetBooleanField"); 
}

jboolean JPJavaEnv::CallStaticBooleanMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
        jboolean res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticBooleanMethodA(env, clazz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Boolean");
    return res;
 
}

jboolean JPJavaEnv::CallStaticBooleanMethod(jclass clazz, jmethodID mid)
{
        jboolean res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticBooleanMethod(env, clazz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Boolean");
    return res;
 
}

jboolean JPJavaEnv::CallBooleanMethodA(jobject obj, jmethodID mid, jvalue* val)
{
        jboolean res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallBooleanMethodA(env, obj, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Boolean");
    return res;
 
}

jboolean JPJavaEnv::CallBooleanMethod(jobject obj, jmethodID mid)
{
        jboolean res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallBooleanMethod(env, obj, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Boolean");
    return res;
 
}

jboolean JPJavaEnv::CallNonvirtualBooleanMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
        jboolean res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualBooleanMethodA(env, obj, claz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Boolean");
    return res;
 
}

jboolean JPJavaEnv::CallNonvirtualBooleanMethod(jobject obj, jclass claz, jmethodID mid)
{
        jboolean res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualBooleanMethod(env, obj, claz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Boolean");
    return res;
 
}

jobject JPJavaEnv::GetStaticObjectField(jclass clazz, jfieldID fid) 
{
    JNIEnv* env = getJNIEnv(); 
    jobject res = env->functions->GetStaticObjectField(env, clazz, fid);
    JAVA_CHECK("GetStaticObjectField");
    return res;
}

jobject JPJavaEnv::GetObjectField(jobject clazz, jfieldID fid)
{
    JNIEnv* env = getJNIEnv(); 
    jobject res = env->functions->GetObjectField(env, clazz, fid);
    JAVA_CHECK("GetObjectField");
    return res;
}
void JPJavaEnv::SetStaticObjectField(jclass clazz, jfieldID fid, jobject val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetStaticObjectField(env, clazz, fid, val);
    JAVA_CHECK("SetStaticObjectField");
}

void JPJavaEnv::SetObjectField(jobject clazz, jfieldID fid, jobject val)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetObjectField(env, clazz, fid, val);
    JAVA_CHECK("SetObjectField"); 
}

jobject JPJavaEnv::CallStaticObjectMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
        jobject res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticObjectMethodA(env, clazz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Object");
    return res;
 
}

jobject JPJavaEnv::CallStaticObjectMethod(jclass clazz, jmethodID mid)
{
        jobject res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallStaticObjectMethod(env, clazz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Object");
    return res;
 
}

jobject JPJavaEnv::CallObjectMethodA(jobject obj, jmethodID mid, jvalue* val)
{
        jobject res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallObjectMethodA(env, obj, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Object");
    return res;
 
}

jobject JPJavaEnv::CallObjectMethod(jobject obj, jmethodID mid)
{
        jobject res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallObjectMethod(env, obj, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Object");
    return res;
 
}

jobject JPJavaEnv::CallNonvirtualObjectMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
        jobject res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualObjectMethodA(env, obj, claz, mid, val); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Object");
    return res;
 
}

jobject JPJavaEnv::CallNonvirtualObjectMethod(jobject obj, jclass claz, jmethodID mid)
{
        jobject res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();
 
    res = env->functions->CallNonvirtualObjectMethod(env, obj, claz, mid); 
    
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("Object");
    return res;
 
}

jbyteArray JPJavaEnv::NewByteArray(jint len)
{
    JNIEnv* env = getJNIEnv(); 
    jbyteArray res = env->functions->NewByteArray(env, len);
    JAVA_CHECK("NewByteArray");
    return res;
}

void JPJavaEnv::SetByteArrayRegion(jbyteArray array, int start, int len, jbyte* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetByteArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("SetByteArrayRegion");
}

void JPJavaEnv::GetByteArrayRegion(jbyteArray array, int start, int len, jbyte* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->GetByteArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("GetByteArrayRegion");
}

jbyte* JPJavaEnv::GetByteArrayElements(jbyteArray array, jboolean* isCopy)
{
    JNIEnv* env = getJNIEnv(); 
    jbyte* res = env->functions->GetByteArrayElements(env, array, isCopy);
    JAVA_CHECK("GetByteArrayElements");
    return res;
}

void JPJavaEnv::ReleaseByteArrayElements(jbyteArray array, jbyte* v, jint mode)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->ReleaseByteArrayElements(env, array, v, mode);
    JAVA_CHECK("ReleaseByteArrayElements");
}

jshortArray JPJavaEnv::NewShortArray(jint len)
{
    JNIEnv* env = getJNIEnv(); 
    jshortArray res = env->functions->NewShortArray(env, len);
    JAVA_CHECK("NewShortArray");
    return res;
}

void JPJavaEnv::SetShortArrayRegion(jshortArray array, int start, int len, jshort* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetShortArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("SetShortArrayRegion");
}

void JPJavaEnv::GetShortArrayRegion(jshortArray array, int start, int len, jshort* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->GetShortArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("GetShortArrayRegion");
}

jshort* JPJavaEnv::GetShortArrayElements(jshortArray array, jboolean* isCopy)
{
    JNIEnv* env = getJNIEnv(); 
    jshort* res = env->functions->GetShortArrayElements(env, array, isCopy);
    JAVA_CHECK("GetShortArrayElements");
    return res;
}

void JPJavaEnv::ReleaseShortArrayElements(jshortArray array, jshort* v, jint mode)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->ReleaseShortArrayElements(env, array, v, mode);
    JAVA_CHECK("ReleaseShortArrayElements");
}

jintArray JPJavaEnv::NewIntArray(jint len)
{
    JNIEnv* env = getJNIEnv(); 
    jintArray res = env->functions->NewIntArray(env, len);
    JAVA_CHECK("NewIntArray");
    return res;
}

void JPJavaEnv::SetIntArrayRegion(jintArray array, int start, int len, jint* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetIntArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("SetIntArrayRegion");
}

void JPJavaEnv::GetIntArrayRegion(jintArray array, int start, int len, jint* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->GetIntArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("GetIntArrayRegion");
}

jint* JPJavaEnv::GetIntArrayElements(jintArray array, jboolean* isCopy)
{
    JNIEnv* env = getJNIEnv(); 
    jint* res = env->functions->GetIntArrayElements(env, array, isCopy);
    JAVA_CHECK("GetIntArrayElements");
    return res;
}

void JPJavaEnv::ReleaseIntArrayElements(jintArray array, jint* v, jint mode)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->ReleaseIntArrayElements(env, array, v, mode);
    JAVA_CHECK("ReleaseIntArrayElements");
}

jlongArray JPJavaEnv::NewLongArray(jint len)
{
    JNIEnv* env = getJNIEnv(); 
    jlongArray res = env->functions->NewLongArray(env, len);
    JAVA_CHECK("NewLongArray");
    return res;
}

void JPJavaEnv::SetLongArrayRegion(jlongArray array, int start, int len, jlong* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetLongArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("SetLongArrayRegion");
}

void JPJavaEnv::GetLongArrayRegion(jlongArray array, int start, int len, jlong* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->GetLongArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("GetLongArrayRegion");
}

jlong* JPJavaEnv::GetLongArrayElements(jlongArray array, jboolean* isCopy)
{
    JNIEnv* env = getJNIEnv(); 
    jlong* res = env->functions->GetLongArrayElements(env, array, isCopy);
    JAVA_CHECK("GetLongArrayElements");
    return res;
}

void JPJavaEnv::ReleaseLongArrayElements(jlongArray array, jlong* v, jint mode)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->ReleaseLongArrayElements(env, array, v, mode);
    JAVA_CHECK("ReleaseLongArrayElements");
}

jfloatArray JPJavaEnv::NewFloatArray(jint len)
{
    JNIEnv* env = getJNIEnv(); 
    jfloatArray res = env->functions->NewFloatArray(env, len);
    JAVA_CHECK("NewFloatArray");
    return res;
}

void JPJavaEnv::SetFloatArrayRegion(jfloatArray array, int start, int len, jfloat* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetFloatArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("SetFloatArrayRegion");
}

void JPJavaEnv::GetFloatArrayRegion(jfloatArray array, int start, int len, jfloat* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->GetFloatArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("GetFloatArrayRegion");
}

jfloat* JPJavaEnv::GetFloatArrayElements(jfloatArray array, jboolean* isCopy)
{
    JNIEnv* env = getJNIEnv(); 
    jfloat* res = env->functions->GetFloatArrayElements(env, array, isCopy);
    JAVA_CHECK("GetFloatArrayElements");
    return res;
}

void JPJavaEnv::ReleaseFloatArrayElements(jfloatArray array, jfloat* v, jint mode)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->ReleaseFloatArrayElements(env, array, v, mode);
    JAVA_CHECK("ReleaseFloatArrayElements");
}

jdoubleArray JPJavaEnv::NewDoubleArray(jint len)
{
    JNIEnv* env = getJNIEnv(); 
    jdoubleArray res = env->functions->NewDoubleArray(env, len);
    JAVA_CHECK("NewDoubleArray");
    return res;
}

void JPJavaEnv::SetDoubleArrayRegion(jdoubleArray array, int start, int len, jdouble* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetDoubleArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("SetDoubleArrayRegion");
}

void JPJavaEnv::GetDoubleArrayRegion(jdoubleArray array, int start, int len, jdouble* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->GetDoubleArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("GetDoubleArrayRegion");
}

jdouble* JPJavaEnv::GetDoubleArrayElements(jdoubleArray array, jboolean* isCopy)
{
    JNIEnv* env = getJNIEnv(); 
    jdouble* res = env->functions->GetDoubleArrayElements(env, array, isCopy);
    JAVA_CHECK("GetDoubleArrayElements");
    return res;
}

void JPJavaEnv::ReleaseDoubleArrayElements(jdoubleArray array, jdouble* v, jint mode)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->ReleaseDoubleArrayElements(env, array, v, mode);
    JAVA_CHECK("ReleaseDoubleArrayElements");
}

jcharArray JPJavaEnv::NewCharArray(jint len)
{
    JNIEnv* env = getJNIEnv(); 
    jcharArray res = env->functions->NewCharArray(env, len);
    JAVA_CHECK("NewCharArray");
    return res;
}

void JPJavaEnv::SetCharArrayRegion(jcharArray array, int start, int len, jchar* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetCharArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("SetCharArrayRegion");
}

void JPJavaEnv::GetCharArrayRegion(jcharArray array, int start, int len, jchar* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->GetCharArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("GetCharArrayRegion");
}

jchar* JPJavaEnv::GetCharArrayElements(jcharArray array, jboolean* isCopy)
{
    JNIEnv* env = getJNIEnv(); 
    jchar* res = env->functions->GetCharArrayElements(env, array, isCopy);
    JAVA_CHECK("GetCharArrayElements");
    return res;
}

void JPJavaEnv::ReleaseCharArrayElements(jcharArray array, jchar* v, jint mode)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->ReleaseCharArrayElements(env, array, v, mode);
    JAVA_CHECK("ReleaseCharArrayElements");
}

jbooleanArray JPJavaEnv::NewBooleanArray(jint len)
{
    JNIEnv* env = getJNIEnv(); 
    jbooleanArray res = env->functions->NewBooleanArray(env, len);
    JAVA_CHECK("NewBooleanArray");
    return res;
}

void JPJavaEnv::SetBooleanArrayRegion(jbooleanArray array, int start, int len, jboolean* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->SetBooleanArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("SetBooleanArrayRegion");
}

void JPJavaEnv::GetBooleanArrayRegion(jbooleanArray array, int start, int len, jboolean* vals)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->GetBooleanArrayRegion(env, array, start, len, vals);
    JAVA_CHECK("GetBooleanArrayRegion");
}

jboolean* JPJavaEnv::GetBooleanArrayElements(jbooleanArray array, jboolean* isCopy)
{
    JNIEnv* env = getJNIEnv(); 
    jboolean* res = env->functions->GetBooleanArrayElements(env, array, isCopy);
    JAVA_CHECK("GetBooleanArrayElements");
    return res;
}

void JPJavaEnv::ReleaseBooleanArrayElements(jbooleanArray array, jboolean* v, jint mode)
{
    JNIEnv* env = getJNIEnv(); 
    env->functions->ReleaseBooleanArrayElements(env, array, v, mode);
    JAVA_CHECK("ReleaseBooleanArrayElements");
}

int JPJavaEnv::MonitorEnter(jobject a0)
{     int res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->MonitorEnter(env, a0);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("MonitorEnter");
    return res;

}

int JPJavaEnv::MonitorExit(jobject a0)
{     int res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->MonitorExit(env, a0);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("MonitorExit");
    return res;

}

jmethodID JPJavaEnv::FromReflectedMethod(jobject a0)
{     jmethodID res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->FromReflectedMethod(env, a0);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("FromReflectedMethod");
    return res;

}

jfieldID JPJavaEnv::FromReflectedField(jobject a0)
{     jfieldID res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->FromReflectedField(env, a0);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("FromReflectedField");
    return res;

}

jclass JPJavaEnv::FindClass(const char* a0)
{     jclass res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->FindClass(env, a0);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("FindClass");
    return res;

}

jboolean JPJavaEnv::IsInstanceOf(jobject a0, jclass a1)
{     jboolean res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->IsInstanceOf(env, a0, a1);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("IsInstanceOf");
    return res;

}

jobjectArray JPJavaEnv::NewObjectArray(int a0, jclass a1, jobject a2)
{     jobjectArray res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->NewObjectArray(env, a0, a1, a2);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("NewObjectArray");
    return res;

}

void JPJavaEnv::SetObjectArrayElement(jobjectArray a0, int a1, jobject a2)
{ 
    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	env->functions->SetObjectArrayElement(env, a0, a1, a2);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("SetObjectArrayElement");

}

void JPJavaEnv::CallStaticVoidMethodA(jclass a0, jmethodID a1, jvalue* a2)
{ 
    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	env->functions->CallStaticVoidMethodA(env, a0, a1, a2);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("CallStaticVoidMethodA");

}

void JPJavaEnv::CallVoidMethodA(jobject a0, jmethodID a1, jvalue* a2)
{ 
    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	env->functions->CallVoidMethodA(env, a0, a1, a2);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("CallVoidMethodA");

}

void JPJavaEnv::CallVoidMethod(jobject a0, jmethodID a1)
{ 
    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	env->functions->CallVoidMethod(env, a0, a1);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("CallVoidMethod");

}

jboolean JPJavaEnv::IsAssignableFrom(jclass a0, jclass a1)
{     jboolean res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->IsAssignableFrom(env, a0, a1);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("IsAssignableFrom");
    return res;

}

jstring JPJavaEnv::NewString(const jchar* a0, int a1)
{     jstring res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->NewString(env, a0, a1);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("NewString");
    return res;

}

jclass JPJavaEnv::GetSuperclass(jclass a0)
{     jclass res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->GetSuperclass(env, a0);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("GetSuperclass");
    return res;

}

const char* JPJavaEnv::GetStringUTFChars(jstring a0, jboolean* a1)
{     const char* res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->GetStringUTFChars(env, a0, a1);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("GetStringUTFChars");
    return res;

}

void JPJavaEnv::ReleaseStringUTFChars(jstring a0, const char* a1)
{ 
    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	env->functions->ReleaseStringUTFChars(env, a0, a1);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("ReleaseStringUTFChars");

}

jsize JPJavaEnv::GetArrayLength(jarray a0)
{     jsize res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->GetArrayLength(env, a0);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("GetArrayLength");
    return res;

}

jobject JPJavaEnv::GetObjectArrayElement(jobjectArray a0, int a1)
{     jobject res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->GetObjectArrayElement(env, a0, a1);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("GetObjectArrayElement");
    return res;

}

jclass JPJavaEnv::GetObjectClass(jobject a0)
{     jclass res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->GetObjectClass(env, a0);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("GetObjectClass");
    return res;

}

jmethodID JPJavaEnv::GetMethodID(jclass a0, char* a1, char* a2)
{     jmethodID res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->GetMethodID(env, a0, a1, a2);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("GetMethodID");
    return res;

}

jmethodID JPJavaEnv::GetStaticMethodID(jclass a0, char* a1, char* a2)
{     jmethodID res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->GetStaticMethodID(env, a0, a1, a2);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("GetStaticMethodID");
    return res;

}

jfieldID JPJavaEnv::GetFieldID(jclass a0, char* a1, char* a2)
{     jfieldID res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->GetFieldID(env, a0, a1, a2);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("GetFieldID");
    return res;

}

jfieldID JPJavaEnv::GetStaticFieldID(jclass a0, char* a1, char* a2)
{     jfieldID res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->GetStaticFieldID(env, a0, a1, a2);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("GetStaticFieldID");
    return res;

}

const jchar* JPJavaEnv::GetStringChars(jstring a0, jboolean* a1)
{
	const jchar* res;
	JNIEnv* env = getJNIEnv();
	res = env->functions->GetStringChars(env, a0, a1);
JAVA_CHECK("GetStringChars");
	return res;
}

void JPJavaEnv::ReleaseStringChars(jstring a0, const jchar* a1)
{
	JNIEnv* env = getJNIEnv();
	env->functions->ReleaseStringChars(env, a0, a1);
JAVA_CHECK("ReleaseStringChars");
}

jsize JPJavaEnv::GetStringLength(jstring a0)
{
	jsize res;
	JNIEnv* env = getJNIEnv();
	res = env->functions->GetStringLength(env, a0);
JAVA_CHECK("GetStringLength");
	return res;
}

jclass JPJavaEnv::DefineClass(const char* a0, jobject a1, const jbyte* a2, jsize a3)
{     jclass res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->DefineClass(env, a0, a1, a2, a3);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("DefineClass");
    return res;

}

jint JPJavaEnv::RegisterNatives(jclass a0, const JNINativeMethod* a1, jint a2)
{     jint res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->RegisterNatives(env, a0, a1, a2);
 
    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("RegisterNatives");
    return res;

}
