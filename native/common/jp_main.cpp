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
        int rc = 0;
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

        // Set isolated mode so that Python doesn't call exit
        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        config.isolated = 1;

        PyStatus status = PyConfig_SetBytesArgv(&config, argc, argv);
        if (PyStatus_Exception(status)) {
            PyConfig_Clear(&config);
            goto exception;
        }
        
        status = Py_InitializeFromConfig(&config);
        if (PyStatus_Exception(status)) {
            PyConfig_Clear(&config);
            goto exception;
        }
        PyConfig_Clear(&config);

        // Call Python from Java
        rc = Py_RunMain();

        // Problem:  Python doesn't exist the main loop and return here.
        // instead it finalizes and shutsdown everything even if there are 
        // other Java threads still using Python.  This is apparently inherent
        // in Python design.   So it may not be possible to get them cleanly
        // working together until Python splits there main loop like Java
        // did

exception:

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
