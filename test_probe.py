import jpype
import _jpype
import numpy as np
import weakref
import inspect
from jpype import JClass

jpype.startJVM()

#############################################################################
# Define all the Java types using JClass

PyByteArray = JClass("python.lang.PyByteArray")
PyBytes = JClass("python.lang.PyBytes")
PyComplex = JClass("python.lang.PyComplex")
PyDict = JClass("python.lang.PyDict")
PyEnumerate = JClass("python.lang.PyEnumerate")
PyFloat = JClass("python.lang.PyFloat")
PyFrozenSet = JClass("python.lang.PyFrozenSet")
PyExc = JClass("python.lang.PyExc")
PyInt = JClass("python.lang.PyInt")
PyList = JClass("python.lang.PyList")
PyMemoryView =  JClass("python.lang.PyMemoryView")
PyObject = JClass("python.lang.PyObject")
PyRange =  JClass("python.lang.PyList")
PySet =  JClass("python.lang.PySet")
PySlice = JClass("python.lang.PySlice")
PyString = JClass("python.lang.PyString")
PyTuple = JClass("python.lang.PyTuple")
PyType =  JClass("python.lang.PyType")
PyZip = JClass("python.lang.PyZip")

PyAbstractSet = JClass("python.lang.PyAbstractSet")
PyAwaitable = JClass("python.lang.PyAwaitable")
PyBuffer = JClass("python.lang.PyBuffer")
PyCallable = JClass("python.lang.PyCallable")
PyCollection = JClass("python.lang.PyCollection")
PyContainer = JClass("python.lang.PyContainer")
PyCoroutine = JClass("python.lang.PyCoroutine")
PyGenerator = JClass("python.lang.PyGenerator")
PyIndex = JClass("python.lang.PyIndex")
PyIter = JClass("python.lang.PyIter")
PyIterable = JClass("python.lang.PyIterable")
PyMapping = JClass("python.lang.PyMapping")
PyMutableSet = JClass("python.lang.PyMutableSet")
PyNumber = JClass("python.lang.PyNumber")
PySequence = JClass("python.lang.PySequence")
PySized = JClass("python.lang.PySized")


#############################################################################
# Add all of the concrete types to the _concrete interfaces list.
_jpype._concrete[bytearray] = PyByteArray
_jpype._concrete[bytes] = PyBytes
_jpype._concrete[complex] = PyComplex
_jpype._concrete[dict] = PyDict
_jpype._concrete[enumerate] = PyEnumerate
_jpype._concrete[float] = PyFloat
_jpype._concrete[frozenset] = PyFrozenSet
_jpype._concrete[BaseException] = PyExc
_jpype._concrete[int] = PyInt
_jpype._concrete[list] = PyList
_jpype._concrete[memoryview] =  PyMemoryView
_jpype._concrete[object] = PyObject
_jpype._concrete[range] =  PyList
_jpype._concrete[set] =  PySet
_jpype._concrete[slice] = PySlice
_jpype._concrete[str] = PyString
_jpype._concrete[tuple] = PyTuple
_jpype._concrete[type] =  PyType
_jpype._concrete[zip] = PyZip

#############################################################################
# Add all of the abstract types to the _protocol interfaces list
# The key must be a string and the value a Java class
_jpype._protocol["abstract_set"] = PyAbstractSet
_jpype._protocol["awaitable"] = PyAwaitable
_jpype._protocol["buffer"] = PyBuffer
_jpype._protocol["callable"] = PyCallable
_jpype._protocol["collection"] = PyCollection
_jpype._protocol["container"] = PyContainer
_jpype._protocol["coroutine"] = PyCoroutine
_jpype._protocol["generator"] = PyGenerator
_jpype._protocol["index"] = PyIndex
_jpype._protocol["iter"] = PyIter
_jpype._protocol["iterable"] = PyIterable
_jpype._protocol["mapping"] = PyMapping
_jpype._protocol["mutable_set"] = PyMutableSet
_jpype._protocol["number"] = PyNumber
_jpype._protocol["sequence"] = PySequence
_jpype._protocol["sized"] = PySized

#############################################################################
# This section defines all of the functions required to implement the Java
# interfaces when a builtin or class method is not sufficient.

# Specific object for missing fields
missing = object()

