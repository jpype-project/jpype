// --- file: python/pyjp_probe.cpp ---
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
#include "jp_gc.h"
#include "jp_proxy.h"
#include <set>
#include <vector>

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static void interrogate(PyJPModuleState* st, JPPyObject& interfaces, PyTypeObject *type)
{
	// Buffer appears in 3.12 so will probe dunder instead
	bool is_callable = (type->tp_call != nullptr);
	bool is_buffer = (type->tp_as_buffer != nullptr);

	bool seq_get = (type->tp_as_sequence != nullptr) && (type->tp_as_sequence->sq_item != nullptr);
	bool seq_set = (type->tp_as_sequence != nullptr) && (type->tp_as_sequence->sq_ass_item != nullptr);
	bool map_get = (type->tp_as_mapping != nullptr) && (type->tp_as_mapping->mp_subscript != nullptr);
	bool map_set = (type->tp_as_mapping != nullptr) && (type->tp_as_mapping->mp_ass_subscript != nullptr);

	bool as_float = (type->tp_as_number != nullptr) && (type->tp_as_number->nb_float != nullptr);
	bool as_int = (type->tp_as_number != nullptr) && (type->tp_as_number->nb_int != nullptr);
	bool logical = (type->tp_as_number != nullptr) && ((type->tp_as_number->nb_and != nullptr)
		|| (type->tp_as_number->nb_or != nullptr)
		|| (type->tp_as_number->nb_xor != nullptr));
	bool as_matrix = (type->tp_as_number != nullptr) && (type->tp_as_number->nb_matrix_multiply != nullptr);

	bool as_resource = (PyObject_HasAttrString((PyObject*) type, "__enter__") != 0);
	bool as_index = (PyObject_HasAttrString((PyObject*) type, "__index__") != 0);

	// High-speed subclass validations utilizing our pre-cached ABC objects
	bool is_collection = (st->abc_collection != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_collection) != 0);
	bool is_container = (st->abc_container != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_container) != 0);
	bool is_sequence = (st->abc_sequence != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_sequence) != 0);
	bool is_mapping = (st->abc_mapping != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_mapping) != 0);
	bool is_set = (st->abc_set != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_set) != 0);
	bool is_generator = (st->abc_generator != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_generator) != 0);
	bool is_iterable = (st->abc_iterable != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_iterable) != 0);
	bool is_iterator = (st->abc_iterator != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_iterator) != 0);
	bool is_awaitable = (st->abc_awaitable != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_awaitable) != 0);
	bool is_coroutine = (st->abc_coroutine != nullptr
			&& PyObject_IsSubclass((PyObject*) type, st->abc_coroutine) != 0);

	bool nb_and = (type->tp_as_number != nullptr) && (type->tp_as_number->nb_and != nullptr);
	bool nb_or = (type->tp_as_number != nullptr) && (type->tp_as_number->nb_or != nullptr);
	bool nb_xor = (type->tp_as_number != nullptr) && (type->tp_as_number->nb_xor != nullptr);

#if 0
	printf("probe type=%s\n", type->tp_name);
	printf("  is_callable=%d\n", is_callable);
	printf("  is_buffer=%d\n", is_buffer);

	printf("  seq_get=%d\n", seq_get);
	printf("  seq_set=%d\n", seq_set);
	printf("  map_get=%d\n", map_get);
	printf("  map_set=%d\n", map_set);

	printf("  as_float=%d\n", as_float);
	printf("  as_int=%d\n", as_int);
	printf("  logical=%d\n", logical);
	printf("  as_matrix=%d\n", as_matrix);

	printf("  as_resource=%d\n", as_resource);
	printf("  as_index=%d\n", as_index);

	printf("  is_collection=%d\n", is_collection);
	printf("  is_container=%d\n", is_container);
	printf("  is_sequence=%d\n", is_sequence);
	printf("  is_mapping=%d\n", is_mapping);
	printf("  is_set=%d\n", is_set);
	printf("  is_generator=%d\n", is_generator);
	printf("  is_iterable=%d\n", is_iterable);
	printf("  is_iterator=%d\n", is_iterator);
	printf("  is_awaitable=%d\n", is_awaitable);
	printf("  is_coroutine=%d\n", is_coroutine);
	printf("  nb_and=%d nb_or=%d nb_xor=%d\n", nb_and, nb_or, nb_xor);
	printf("\n");
	fflush(stdout);
