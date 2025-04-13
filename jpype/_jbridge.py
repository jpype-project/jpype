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

__all__ = []

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

def _call(x, a, k):
    return x(a,k)

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

def _values(x):
    raise RuntimeError()

def _entries(x):
    raise RuntimeError()

def _not(x):
    return not x

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


_PyJPBackendMethods = {
    "bytearray": bytearray,
    "bytearray_fromhex": bytearray.fromhex,
    "bytes": bytes,
    "bytes_fromhex": bytes.fromhex,
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
}


# Now we need to set up the dictionary for the proxy object
#  This seems trivial but the name binds the Java interface to existing functions
_PyObjectMethods = { 
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

_PyAttributesMethods = { 
    "asObject": _identity,
    "get": _getattr,
    "set": _setattr,
    "del": _delattr,
    "has": _hasattr,
    "dir": dir,
    "dict": _getdict
}

# Protocols

_PyAttributesMethods = { 
    "asObject": _identity,
    "get": _getattr,
    "set": _setattr,
    "del": _delattr,
    "has": _hasattr,
    "dir": dir,
    "dict": _getdict
}

_PyCallableMethods = { 
    "asObject": _identity,
    "_call": _call,
}


_PyMappingMethods = { 
    "asObject": _identity,
    "get": _getitem,
    "put": _setitem,
    "remove": _delitem,
    "containsKey": _contains,
    "containsValue": _containsvalue,
    "putAll": _putall,
    "clear": _clear,
    "isEmpty": _isempty,
    "keySet": _keyset,
    "values": _values,
    "entrySet": _entries,
}

_PyNumberMethods = {
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

_PyIterableMethods = {
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

_PyIteratorMethods = {
    "filter": _filter,
    "next": next,
}
_PyIterableMethods.update(_PyObjectMethods)

## Concrete types

_PyBytesMethods = {
    "decode": bytes.decode,
    "translate": bytes.translate,
}
_PyBytesMethods.update(_PyObjectMethods)

_PyByteArrayMethods = {
    "decode": bytearray.decode,
    "translate": bytearray.translate,
}
_PyByteArrayMethods.update(_PyObjectMethods)

_PyComplexMethods = {
    "conjugate": complex.conjugate
}
_PyComplexMethods.update(_PyObjectMethods)
_PyComplexMethods.update(_PyNumberMethods)

_PyDictMethods = {}
_PyDictMethods.update(_PyObjectMethods)

# enumerate, zip, range
_PyGeneratorMethods = {
    "iter": iter
}
_PyGeneratorMethods.update(_PyObjectMethods)

_PyListMethods = {}
_PyListMethods.update(_PyIterableMethods)

_PyMemoryViewMethods = {}
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

_PySliceMethods = {
    "indices": slice.indices
}
_PySliceMethods.update(_PyObjectMethods)

_PyStringMethods = {
    "charAt": _charat,
    "length": len,
    "subSequence": _str_subseq,
}
_PyStringMethods.update(_PyObjectMethods)

_PyTupleMethods = {}
_PyTupleMethods.update(_PyIterableMethods)

_PyTypeMethods = {}
_PyTypeMethods.update(_PyObjectMethods)


def initialize():
    # Install the handler
    bridge = JClass("org.jpype.bridge.Bridge").getInstance()
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
    _PyGenerator = JClass("python.lang.PyGenerator")
    _PyIterable = JClass("python.lang.PyIterable")
    _PyIterator = JClass("python.lang.PyIterator")
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

    # Protocolos
    _PyAttributes = JClass("python.protocol.PyAttributes")
    _PyCallable = JClass("python.protocol.PyCallable")
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
    _PyIterator._methods = _PyIteratorMethods
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
    @JConversion(_PyIterator, attribute="__next__")
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

