# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
import _jpype
from . import _jclass
from . import _jproxy
from . import types as _jtypes
from . import _jcustomizer
from collections.abc import Mapping, Sequence, MutableSequence
import itertools
from typing import MutableMapping, Callable, List

__all__: List[str] = []

JImplements = _jproxy.JImplements
JProxy = _jproxy.JProxy
JOverride = _jclass.JOverride
JConversion = _jcustomizer.JConversion
JClass = _jclass.JClass

###################################################################################
# Set up methods binds from Java to Python

def _isTrue(x):
    return x==True

def _not(x):
    return not x

def _setitem(x, k, v):
    x[k] = v

def _getitem(x, k):
    return x[k]

def _setitem_str(x, k, v):
    x[str(k)] = v

def _getitem_str(x, k):
    return x[str(k)]

def _delitem(x, k):
    del x[k]

def _identity(x):
    return x

def _newdict(args):
    out = dict()
    for p,v in args.entrySet():
        out[p]=v
    return out

def _asfunc(x):
    if hasattr(x,__call__):
        return x
    return None

def _hasattr(x,k):
    return hasattr(x, str(k))

def _getattr(x,k):
    return getattr(x, str(k))

def _setattr(x,k,v):
    setattr(x, str(k), v)

def _delattr(x,k):
    delattr(x, str(k))

def _getdict(x):
    return x.__dict__

def _isinstance(x, args):
    return isinstance(x, tuple(args))

def _contains(x, v):
    return v in x

def _putall(x, m):
    for p,v in m.entrySet():
        x[p] = v

def _containsvalue(x, v):
    return v in x.values()

def _clear(x):
    x.clear()

def _isempty(x):
    return len(x) == 0

def _keyset(x):
    return JClass("org.jpype.bridge.KeySet")(x)

def _add(x,v):
    return x+v

def _sub(x,v):
    return x-v

def _mult(x,v):
    return x*v

def _div(x,v):
    return x/v

def _addassign(x,v):
    x += v
    return x

def _subassign(x,v):
    x -= v
    return x

def _multassign(x,v):
    x *= v
    return x

def _divassign(x,v):
    x /= v
    return x

def _matmul(x,v):
    return x@v

def _divmod(x,v):
    return x//v

def _pow(x,v):
    return x**v

def _remainder(x,v):
    return x%v

def _call(x, v, k):
    if k is None:
        return x(*v)
    return x(*v, **k)

def _range(*args):
    return range(*args)

def _map(x,f):
    return map(f,x)

def  _set_union(x, args):
    return x.union(*tuple(args))

def  _set_intersect(x, args):
    return x.intersect(*tuple(args))

def  _set_difference(x, args):
    return x.difference(*tuple(args))

def  _set_symmetric_difference(x, args):
    return x.symmetric_difference(*tuple(args))

def _set_add(s, v):
    if v in s:
        return False
    s.add(v)
    return True

def _set_contains(s, v):
    return v in s

def _equals(x,y):
    return x == y

def _filter(x, f):
    return filter(f,x)

def _charat(x,y):
    return x[y]

def _str_subseq(x,s,e):
    return x[s:e]

def _c_add(x, *args):
    if len(args)==1:
        x.append(args[0])
        return True
    x.insert(args[0], args[1])
    return None

def _indexof(x, v):
    try:
        return x.index(v)
    except ValueError:
        return -1

def _c_set(x, i, v):
    if i<0:
        raise ValueError()
    out = x[i]
    x[i] = v
    return out

def _removeall(x, c):
    c = set(c)
    nl = [i for i in x if not i in c]
    x.clear()
    x.extend(nl)

def _retainall(x, c):
    c = set(c)
    nl = [i for i in x if i in c]
    x.clear()
    x.extend(nl)

def _real(x):
    return x.real

def _imag(x):
    return x.imag

def _items(x):
    return x.items()

def _values(x):
    return x.values()

def _sublist(x, s, e):
    return x[s:e]

def _start(x):
    return x.start

def _end(x):
    return x.end

def _step(x):
    return x.step


