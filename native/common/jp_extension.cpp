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
 **************************************************************************** */
#include "jpype.h"
#include "pyjp.h"
#include "jp_proxy.h"
#include "jp_classloader.h"
#include "jp_reference_queue.h"
#include "jp_primitive_accessor.h"
#include "jp_boxedtype.h"
#include "jp_functional.h"

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_extension_Factory__init(
		JNIEnv *env, jclass clazz,
		jlong functionId,
		jobject self,
		jlong returnTypePtr,
		jlongArray parameterTypePtrs,
		jobjectArray args)
{
}

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_extension_Factory__call(
		JNIEnv *env, jclass clazz,
		jlong functionId,
		jobject self,
		jlong returnTypePtr,
		jlongArray parameterTypePtrs,
		jobjectArray args)
{
}
