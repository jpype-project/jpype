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

extern "C" JNIEXPORT void JNICALL Java_org_jpype_Main_launch(
		JNIEnv *env, jclass clazz,
		jobjectArray args)
{
    try
    {
        JP_TRACE_IN("Java_org_jpype_Main_launch");
        try {
            // Attach the JVM and set up the resources for the private module
            printf("fetch module\n");
            JPPyObject publicModule = JPPyObject::use(PyImport_ImportModule("jpype"));
            JPPyObject privateModule = JPPyObject::use(PyImport_ImportModule("_jpype"));
            JPPyObject builtins = JPPyObject::use(PyEval_GetBuiltins());
            if (publicModule.isNull() || privateModule.isNull() || builtins.isNull())
            {
                fprintf(stderr, "Unable to find required resources\n");
                return;
            }
            printf("add to builtin\n");
            //PyObject_SetAttrString(builtins.get(), "jpype", publicModule.get());

            printf("attach\n");
            JPContext_global->attachJVM(env, true);
            printf("install gc\n");
            PyJPModule_installGC(privateModule.get());
            printf("load resources\n");
            PyJPModule_loadResources(privateModule.get());

            printf("parse args\n");

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
            Py_BytesMain(argc, argv);

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
        JP_TRACE_OUT;  // GCOVR_EXCL_LINE
    }
    catch (...) // JP_TRACE_OUT implies a throw but that is not allowed.
    {}
}