_PyJPBackendMethods: MutableMapping[str, Callable] = {
    "bytearray": bytearray,
    "bytearray_fromhex": bytearray.fromhex,
    "bytes": bytes,
    "bytes_fromhex": bytes.fromhex,
    "call": _call,
    "complex": complex,
    "delattr": _delattr,
    "dict": dict,
    "dir": dir,
    "enumerate": enumerate,
    "eval": eval,
    "exec": exec,
    "getDict": _getdict,
    "getattr": _getattr,
    "getitem": _getitem_str,
    "hasattr": _hasattr,
    "isinstance": _isinstance,
    "iter": iter,
    "list": list,
    "memoryview": memoryview,
    "newDict": _newdict,
    "newSet": set,
    "newTuple": tuple,
    "next": next,
    "object": object,
    "range": _range,
    "repr": repr,
    "set": set,
    "setattr": _setattr,
    "setitem": _setitem_str,
    "slice": slice,
    "str": str,
    "tee": itertools.tee,
    "tuple": tuple,
    "type": type,
    "zip": zip,
    "items": _items,
    "values": _values,
}


# Now we need to set up the dictionary for the proxy object
#  This seems trivial but the name binds the Java interface to existing functions
_PyObjectMethods: MutableMapping[str, Callable] = { 
    "getType": type,
    "isInstance": isinstance,
    "asAttributes": _identity,
    "asSequence": _identity,
    "asFunc": _asfunc,
    "toInt": _identity,
    "toFloat": _identity,
    "hashCode": hash,
    "equals": _equals,
}

# Protocols

_PyAttributesMethods: MutableMapping[str, Callable]= { 
    "asObject": _identity,
    "get": _getattr,
    "set": _setattr,
    "remove": _delattr,
    "has": _hasattr,
    "dir": dir,
    "dict": _getdict
}

_PyCallableMethods: MutableMapping[str, Callable] = { 
    "asObject": _identity,
}


_PyMappingMethods: MutableMapping[str, Callable] = { 
    "asObject": _identity,
    "clear": _clear,
    "containsKey": _contains,
    "containsValue": _containsvalue,
    "get": _getitem,
    "put": _c_set,
    "remove": _delitem,
    "putAll": _putall,
}

_PyNumberMethods: MutableMapping[str, Callable] = {
    "asObject": _identity,
    "toInt": int,
    "toFloat": float,
    "toBool": bool,
    "not": _not,
    "add": _add,
    "sub": _sub,
    "mult": _mult,
    "div": _div,
    "addAssign": _addassign,
    "subAssign": _subassign,
    "multAssign": _multassign,
    "divAssign": _divassign,
    "matMult": _matmul,
    "divMod": _divmod,
    "pow": _pow,
    "remainder": _remainder,
}

_PySequenceMethods = {
    "asObject": _identity,
    "get": _getitem,
    "set": _setitem,
    "remove": _delitem,
    "size": len,
}

_PyIterableMethods: MutableMapping[str, Callable] = {
    "any": any,
    "all": all,
    "iter": iter,
    "map": _map,
    "min": min,
    "max": max,
    "reversed": reversed,
    "sorted": sorted,
    "sum": sum,
}
_PyIterableMethods.update(_PyObjectMethods)

_PyIterMethods: MutableMapping[str, Callable] = {
    "filter": _filter,
    "next": next,
}
_PyIterMethods.update(_PyObjectMethods)

## Concrete types

_PyBytesMethods: MutableMapping[str, Callable] = {
    "decode": bytes.decode,
    "translate": bytes.translate,
}
_PyBytesMethods.update(_PyObjectMethods)

_PyByteArrayMethods: MutableMapping[str, Callable] = {
    "decode": bytearray.decode,
    "translate": bytearray.translate,
}
_PyByteArrayMethods.update(_PyObjectMethods)

_PyComplexMethods: MutableMapping[str, Callable] = {
    "real": _real,
    "imag": _imag,
    "conjugate": complex.conjugate
}
_PyComplexMethods.update(_PyObjectMethods)
_PyComplexMethods.update(_PyNumberMethods)

_PyDictMethods: MutableMapping[str, Callable] = {
    "clear": _clear,
    "containsKey": _contains,
    "containsValue": _containsvalue,
    "get": _getitem,
    "put": _c_set,
    "remove": _delitem,
    "putAll": _putall,
}
_PyDictMethods.update(_PyObjectMethods)

_PyExcMethods: MutableMapping[str, Callable] = {
    "getMessage": str,
}
_PyExcMethods.update(_PyObjectMethods)

# enumerate, zip, range
_PyGeneratorMethods: MutableMapping[str, Callable] = {
    "iter": iter
}
_PyGeneratorMethods.update(_PyObjectMethods)

        