# Generic methods that aply to many Python objects
def _contains(obj, value):
    return value in obj

def _equals(x,y):
    return x == y

def _getitem(self, index):
    return self[index]

def _getitem_range(self, start, end):
    return self[start:end]

def _setitem(self, index, obj):
    self[index] = obj

def _setitem_return(self, index, obj):
    old_value = self[index]
    self[index] = value
    return old_value

def _delitem(self, index):
    del self[index]

def _delitem_return(self, index):
    item = self[index]
    del self[index]
    return item

  

# Specific methods by type
def complex_real(self):
    return self.real

def complex_imag(self):
    return self.imag

def complex_conjugate(self):
    return self.imag

def dict_pop(self, key, default_value=None):
    return self.pop(key, default_value)

def dict_pop_item(self):
    if not self:
        raise KeyError("popitem(): dictionary is empty")
    return self.popitem()

def dict_put(self, key, value):
    rc = self.pop(key, None)
    self[key] = value
    return rc

def dict_remove(self, key, value=missing):
    if value is missing:
        if self.get(key) == value:
            del self[key]
            return True
        return False
    return self.pop(key, None)

def dict_update_iterable(self, iterable):
    for key, value in iterable:
        self[key] = value

def frozenset_difference(self, *sets):
    return self.difference(*sets)

def frozenset_intersect(self, *sets):
    return self.intersection(*sets)

def frozenset_symmetric_difference(self, *sets):
    return self.symmetric_difference(*sets)

def iterable_map_elements(self, callable):
    return map(callable, self)

def iter_filter(self, callable):
    return filter(callable, self)

def list_add(self, e, v=missing):
    if v is missing:
        self.append(e)
        return True
    self.insert(index, element)
    
def seq_index_of(self, o):
    try:
        return self.index(o)
    except ValueError:
        return -1

def list_insert(self, index, c):
    for i, element in enumerate(c):
        self.insert(index + i, element)

def list_remove_all(self, c):
    c = set(c)
    original_size = len(self)
    self[:] = [item for item in self if item not in c]
    return len(self) != original_size

def list_retain_all(self, c):
    c = set(c)
    original_size = len(self)
    self[:] = [item for item in self if item in c]
    return len(self) != original_size

def list_set(self, index, element):
    old_value = self[index]
    self[index] = element
    return old_value

def list_sublist(self, from_index, to_index):
    if from_index > to_index:
        raise ValueError("fromIndex must be less than or equal to toIndex")
    return self[from_index:to_index]

def frozenset_union(self, *sets):
    return self.union(*sets)

def mapping_contains_value(self, value):
    return value in self.values()

def mapping_get(self, key, default=None):
    return self.get(key, default)

def mapping_put(self, key, value):
    previous_value = self.get(key, None)
    self[key] = value
    return previous_value

def mapping_put_all(self, other_mapping):
    self.update(other_mapping)

def mapping_remove(self, key):
    return self.pop(key, None)

def mapping_clear(self):
    self.clear()

def memoryview_is_read_only(self):
    return self.readonly

def memoryview_get_format(self):
    return self.format

def memoryview_get_shape(self):
    return self.shape

def memoryview_get_strides(self):
    return self.strides

def memoryview_get_sub_offsets(self):
    return self.suboffsets

def memoryview_get_buffer(self):
    return self.obj  # Access the underlying buffer object

def number_add(self, other):
    return self + other

def number_add_in_place(self, other):
    self += other
    return self

def number_subtract(self, other):
    return self - other

def number_subtract_in_place(self, other):
    self -= other
    return self

def number_multiply(self, other):
    return self * other

def number_multiply_in_place(self, other):
    self *= other
    return self

def number_divide(self, other):
    return self / other

def number_divide_in_place(self, other):
    self /= other
    return self

def number_floor_divide(self, other):
    return self // other

def number_modulus(self, other):
    return self % other

def number_power(self, exponent):
    return self ** exponent

def number_negate(self):
    return -self

def number_positive(self):
    return +self

def number_compare_to(self, other):
    return (self > other) - (self < other)

def range_get_start(obj):
    return obj.start

def range_get_stop(obj):
    return obj.stop

def range_get_step(obj):
    return obj.step

