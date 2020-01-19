/*
 * Copyright 2018 nelson85.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PYJP_H
#define PYJP_H
#include <Python.h>
#include <jpype.h>

#ifdef JP_TRACING_ENABLE
#define JP_PY_TRY(...) \
  JPypeTracer _trace(__VA_ARGS__); \
  try { do {} while(0)
#define JP_PY_CATCH(...) \
  } catch(...) { \
  JPPythonEnv::rethrow(JP_STACKINFO()); } \
  return __VA_ARGS__
#else
#define JP_PY_TRY(...)  try { do {} while(0)
#define JP_PY_CATCH(...)  } catch(...) \
  { JPPythonEnv::rethrow(JP_STACKINFO()); } \
  return __VA_ARGS__
#endif

/** This is the module specifically for the cpython modules to include.
 */
#include <pyjp_module.h>
#include <pyjp_array.h>
#include <pyjp_class.h>
#include <pyjp_field.h>
#include <pyjp_method.h>
#include <pyjp_module.h>
#include <pyjp_monitor.h>
#include <pyjp_value.h>


#ifdef __cplusplus
extern "C"
{
#endif

struct PyJPProxy
{
	PyObject_HEAD
	JPProxy* m_Proxy;
	PyObject* m_Target;
	PyObject* m_Callable;
} ;

extern PyTypeObject* PyJPProxy_Type;

#ifdef __cplusplus
}
#endif

#define ASSERT_JVM_RUNNING(X) JPEnv::assertJVMRunning(X, JP_STACKINFO())
#define PY_STANDARD_CATCH catch(...) { JPPythonEnv::rethrow(JP_STACKINFO()); }

#endif /* PYJP_H */