#endif

	JPPyObject cls;

	// We always add PyObject methods
	PyObject *object = PyDict_GetItem(st->concreteDict, (PyObject*) &PyBaseObject_Type); // borrowed
	if (object != nullptr)
		PyList_Append(interfaces.get(), object);

	bool flags[15] = {
		is_callable, is_buffer, is_sequence, is_mapping,
		is_iterable, is_iterator, is_generator, is_coroutine,
		is_awaitable, is_set, is_collection, is_container,
		as_index, (as_int || as_float), (!as_int && !as_float && nb_or)
	};

	for (int i = 0; i < 15; ++i)
	{
		if (flags[i] && st->protocol_pipeline[i] != nullptr)
			PyList_Append(interfaces.get(), st->protocol_pipeline[i]);
	}
}

bool finalizeInterfaces(PyJPModuleState* st, JPPyObject& existing_interfaces, JPPyObject& interfaces)
{
	// Convert the list of interfaces into a tuple
	//   ::call will throw so no need to check isValid
	JPPyObject interfaces_tuple = JPPyObject::call(PyList_AsTuple(interfaces.get()));

	// Perform lookup in cacheInterfacesDict
	PyObject* item = PyDict_GetItem(st->cacheInterfacesDict, interfaces_tuple.get()); // Borrowed reference
	if (item == nullptr)
	{
		// If the tuple does not exist, insert it as both the key and value
		if (PyDict_SetItem(st->cacheInterfacesDict, interfaces_tuple.get(), interfaces_tuple.get()) == -1)
			return false;

		// Wrap the newly inserted item in JPPyObject
		existing_interfaces = JPPyObject::use(interfaces_tuple.get());
	}
	else
	{
		// Wrap the borrowed reference
		existing_interfaces = JPPyObject::use(item);
	}
	return true;
}

bool finalizeMethods(PyJPModuleState* st, JPPyObject& existing_methods, JPPyObject& existing_interfaces)
{
	// First consult the methods cache for the tuple
	PyObject* methods_item = PyDict_GetItem(st->cacheMethodsDict, existing_interfaces.get()); // Borrowed reference
	if (methods_item == nullptr)
	{
		// Create a new dictionary for methods
		JPPyObject new_methods = JPPyObject::call(PyDict_New());

		// Iterate through the tuple and update methods
		PyObject* it = existing_interfaces.get();
		Py_ssize_t tuple_size = PyTuple_Size(it);
		for (Py_ssize_t i = 0; i < tuple_size; ++i)
		{
			PyObject* interf = PyTuple_GetItem(it, i); // Borrowed reference
			if (interf == nullptr)
				return false;

			PyObject* methods_item = PyDict_GetItem(st->methodsDict, interf); // Borrowed reference
			if (methods_item != nullptr)
			{
				JPPyObject methods = JPPyObject::use(methods_item);
				if (PyDict_Update(new_methods.get(), methods.get()) == -1)
					return false;
			}
		}

		// Insert the new methods dictionary into cacheMethodsDict
		if (PyDict_SetItem(st->cacheMethodsDict, existing_interfaces.get(), new_methods.get()) == -1)
			return false;

		existing_methods = new_methods;
	}
	else
	{
		// Wrap the borrowed reference
		existing_methods = JPPyObject::use(methods_item);
	}
	return true;
}