def range_get_slice(obj, start, end):
    return range(obj.start + start * obj.step, 
                 obj.start + end * obj.step, 
                 obj.step)

def set_add(self, element):
    rc = element in self
    self.add(element)
    return rc

def set_difference(self, *sets):
    return self.difference(*sets)

def set_difference_update(self, *sets):
    self.difference_update(*sets)

def set_equals(self, obj):
    return self == obj

def set_intersect(self, *sets):
    return self.intersection(*sets)

def set_intersection_update(self, *sets):
    self.intersection_update(*sets)

def set_is_disjoint(self, other_set):
    return self.isdisjoint(other_set)

def set_is_superset(self, other_set):
    return self.issuperset(other_set)

def set_symmetric_difference(self, *sets):
    return self.symmetric_difference(*sets)

def set_symmetric_difference_update(self, other_set):
    self.symmetric_difference_update(other_set)

def set_union(self, *sets):
    return self.union(*sets)

def set_union_update(self, *sets):
    self.update(*sets)

def sequence_get_slice(self, *indices):
    return self[tuple(*indices)]

def slice_get_start(self):
    return self.start

def slice_get_stop(self):
    return self.stop

def slice_get_step(self):
    return self.step

def slice_indices(self, length):
    return self.indices(length)

def slice_is_valid(self):
    if self.step == 0:
      return False
    return True

def str_count_occurrences(self, substring, start=None, end=None):
    if start is None and end is None:
        return self.count(substring)
    elif end is None:
        return self.count(substring, start)
    else:
        return self.count(substring, start, end)

def str_ends_with_suffix(self, suffix, start=None, end=None):
    if start is None and end is None:
        return self.endswith(suffix)
    elif end is None:
        return self[start:].endswith(suffix)
    else:
        return self[start:end].endswith(suffix)

def str_expand_tabs_to_spaces(self, tab_size):
    return self.expandtabs(tab_size)

def str_find_last_substring(self, substring, start=None, end=None):
    if start is None and end is None:
        return self.rfind(substring)
    elif end is None:
        return self.rfind(substring, start)
    else:
        return self.rfind(substring, start, end)

def str_find_substring(self, substring, start=None, end=None):
    if start is None and end is None:
        return self.find(substring)
    elif end is None:
        return self.find(substring, start)
    else:
        return self.find(substring, start, end)

def str_format_using_mapping(self, mapping):
    return self.format_map(mapping)

def str_format_with(self, args, kwargs):
    return self.format(*args, **kwargs)

def str_index_of_last_substring(self, substring, start=None, end=None):
    if start is None and end is None:
        return self.rindex(substring)
    elif end is None:
        return self.rindex(substring, start)
    else:
        return self.rindex(substring, start, end)

def str_index_of_substring(self, substring, start=None, end=None):
    if start is None and end is None:
        return self.index(substring)
    elif end is None:
        return self.index(substring, start)
    else:
        return self.index(substring, start, end)

def str_padded_center(self, width, fill=' '):
    return self.center(width, fill)

def str_remove_leading_prefix(self, prefix):
    return self.removeprefix(prefix)

def str_remove_trailing_suffix(self, suffix):
    return self.removesuffix(suffix)

def str_replace_substring(self, old_substring, replacement, count=None):
    if count is None:
        return self.replace(old_substring, replacement)
    else:
        return self.replace(old_substring, replacement, count)

def str_split_into(self, separator=None, max_split=-1):
    return self.split(separator, max_split)

def str_split_into_lines(self, keep_ends=False):
    return self.splitlines(keep_ends)

def str_split_into_reverse(self, separator=None, max_split=-1):
    return self.rsplit(separator, max_split)

def str_starts_with_prefix(self, prefix, start=None, end=None):
    if start is None and end is None:
        return self.startswith(prefix)
    elif end is None:
        return self.startswith(prefix, start)
    else:
        return self.startswith(prefix, start, end)

def str_strip_characters(self, characters=None):
    return self.strip(characters)

def str_strip_leading(self, characters=None):
    return self.lstrip(characters)

def str_strip_trailing(self, characters=None):
    return self.rstrip(characters)

def str_strip_whitespace(self):
    return self.strip()

def str_to_encoded(self, encoding='utf-8', error_handling='strict'):
    return self.encode(encoding, error_handling)

