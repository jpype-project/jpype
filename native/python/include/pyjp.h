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

/** This is the module specifically for the cpython modules to include.
 */
#include <Python.h>
#include <jpype.h>
#include <pyjp_module.h>
#include <pyjp_array.h>
#include <pyjp_class.h>
#include <pyjp_field.h>
#include <pyjp_method.h>
#include <pyjp_module.h>
#include <pyjp_monitor.h>
#include <pyjp_value.h>
#include <pyjp_context.h>
#include <pyjp_proxy.h>

#define ASSERT_JVM_RUNNING(context, X) context->assertJVMRunning(X, JP_STACKINFO())
#define PY_STANDARD_CATCH catch(...) { JPPythonEnv::rethrow(JP_STACKINFO()); }

#endif /* PYJP_H */