// Helper function to recursively crawl the interface tree
void collectInterfacesRecursive(JPClass* currentClass, std::set<JPClass*>& visited, std::vector<JPClass*>& orderedList)
{
	if (currentClass == nullptr)
		return;

	// Fetch the explicit list of interfaces declared directly by this class/interface
	const std::vector<JPClass*>& intf_list = currentClass->getInterfaces();
	
	for (JPClass* intf : intf_list)
	{
		if (visited.find(intf) != visited.end())
			continue;
		if (!intf->isPython())
			continue;
		// Mark it as visited to avoid infinite recursion loops 
		visited.insert(intf);
		
		// Push it to our flat ordered collection list
		orderedList.push_back(intf);
		
		// Recursively collect all parents/extended interfaces of THIS interface
		collectInterfacesRecursive(intf, visited, orderedList);
	}
}

// Returns a new reference and can set exceptions
PyObject* PyJP_probe(PyJPModuleState* st, PyTypeObject *type)
{
	// We would start by checking the cache here
	JPPyObject cached = JPPyObject::use(PyObject_GetItem(st->cacheDict, (PyObject*) type));
	if (cached.isValid())
		return cached.keep();
	PyErr_Clear();

	// Safety check: Ensure our late-binding ready() milestone was reached
	if (st->abc_sequence == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError,
				"JPype Engine Error: ready() checkpoint was not initialized.");
		return nullptr;
	}

	// So first method is to search for a concrete which isn't object
	PyObject *mro = type->tp_mro;
	Py_ssize_t sz = PyTuple_Size(mro);

	JPPyObject interfaces = JPPyObject::accept(PyList_New(0));
	JPPyObject existing_interfaces;
	JPPyObject existing_methods;

	// 2. SHORT-CIRCUIT MECHANISM: Scan MRO for an exact concrete match
	// Skip the very last element, which is always object
	PyObject *concrete_match = nullptr;
	for (Py_ssize_t i = 0; i < sz - 1; ++i)
	{
		PyObject *mro_class = PyTuple_GetItem(mro, i); // borrowed
		PyObject *found = PyDict_GetItem(st->concreteDict, mro_class); // borrowed

		if (found != nullptr)
		{
			concrete_match = found;
			break;
		}
	}

	if (concrete_match != nullptr)
	{
		JPJavaFrame frame = JPJavaFrame::outer(st->context);
		JPClass* targetClass = PyJPClass_getJPClass((PyObject*) concrete_match);
		if (targetClass == nullptr)
		{
			// If the type mapping data isn't initialized yet, clear error and force fallback
			PyErr_Clear();
			concrete_match = nullptr;
		}
		else
		{
			// 1. Append the concrete base interface behavior we matched in the MRO
			PyList_Append(interfaces.get(), concrete_match);

			// 2. Set up collections to capture the deep dependency graph
			std::set<JPClass*> visitedInterfaces;
			std::vector<JPClass*> deepInterfaceList;

			// 3. Crawl recursively up the tree starting from our matched class context
			collectInterfacesRecursive(targetClass, visitedInterfaces, deepInterfaceList);

			// 4. Safely convert and append all discovered interfaces back into Python space
			for (JPClass* intf : deepInterfaceList)
			{
				PyTypeObject* py_intf = intf->getHost();
				if (py_intf != nullptr)
				{
					PyList_Append(interfaces.get(), (PyObject*) py_intf);
				}
			}
		}
	}

	if (concrete_match == nullptr)
	{
		// Fallback to structural duck-typing analysis
		interrogate(st, interfaces, type);

		// FIXME remove this once we confirm the interface lists are all synced up
		// We look to see if there is a concrete method
		if (sz > 1)
		{
			PyObject *primary = PyTuple_GetItem(mro, sz - 2); // borrowed
			JPPyObject cls = JPPyObject::use(PyDict_GetItem(st->concreteDict, primary));
			if (cls.isValid())
				PyList_Append(interfaces.get(), cls.get());
		}
	}

	// Process pipeline compilation stages safely tracking return results
	if (!finalizeInterfaces(st, existing_interfaces, interfaces))
		return nullptr;

	if (!finalizeMethods(st, existing_methods, existing_interfaces))
		return nullptr;

	// Build the final target tuple package
	JPPyObject result = JPPyObject::call(PyTuple_New(2));
	if (!result.isValid())
		return nullptr;

	PyTuple_SetItem(result.get(), 0, existing_interfaces.keep()); // Reference transferred
	PyTuple_SetItem(result.get(), 1, existing_methods.keep());

	// Cache the resolved blueprint for instantaneous future lookups
	if (PyObject_SetItem(st->cacheDict, (PyObject*) type, result.get()) == -1)
		return nullptr;

	return result.keep();
}