def tuple_index_of(self, obj):
    try:
        return self.index(obj)
    except ValueError:
        return -1

def type_get_name(self):
    return self.__name__

def type_mro(self):
    return self.__mro__

def type_get_base(self):
    return self.__base__

def type_get_bases(self):
    return self.__bases__

def type_is_subclass_of(self, type):
    return issubclass(self, type)

def type_is_instance(self, obj):
    return isinstance(obj, self)

def type_get_method(self, name):
    return getattr(self, name, None)

def type_is_abstract(self):
    from abc import ABCMeta
    return isinstance(self, ABCMeta)

def type_get_subclasses(self):
    return self.__subclasses__()

def _call(obj, args, kwargs):
    if kwargs is not None:
        return obj(*args, **kwargs)
    return obj(*args)
    
def _delattr_return(obj, key):
    return var(obj).pop(key, None)

def _delattr(obj, key):
    delattr(obj, key)

def dict_keys(obj):
    return obj.keys()

def dict_values(obj):
    return obj.keys()

def dict_iterable(obj, iterable):
    out = dict()
    out.update(iterable)
    return out

def object_doc(obj):
    return __doc__

def callable_signature(obj):
    return inspect.signature(obj)

def type_isinstance(obj, types):
    return isinstance(obj, tuple(types))

def dict_items(self):
    return self.items()

backend_methods = {
    "bytearray": bytearray,
    "bytearrayFromHex": bytearray.fromhex,
    "bytes": bytes,
    "bytesFromHex": bytes.fromhex,
    "call": _call,
    "contains": _contains,
    "delitemByIndex": _delitem,
    "delitemByObject": _delitem,
    "delattrReturn": _delattr_return,
    "delattrString": delattr,
    "dir": dir,
    "enumerate": enumerate,
    "eval": eval,
    "exec": exec,
    "getDict": vars,
    "getDocString": object_doc,
    "getSignature": callable_signature,
    "getattrDefault": getattr,
    "getattrObject": getattr,
    "getattrString": getattr,
    "getitemMappingObject": _getitem,
    "getitemMappingString": _getitem,
    "getitemSequence": _getitem,
    "hasattrString": hasattr,
    "isCallable": callable,
    "isinstanceFromArray": type_isinstance,
    "items": dict_items,
    "keys": dict_keys,
    "len": len,
    "list": list,
    "memoryview": memoryview,
    "newByteArray": bytearray,
    "newByteArrayFromBuffer": bytes,
    "newByteArrayFromIterable": bytearray,
    "newByteArrayOfSize": bytearray,
    "newBytesOfSize": bytes,
    "newComplex": complex,
    "newDict": dict,
    "newDictFromIterable": dict,
    "newEnumerate": enumerate,
    "newFloat": float,
    "newFrozenSet": frozenset,
    "newInt": int,
    "newList": list,
    "newListFromIterable": list,
    "newSet": set,
    "newSetFromIterable": set,
    "newTuple": tuple,
    "newTupleFromIterator": tuple,
    "newZip": zip,
}

#############################################################################
# This section builds dictionaries to map from the Java interfaces to the
# Python implementation.  If the behavior of the Java method matches that in
# Python both for arguments and for the return value, then a reference to the
# class method is used.  If the arguments and return match a Python builtin
# function then the Python builtin is used.  If either the arguments or the
# return do not match then a Python implementation of the function must be
# implemented.
#
# Only non-default methods appear in the methods dictionaries.