_PyListMethods: MutableMapping[str, Callable] = {
    "add": _c_add,
    "addAny": _c_add,
    "clear": list.clear,
    "contains": _contains,
    "extend": list.extend,
    "get": _getitem,
    "indexOf": _indexof,
    "insert": list.insert,
    "removeAll": _removeall,
    "retainAll": _retainall,
    "set": _c_set,
    "setAny": _setitem,
    "size": len,
    "subList": _sublist,
}
_PyListMethods.update(_PyIterableMethods)

_PyMemoryViewMethods: MutableMapping[str, Callable] = {}
_PyMemoryViewMethods.update(_PyObjectMethods)

_PySetMethods = {
    "add": _set_add,
    "clear": set.clear,
    "contains": _set_contains,
    "copy": set.copy,
    "difference": _set_difference,
    "discard": set.discard,
    "intersect": _set_intersect,
    "isDisjoint": set.isdisjoint,
    "isSubset": set.issubset,
    "isSuperset": set.issuperset,
    "size": len,
    "pop": set.pop,
    "symmetricDifference": _set_symmetric_difference,
    "union": _set_union,
    "update": set.update,
}
_PySetMethods.update(_PyObjectMethods)

_PySliceMethods: MutableMapping[str, Callable] = {
    "start": _start,
    "end": _end,
    "step": _step,
    "indices": slice.indices
}
_PySliceMethods.update(_PyObjectMethods)

_PyStringMethods: MutableMapping[str, Callable] = {
    "charAt": _charat,
    "length": len,
    "subSequence": _str_subseq,
    "capitalize":     str.capitalize,
    "casefold":       str.casefold,
    "center":         str.center,
    "count":          str.count,
    "encode":         str.encode,
    "endsWith":       str.endswith,
    "expandTabs":     str.expandtabs,
    "find":           str.find,
    "format":         str.format,
    "formatMap":      str.format_map,
    "index":          str.index,
    "isAlnum":        str.isalnum,
    "isAlpha":        str.isalpha,
    "isAscii":        str.isascii,
    "isDecimal":      str.isdecimal,
    "isDigit":        str.isdigit,
    "isIdentifier":   str.isidentifier,
    "isLower":        str.islower,
    "isNumeric":      str.isnumeric,
    "isPrintable":    str.isprintable,
    "isSpace":        str.isspace,
    "isTitle":        str.istitle,
    "isUpper":        str.isupper,
    "join":           str.join,
    "lstrip":         str.lstrip,
    "partition":      str.partition,
#    "removePrefix":   str.removeprefix,
#    "removeSuffix":   str.removesuffix,
    "replace":        str.replace,
    "rfind":          str.rfind,
    "rindex":         str.rindex,
    "rpartition":     str.rpartition,
    "rsplit":         str.rsplit,
    "split":          str.split,
    "splitLines":     str.splitlines,
    "startsWith":     str.startswith,
    "swapCase":       str.swapcase,
    "title":          str.title,
    "translate":      str.translate,
    "upper":          str.upper,
    "zfill":          str.zfill,
}
_PyStringMethods.update(_PyObjectMethods)

_PyTupleMethods = {
    "contains": _contains,
    "get": _getitem,
    "indexOf": _indexof,
    "size": len,
    "subList": _sublist,
}
_PyTupleMethods.update(_PyIterableMethods)

_PyTypeMethods = {
    "mro": type.mro,
}
_PyTypeMethods.update(_PyObjectMethods)


