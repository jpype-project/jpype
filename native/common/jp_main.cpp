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

void PyJPModule_installGC(PyObject* module);
void PyJPModule_loadResources(PyObject* module);

extern jobject urlClassLoader;

extern "C" JNIEXPORT void JNICALL Java_org_jpype_Main_initialize(
		JNIEnv *env, jclass clazz,
        jobject classLoader)
{
    try
    {
        // Set the classloader so that we don't make a second one during bootstrapping
        urlClassLoader = classLoader;
        // Attach the JVM
        JPContext_global->attachJVM(env, true);
    } catch (JPypeException& ex)
    {
        printf("JPypeException\n");
    } catch (...)  // GCOVR_EXCL_LINE
    {
        printf("other exception\n");
    }
}

extern "C" JNIEXPORT void JNICALL Java_org_jpype_Main_launch(
		JNIEnv *env, jclass clazz,
		jobjectArray args)
{
    try {
        // Fetch the Python modules
        JPPyObject publicModule = JPPyObject::use(PyImport_ImportModule("jpype"));
        JPPyObject privateModule = JPPyObject::use(PyImport_ImportModule("_jpype"));
        JPPyObject builtins = JPPyObject::use(PyEval_GetBuiltins());
        if (publicModule.isNull() || privateModule.isNull() || builtins.isNull())
        {
            fprintf(stderr, "Unable to find required resources\n");
            return;
        }

        // Set up the GC
        PyJPModule_installGC(privateModule.get());
        PyObject_SetAttrString(builtins.get(), "jpype", publicModule.get());
        PyJPModule_loadResources(privateModule.get());

        // Copy the arguments
        int argc = env->GetArrayLength(args);
        char** argv = new char*[argc];
        for (int i = 0; i<argc; ++i)
        {   
            jboolean iscpy = 0;
            jstring str = (jstring) env->GetObjectArrayElement(args, i);
            const char* c = env->GetStringUTFChars(str, &iscpy);
            argv[i] = strdup(c);
            if (iscpy)
                env->ReleaseStringUTFChars(str, c);
        }

        // Redirect to Python
        printf("Python main start\n"); fflush(stdout);
        Py_BytesMain(argc, argv);
        printf("Python main end\n"); fflush(stdout);

        // Dump the memory
        for (int i = 0; i<argc; ++i)
        {   
            free(argv[i]);
        }

        // At this point there may be other threads launched by Java or Python so we 
        // can't cleanup.  Just return control.
    } catch (JPypeException& ex)
    {
        JP_TRACE("JPypeException raised");
        printf("JPypeException\n");
    } catch (...)  // GCOVR_EXCL_LINE
    {
        printf("other exception\n");
    }
    return;
}