# Define method dictionaries
_jpype._methods[PyByteArray] = {
    "decode": bytearray.decode,
    "translate": bytearray.translate,
}
_jpype._methods[PyBytes] = {
    "decode": bytes.decode,
    "translate": bytes.translate,
}
_jpype._methods[PyComplex] = {
    "real": complex_real,
    "imag": complex_imag,
    "conjugate": complex_conjugate,
}
# Define the dictionary for PyDict methods
_jpype._methods[PyDict] = {
    "clear": dict.clear,
    "containsKey": _contains,
    "containsValue": mapping_contains_value,
    "entrySet": dict_items,
    "get": mapping_get,
    "getOrDefault": mapping_get,
    "keySet": dict_keys,
    "pop": dict_pop,
    "popItem": dict_pop_item,
    "put": dict_put,
    "putAny": dict_put,
    "putAll": dict.update,
    "remove": dict_remove,
    "setDefault": dict.setdefault,
    "update": dict.update,
    "updateIterable": dict_update_iterable,
}
_jpype._methods[PyEnumerate] = {} # No java methods
_jpype._methods[PyFloat] = {} # No Java methods
_jpype._methods[PyFrozenSet] = {
    "copy": frozenset.copy,
    "difference": frozenset_difference,
    "intersect": frozenset_intersect,
    "isDisjoint": frozenset.isdisjoint,
    "isSubset": frozenset.issubset,
    "isSuperset": frozenset.issuperset,
    "symmetricDifference": frozenset_symmetric_difference,
    "union": frozenset_union,
}
_jpype._methods[PyExc] = {} # No java methods
_jpype._methods[PyInt] = {} # No java methods
_jpype._methods[PyList] = {
    "add": list_add,
    "addAny": list_add,
    "clear": list.clear,
    "contains": _contains,
    "extend": list.extend,
    "get": _getitem,
    "indexOf": seq_index_of,
    "insert": list_insert,
    "removeAll": list_remove_all,
    "retainAll": list_retain_all,
    "set": list_set,
    "setAny": _setitem,
    "subList": list_sublist,
}
_jpype._methods[PyMemoryView] = {
    "getSlice": _getitem_range,
    "release": memoryview.release,
    "isReadOnly": memoryview_is_read_only,
    "getFormat": memoryview_get_format,
    "getShape": memoryview_get_shape,
    "getStrides": memoryview_get_strides,
    "getSubOffsets": memoryview_get_sub_offsets,
    "getBuffer": memoryview_get_buffer,
}
_jpype._methods[PyMemoryView] = {}
_jpype._methods[PyObject] = {
    "hashCode": hash,
    "equals": _equals,
    "toString": str,
}
_jpype._methods[PyRange] = {
    "getStart": range_get_start,
    "getStop": range_get_stop,
    "getStep": range_get_step,
    "getLength": len,
    "getItem": _getitem,
    "getSlice": range_get_slice,
    "contains": _contains,
    "toList": list,
}
_jpype._methods[PySet] = {
    "add": set_add,
    "addAny": set.add,
    "clear": set.clear,
    "contains": _contains,
    "copy": set.copy,
    "difference": set_difference,
    "differenceUpdate": set_difference_update,
    "discard": set.discard,
    "equals": _equals,
    "hashCode": hash,
    "intersect": set_intersect,
    "intersectionUpdate": set_intersection_update,
    "isDisjoint": set.isdisjoint,
    "isSubset": set.issubset,
    "isSuperset": set.issuperset,
    "pop": set.pop,
    "size": len,
#    "symmetricDifference": set_symmetric_difference,
#    "symmetricDifferenceUpdate": set.symmetric_difference,
    "toList": list,
    "union": set_union,
    "unionUpdate": set_union_update,
    "update": set.update,
}
_jpype._methods[PySlice] = {
    "getStart": slice_get_start,
    "getStop": slice_get_stop,
    "getStep": slice_get_step,
    "indices": slice_indices,
    "isValid": slice_is_valid,
}
_jpype._methods[PyString] = {
    "charAt": _getitem,
    "containsSubstring": _contains,
    "countOccurrences": str_count_occurrences,
    "endsWithSuffix": str_ends_with_suffix,
    "expandTabs": str.expandtabs,
    "findLastSubstring": str_find_last_substring,
    "findSubstring": str_find_substring,
    "formatUsingMapping": str.format_map,
    "formatWith": str_format_with,
    "getCharacterAt": _getitem,
    "indexOfLastSubstring": str_index_of_last_substring,
    "indexOfSubstring": str_index_of_substring,
    "isAlphabetic": str.isalpha,
    "isAlphanumeric": str.isalnum,
    "isAsciiCharacters": str.isascii,
    "isDecimalNumber": str.isdecimal,
    "isDigitCharacters": str.isdigit,
    "isLowercase": str.islower,
    "isNumericCharacters": str.isnumeric,
    "isPrintableCharacters": str.isprintable,
    "isTitleCase": str.istitle,
    "isUppercase": str.isupper,
    "isValidIdentifier": str.isidentifier,
    "isWhitespace": str.isspace,
    "join": str.join,
    "length": len,
    "removePrefix": str.removeprefix,
    "removeSuffix": str.removesuffix,
    "replaceSubstring": str_replace_substring,
    "splitInto": str_split_into,
    "splitIntoLines": str_split_into_lines,
    "splitIntoPartition": str.partition,
    "splitIntoReverse": str_split_into_reverse,
    "splitIntoReversePartition": str.rpartition,
    "startsWithPrefix": str_starts_with_prefix,
    "stripCharacters": str_strip_characters,
    "stripLeading": str_strip_leading,
    "stripTrailing": str_strip_trailing,
    "stripWhitespace": str.strip,
    "subSequence": _getitem_range,
    "swapCaseCharacters": str.swapcase,
    "toCapitalized": str.capitalize,
    "toCaseFolded": str.casefold,
    "toEncoded": str_to_encoded,
    "toTitleCase": str.title,
    "toUppercase": str.upper,
    "translateUsingMapping": str.translate,
    "zeroFill": str.zfill,
}
_jpype._methods[PyTuple] = {
    "get": _getitem,         
    "indexOf": seq_index_of,
    "subList": _getitem_range,   
}
_jpype._methods[PyTuple] = {}
_jpype._methods[PyType] = {
    "getName": type_get_name, 
    "mro": type_mro,         
    "getBase": type_get_base, 
    "getBases": type_get_bases,
    "isSubclassOf": type_is_subclass_of,
    "isInstance": type_is_instance, 
    "getMethod": type_get_method, 
    "isAbstract": type_is_abstract, 
    "getSubclasses": type_get_subclasses, 
}
_jpype._methods[PyZip] = {
    "toList": list,
}