def initialize():
    # Install the handler
    bridge = JClass("org.jpype.bridge.Interpreter").getInstance()
    Backend = JClass("org.jpype.bridge.Backend")
    backend = Backend@JProxy(Backend, dict=_PyJPBackendMethods)

    ###################################################################################
    # Name our types into local scope

    # Concrete types
    _PyBytes = JClass("python.lang.PyBytes")
    _PyByteArray = JClass("python.lang.PyByteArray")
    _PyComplex = JClass("python.lang.PyComplex")
    _PyDict = JClass("python.lang.PyDict")
    _PyEnumerate = JClass("python.lang.PyEnumerate")
    _PyJavaObject = JClass("python.lang.PyJavaObject")
    _PyList = JClass("python.lang.PyList")
    _PyMemoryView = JClass("python.lang.PyMemoryView")
    _PyObject = JClass("python.lang.PyObject")
    _PyRange = JClass("python.lang.PyRange")
    _PySet = JClass("python.lang.PySet")
    _PySlice = JClass("python.lang.PySlice")
    _PyString = JClass("python.lang.PyString")
    _PyTuple = JClass("python.lang.PyTuple")
    _PyType = JClass("python.lang.PyType")
    _PyZip = JClass("python.lang.PyZip")
    _PyExc = JClass("python.exception.PyExc")

    # Protocols
    _PyAttributes = JClass("python.protocol.PyAttributes")
    _PyCallable = JClass("python.protocol.PyCallable")
    _PyGenerator = JClass("python.protocol.PyGenerator")
    _PyIterable = JClass("python.protocol.PyIterable")
    _PyIter = JClass("python.protocol.PyIter")
    _PyMapping = JClass("python.protocol.PyMapping")
    _PyNumber = JClass("python.protocol.PyNumber")
    _PySequence = JClass("python.protocol.PySequence")

    ###################################################################################
    # Bind the method tables

    # Define the method tables for each type here
    _PyBytes._methods = _PyBytesMethods
    _PyByteArray._methods = _PyByteArrayMethods
    _PyDict._methods = _PyDictMethods
    _PyEnumerate._methods = _PyGeneratorMethods
    _PyGenerator._methods = _PyGeneratorMethods
    _PyIterable._methods = _PyIterableMethods
    _PyIter._methods = _PyIterMethods
    _PyList._methods = _PyListMethods
    _PyMemoryView._methods = _PyMemoryViewMethods
    _PyObject._methods = _PyObjectMethods
    _PyRange._methods = _PyGeneratorMethods
    _PySet._methods = _PySetMethods
    _PySlice._methods = _PySliceMethods
    _PyString._methods = _PyStringMethods
    _PyTuple._methods = _PyTupleMethods
    _PyType._methods = _PyTypeMethods
    _PyZip._methods = _PyGeneratorMethods

    _PyAttributes._methods = _PyAttributesMethods
    _PyCallable._methods = _PyCallableMethods
    _PyMapping._methods = _PyMappingMethods
    _PyNumber._methods = _PyNumberMethods
    _PySequence._methods = _PySequenceMethods


    ###################################################################################
    # Construct conversions between concrete types and protocols.
    #
    # This is a trivial task as we have JProxy which can add an interface to any Python
    # object.  We just need to make it tag in such a way that it automatically unwraps
    # back to Python when passed from Java.

    # Bind the concrete types
    @JConversion(_PyBytes, instanceof=bytes)
    @JConversion(_PyByteArray, instanceof=bytearray)
    @JConversion(_PyComplex, instanceof=complex)
    @JConversion(_PyDict, instanceof=dict)
    @JConversion(_PyEnumerate, instanceof=enumerate)
    @JConversion(_PyExc, instanceof=BaseException)
    @JConversion(_PyList, instanceof=list)
    @JConversion(_PyMemoryView, instanceof=memoryview)
    @JConversion(_PyRange, instanceof=range)
    @JConversion(_PySet, instanceof=set)
    @JConversion(_PySlice, instanceof=slice)
    @JConversion(_PyString, instanceof=str)
    @JConversion(_PyTuple, instanceof=tuple)
    @JConversion(_PyType, instanceof=type)
    @JConversion(_PyZip, instanceof=zip)
    # Bind dunder
    @JConversion(_PyIterable, attribute="__iter__")
    @JConversion(_PyIter, attribute="__next__")
    # Bind the protocols
    @JConversion(_PyAttributes, instanceof=object)
    @JConversion(_PyCallable, instanceof=object)
    @JConversion(_PyMapping, instanceof=object)
    @JConversion(_PyNumber, instanceof=object)
    @JConversion(_PySequence, instanceof=object)
    def _jconvert(jcls, obj):
        return JProxy(jcls, dict=jcls._methods, inst=obj, convert=True)

    # Create a dispatch which will bind Python concrete types to Java.
    # Most of the time people won't see them, but we can add Java interfaces to 
    # make those objects become Java like.
    _conversionDispatch = {
        bytes: _PyBytes,
        dict: _PyDict,
        list: _PyList,
        memoryview: _PyMemoryView,
        str:  _PyString,
        tuple: _PyTuple,
        type: _PyType,
    }

    # Next we bind the dispatch to the Java types using the dispatch
    @JConversion(_PyObject, instanceof=object)
    def _pyobject(jcls, obj):
        if isinstance(obj, _jpype._JObject):
            return _PyJavaObject(obj)
        # See if there is a more advanced wrapper we can apply
        if len(obj.__class__.__mro__) > 1:
            jcls = _conversionDispatch.get(obj.__class__.__mro__[-2], jcls)
        return _jconvert(jcls, obj)

    bridge.setBackend(backend)

    @JConversion(_PyObject, instanceof=_jpype._JObject)
    def _pyobject(jcls, obj):
        return _PyJavaObject(obj)