// Returns a new reference and can set exceptions
PyObject* PyJP_pyobject(PyJPModuleState* st, PyTypeObject *target_type, PyObject *object_to_cast)
{
	JPClass* targetClass = PyJPClass_getJPClass((PyObject*) target_type);
	if (targetClass == nullptr)
		return nullptr;

	JPContext* context = st->context;
	if (context == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype module context is not available");
		return nullptr;
	}

	JPJavaFrame frame = JPJavaFrame::outer(context);

	if (object_to_cast == Py_None)
	{
		jvalue v;
		v.l = 0;
		return targetClass->convertToPythonObject(frame, v, true).keep();
	}

	// Probe using the targeted type class to find matching method definitions
	JPPyObject probe_result = JPPyObject::accept(PyJP_probe(st, Py_TYPE(object_to_cast)));
	if (!probe_result.isValid())
		return nullptr;

	// Extract the resolved Java class and method pointers from the probe tuple
	JPPyObject jcls = JPPyObject::use(PyTuple_GetItem(probe_result.get(), 0));
	JPPyObject meth = JPPyObject::use(PyTuple_GetItem(probe_result.get(), 1));

	bool type_matched = false;
	if (PyTuple_Check(jcls.get()))
	{
		Py_ssize_t size = PyTuple_Size(jcls.get());
		for (Py_ssize_t i = 0; i < size; ++i)
		{
			if (PyTuple_GetItem(jcls.get(), i) == (PyObject*) target_type)
			{
				type_matched = true;
				break;
			}
		}
	}
	else if (jcls.get() == (PyObject*) target_type)
	{
		type_matched = true;
	}

	if (!type_matched)
	{
		const char* source_name = Py_TYPE(object_to_cast)->tp_name;
		const char* target_name = target_type->tp_name;

		PyErr_Format(PyExc_TypeError,
			"Type mismatch on cast: Python object of type '%s' does not satisfy type '%s'.",
			source_name, target_name);
		return nullptr;
	}

	// Pack arguments securely: (object_to_cast, dispatch_meth, target_jcls, convert_flag)
	JPPyObject proxy_args = JPPyObject::accept(
			PyTuple_Pack(4, object_to_cast, meth.get(), jcls.get(), Py_True));
	if (!proxy_args.isValid())
		return nullptr;

	// Create the specialized JPProxy type instance
	JPPyObject proxy_instance = JPPyObject::accept(
			st->PyJPProxy_Type->tp_new(st->PyJPProxy_Type, proxy_args.get(), nullptr));
	if (!proxy_instance.isValid())
		return nullptr;

	JPProxy *proxy = PyJPProxy_getJPProxy(st, proxy_instance.get());
	if (proxy == nullptr)
		return nullptr;

	// Fetch the live JNI local/global reference structure
	jvalue v = proxy->getProxy(frame);

	// Convert using the exact target class type, not a generic context type
	// FIXME we can make this more efficient by passing the class to the backend and get 0 if the cast would fail
	JPClass* clz = frame.findClassForObject(v.l);
	if (frame.IsAssignableFrom(clz->getJavaClass(frame), targetClass->getJavaClass(frame)) != 1)
	{
		PyErr_SetString(PyExc_TypeError, "Type mismatch on cast");
		return nullptr;
	}

	return targetClass->convertToPythonObject(frame, v, true).keep();
}

#ifdef __cplusplus
}
#endif