_jpype._methods[PyAbstractSet] ={} # No Java methods
_jpype._methods[PyAwaitable] ={} # No Java methods
_jpype._methods[PyBuffer] ={} # No Java methods
_jpype._methods[PyCallable] ={} # No Java methods
_jpype._methods[PyCollection] ={} # No Java methods
_jpype._methods[PyContainer] ={} # No Java methods
_jpype._methods[PyCoroutine] ={} # No Java methods
_jpype._methods[PyGenerator] ={
    "iter": iter,
}
_jpype._methods[PyIndex] ={} # No Java methods
_jpype._methods[PyIter] = {
    "filter": iter_filter,
}
_jpype._methods[PyIterable] = {
    "allMatch": all,
    "anyMatch": any,
    "mapElements": iterable_map_elements,
    "findMax": max,
    "findMin": min,
    "getSorted": sorted,
    "computeSum": sum,
    "iter": iter,
}
_jpype._methods[PyMapping] = {
    "containsValue": mapping_contains_value,
    "containsKey": _contains,
    "putAll": mapping_put_all, 
    "remove": mapping_remove,  
}
_jpype._methods[PyMutableSet] ={} # No Java methods
_jpype._methods[PyNumber] = {
    "add": number_add,
    "addInPlace": number_add_in_place,
    "subtract": number_subtract,
    "subtractInPlace": number_subtract_in_place,
    "multiply": number_multiply,
    "multiplyInPlace": number_multiply_in_place,
    "divide": number_divide,
    "divideInPlace": number_divide_in_place,
    "floorDivide": number_floor_divide,
    "modulus": number_modulus,
    "power": number_power,
    "negate": number_negate,
    "abs": abs,
    "positive": number_positive,
    "toBoolean": bool,
    "toDouble": float,
    "toInteger": int,
    "compareTo": number_compare_to,
}
_jpype._methods[PySequence] = {
    "remove": _delitem_return,
    "set": _setitem_return,
    "setAny": _setitem,
}
_jpype._methods[PySized] ={} # No Java methods

###################################
# Testing probe API
#_jpype.probe(object)
#_jpype.probe(slice)
print(_jpype.probe(list))
_jpype.probe(tuple)
_jpype.probe(set)
_jpype.probe({})
_jpype.probe(weakref.WeakKeyDictionary)
#_jpype.probe(iter([]))
#_jpype.probe(enumerate)
_jpype.probe(np.array([]))
_jpype.probe(int)
_jpype.probe(np.int32)
_jpype.probe(float)
_jpype.probe(np.float32)
