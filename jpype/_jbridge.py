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
from . import _jpackage
from . import _jproxy
from . import types as _jtypes
from . import _jcustomizer
from collections.abc import Mapping, Sequence, MutableSequence
import itertools
import inspect
import functools
import types
import importlib
from typing import MutableMapping, Callable, List
import builtins

__all__: List[str] = []


JImplements = _jproxy.JImplements
JProxy = _jproxy.JProxy
JOverride = _jclass.JOverride
JConversion = _jcustomizer.JConversion
JClass = _jclass.JClass
JString = _jpype.JString
JPackage = _jpackage.JPackage

###################################################################################
# Set up methods binds from Java to Python

# DO NOT INTRODUCE NEW MEMBERS THAT DO THE SAME THING
# USE _attr for simple member access
# FOLLOW THE NAMING SCHEME
# USE A DIRECT BUILTIN IF IT HAS THE RIGHT LOGIC


# SPI Installer callbacks (see plan/SPI.md). Module-level, not nested in
# initialize(), so _core.py's lazy _cache hook can call
# _installer_register_class directly the first time a lazily-registered
# type is actually seen, without needing a closure captured at init time.

def _installer_register_class(pyModule, pyClass, javaInterface, methodsSource):
    ns: MutableMapping[str, object] = {}
    exec(str(methodsSource), ns)
    methods = ns["METHODS"]
    mod = importlib.import_module(str(pyModule))
    pyType = getattr(mod, str(pyClass))
    iface = JClass(str(javaInterface))
    _jpype._concrete[pyType] = iface
    _jpype._methods[iface] = methods


def _installer_register_lazy_class(pyModule, pyClass, javaInterface, methodsSource):
    # Only stashes the ingredients - no import, no exec. Replayed later by
    # _core.py's _LazyCache the first time a matching type is actually seen.
    _jpype._lazy_pending.setdefault(str(pyModule), {})[str(pyClass)] = \
        (str(javaInterface), str(methodsSource))


def initialize():
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
    _PyExc = JClass("python.lang.PyExc")
    _PyInt = JClass("python.lang.PyInt")
    _PyFloat = JClass("python.lang.PyFloat")
    _PyFrozenSet = JClass("python.lang.PyFrozenSet")
    _PyEllipsis = JClass("python.lang.PyEllipsis")
    _PyNotImplemented = JClass("python.lang.PyNotImplemented")
    # types.FunctionType covers both `def` and `lambda` in CPython - there is
    # no separate runtime type for a lambda to key PyLambda off of, so only
    # PyFunction is concretely registered; PyLambda is left unregistered.
    _PyFunction = JClass("python.lang.PyFunction")
    _PyFilter = JClass("python.lang.PyFilter")

    # Protocols
    _PyCallable = JClass("python.lang.PyCallable")
    _PyGenerator = JClass("python.lang.PyGenerator")
    _PyIterable = JClass("python.lang.PyIterable")
    _PyIter = JClass("python.lang.PyIter")
    _PyMapping = JClass("python.lang.PyMapping")
    _PyNumber = JClass("python.lang.PyNumber")
    _PySequence = JClass("python.lang.PySequence")
    _PyAbstractSet = JClass("python.lang.PyAbstractSet")
    _PySized = JClass("python.lang.PySized")
    _PyAwaitable = JClass("python.lang.PyAwaitable")
    _PyBuffer = JClass("python.lang.PyBuffer")
    _PyCollection = JClass("python.lang.PyCollection")
    _PyContainer = JClass("python.lang.PyContainer")
    _PyCoroutine = JClass("python.lang.PyCoroutine")
    _PyIndex = JClass("python.lang.PyIndex")
    _PySubscript = JClass("python.lang.PySubscript")
    _PyMutableSet = JClass("python.lang.PyMutableSet")
    _PyCombinable = JClass("python.lang.PyCombinable")
    _RuntimeException = JClass("java.lang.RuntimeException")

    # Attribute helpers
    def _attr(name):
        return lambda x: getattr(x, name)

    def _delitem(x, i):
        del x[i]

    def _setitem(x, s, v):
        x[s] = v

    def _setitem_str(x, s, v):
        x[str(s)] = v

    def _asfunc(x):
        if hasattr(x,__call__):
            return x
        return None

    # Flips
    def _map(x,f):
        return map(f,x)

    # Starred
    def _new_list_from_array(*elements):
        return list(elements)

    def _range(*args):
        return range(*args)

    # Capture return
    def _delattr_return(x, key):
        key = str(key)
        out = getattr(x, key)
        delattr(x, key)
        return out

    def _delitem_return(x, k):
        out = x[k]
        del x[k]
        return out

    def _setitem_return(x, k, v):
        out = x[k]
        x[k] = v
        return out

    def _setitem_from_object(x, key, value):
        old = None
        try:
            old = x[key]
        except Exception:
            old = None
        x[key] = value
        return old


    ######################################################

    def _call(x, v, k):
        if k is None:
            return x(*v)
        return x(*v, **k)

    def _get_signature(x):
        return str(inspect.signature(x))

    def _is_callable(x):
        return callable(x)

    def _isinstance(x, args):
        try:
            return isinstance(x, tuple(args))
        except TypeError:
            return isinstance(x, args)

    def _tee_iterator(iterator):
        import itertools
        a, b = itertools.tee(iterator)
        return a

    def _unwrap_optional_int(v):
        # Java always passes exactly 3 arguments to backend.slice(), so a
        # caller-supplied null doesn't hit this function's own start=None
        # default - it arrives as a boxed java.lang.Integer proxy whose
        # underlying reference is null. That proxy reprs as "None" but is
        # not the real Py_None singleton, so `is None` never matches it, and
        # slice's own C-level indices() computation misbehaves if it's
        # stored as-is (int() conversion on it raises instead of producing
        # a value). Convert eagerly here so slice.start/.stop/.step always
        # hold either a real int or real None.
        if v is None:
            return None
        try:
            return int(v)
        except TypeError:
            return None

    def _slice_dispatch(start=None, stop=None, step=None):
        return slice(_unwrap_optional_int(start), _unwrap_optional_int(stop), _unwrap_optional_int(step))

    def _next_with_stop(iterator, stop):
        return next(iterator, stop)

    def _mapping_contains_all_values(obj, c):
        values = list(obj.values())
        return all(v in values for v in c)

    def _mapping_remove_all_keys(obj, collection):
        removed = False
        for key in list(collection):
            if key in obj:
                del obj[key]
                removed = True
        return removed

    def _mapping_remove_all_values(obj, collection):
        targets = set(collection)
        to_remove = [k for k, v in obj.items() if v in targets]
        for k in to_remove:
            del obj[k]
        return bool(to_remove)

    def _mapping_remove_value(obj, value):
        to_remove = [k for k, v in obj.items() if v == value]
        for k in to_remove:
            del obj[k]
        return bool(to_remove)

    def _mapping_retain_all_keys(obj, collection):
        keep = set(collection)
        to_remove = [k for k in list(obj.keys()) if k not in keep]
        for k in to_remove:
            del obj[k]
        return bool(to_remove)

    def _mapping_retain_all_values(obj, collection):
        keep = set(collection)
        to_remove = [k for k, v in obj.items() if v not in keep]
        for k in to_remove:
            del obj[k]
        return bool(to_remove)

    def _type(x):
        return _jpype.pyobject(_PyType,type(x))
        return _PyType@type(x)

    _PyJPBackendMethods: MutableMapping[str, Callable] = {
        # Core constructors / builtins
        "asDouble": float,
        "asLong": int,
        "bytearray": bytearray,
        "bytearrayFromHex": lambda s: bytearray.fromhex(str(s)),
        "bytes": bytes,
        "bytesFromHex": lambda s: bytes.fromhex(str(s)),
        "call": _call,
        "contains": lambda x,v: v in x,
        "delitemByIndex": _delitem,
        "delitemByObject": _delitem,
        "delattrReturn": _delattr_return,
        "popItem": lambda x: x.popitem(),
        "delattrString": lambda x,s: delattr(x, str(s)),
        "dir": dir,
        "enumerate": enumerate,
        "eval": lambda s,g,l: eval(str(s),g,l),
        "exec": lambda s,g,l: exec(str(s),g,l),
        "getDict": _attr("__dict__"),
        "getDocString": lambda x: getattr(x, "__doc__", None),
        "getSignature": _get_signature,
        "getattrDefault": lambda x,s,d: getattr(x, str(s), d),
        "getattr": lambda x,s: getattr(x, str(s)),
        "getitemMappingObject": lambda x,i: x[i],
        "getitemMappingString": lambda x,s: x[str(s)],
        "getitemSequence": lambda x,i: x[i],
        "hasattrString": lambda x,s: hasattr(x, str(s)),
        "isCallable": _is_callable,
        "isinstanceFromArray": _isinstance,
        "items": lambda x: x.items(),
        "iter": iter,
        "iterSet": iter,
        "iterMap": iter,
        "keys": lambda x: x.keys(),
        "len": len,
        "list": list,
        "mappingClear": lambda x: x.clear(),
        "mappingContainsAllValues": _mapping_contains_all_values,
        "mappingContainsValue": lambda x,v: v in x.values(),
        "mappingRemoveAllKeys": _mapping_remove_all_keys,
        "mappingRemoveAllValue": _mapping_remove_all_values,
        "mappingRemoveValue": _mapping_remove_value,
        "mappingRetainAllKeys": _mapping_retain_all_keys,
        "mappingRetainAllValue": _mapping_retain_all_values,
        "memoryview": memoryview,
        "newByteArray": lambda: bytearray(),
        "newByteArrayFromBuffer": bytearray,
        "newByteArrayFromIterable": bytearray,
        "newByteArrayFromIterator": bytearray,
        "newByteArrayOfSize": bytearray,
        "newBytesFromBuffer": bytes,
        "newBytesFromIterator": bytes,
        "newBytesOfSize": bytes,
        "newComplex": lambda r,i: complex(r,i),
        "newDict": lambda *m: dict(*m),
        "newDictFromIterable": dict,
        "newEnumerate": enumerate,
        "newFloat": float,
        "frozenset": frozenset,
        "newFrozenSet": frozenset,
        "newInt": int,
        "newList": lambda: [],
        "newListFromArray": _new_list_from_array,
        "newListFromIterable": list,
        "newSet": lambda: set(),
        "newSetFromIterable": set,
        "newTuple": lambda: tuple(),
        "newTupleFromArray": tuple,
        "newTupleFromIterator": tuple,
        "newZip": lambda x: zip(*x),
        "next": _next_with_stop,
        "object": lambda: object(),
        "range": _range,
        "repr": repr,
        "set": set,
        "setattrReturn": lambda x,s,v: setattr(x, str(s), v),
        "setattrString": lambda x,s,v: setattr(x, str(s), v),
        "setitemFromObject": _setitem_from_object,
        "setitemFromString": _setitem_str,
        "setitemMapping": _setitem,
        "setitemSequence": _setitem_return,
        "slice": _slice_dispatch,
        "str": str,
        "teeIterator": _tee_iterator,
        "type": _type,
        "values": lambda x: x.values(),
        "vars": vars,
        "zipFromArray": lambda x: zip(*x),
        "zipFromIterable": lambda x: zip(*x),
    }


    # FIXME
    #  The mappings must match the Java interface names exactly.
    #  We do not provide bindings of default methods in Java as those are already covered
    #  Some methods are removed from the interfaces as they were not needed.
    #  Some classes were switched to concrete and thus no longer need mappings.

    def _to_string(o):
        return JString(str(o))

    def _to_java_int32(v):
        # Java int is signed 32-bit; fold to unsigned range then wrap into
        # the signed range so values >= 2**31 don't overflow on the Java
        # side (was raising PyOverflowError for large Python hashes).
        v &= 0xFFFFFFFF
        if v >= 0x80000000:
            v -= 0x100000000
        return v

    def _hash(x):
        try:
            h = hash(x)
            return _to_java_int32(h ^ (h >> 32))
        except TypeError:
            # Fallback to id(x) which is basically the pointer address
            ptr = id(x)
            return _to_java_int32(ptr ^ (ptr >> 32))

    def _equals(x,y):
        return x == y

    # equals/hashCode/toString are never mangled, even on a mangle-eligible
    # interface: java.lang.reflect.Proxy special-cases these three and
    # always dispatches them via Object.class's own Method object
    # (ProxyFactory.objectMethods), never through ProxyType.buildDescriptor's
    # per-interface mangled descriptor - see ProxyType.isObjectMethodSignature.
    _PyObjectMethods: MutableMapping[str, Callable] = {
        "hashCode": _hash,
        "equals": _equals,
        "toString": _to_string,
    }

    _PyCallableMethods: MutableMapping[str, Callable] = {}
    _PyCoroutineMethods: MutableMapping[str, Callable] = {}
    _PyAwaitableMethods: MutableMapping[str, Callable] = {}

    ### Number types
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

    _PyIntMethods: MutableMapping[str, Callable] = {}
    _PyFloatMethods: MutableMapping[str, Callable] = {}
    _PyIndexMethods: MutableMapping[str, Callable] = {}
    _PySubscriptMethods: MutableMapping[str, Callable] = {}
    _PyNumberMethods: MutableMapping[str, Callable] = {
        ".add": lambda x, v: x + v,
        ".divide": lambda x, v: x / v,
        ".divideWithRemainder": lambda x, d: x // d,
        ".matrixMultiply": lambda x, v: x @ v,
        ".multiply": lambda x, v: x * v,
        ".negate": lambda x: not x,
        ".power": lambda x, p: x ** p,
        ".modulus": lambda x, v: x % v,
        ".subtract": lambda x, v: x - v,
        ".toBoolean": bool,
        ".toDouble": float,
        ".toInteger": int,
        ".abs": abs,
        ".negateValue": lambda x: -x,
        ".positive": lambda x: +x,
        ".floorDivide": lambda x, v: x // v,
        ".compareTo": lambda x, y: -1 if x < y else (1 if x > y else 0),
        ".addInPlace": _addassign,
        ".divideInPlace": _divassign,
        ".multiplyInPlace": _multassign,
        ".subtractInPlace": _subassign,
    }

    _PyComplexMethods: MutableMapping[str, Callable] = {
        ".real": lambda x: x.real,
        ".imag": lambda x: x.imag,
        ".conjugate": complex.conjugate
    }


    def _getMessage(x):
        return str(x.args[0]) if x.args else str(x)

    ### Concrete types
    _PyExcMethods: MutableMapping[str, Callable] = {
        ".getMessage": _getMessage,
    }

    _PySliceMethods: MutableMapping[str, Callable] = {
        ".getStart": _attr("start"),
        ".getStop": _attr("stop"),
        ".getStep": _attr("step"),
        ".indices": lambda x, length: x.indices(int(length)),
        ".isValid": lambda x: x.step !=0,
    }


    def _type_is_instance(x, obj):
        return isinstance(obj, x)

    def _type_get_method(x, name):
        return getattr(x, str(name), None)

    _PyTypeMethods: MutableMapping[str, Callable] = {
        ".getName": _attr("__name__"),
        ".mro": type.mro,
        ".getBase": lambda x: getattr(x, "__base__", ()),
        ".getBases": lambda x: getattr(x, "__bases__", None),
        ".isSubclassOf": lambda x,t: issubclass(x,t),
        ".isInstance": _type_is_instance,
        ".getMethod": _type_get_method,
        ".isAbstract": inspect.isabstract,
        ".getSubclasses": lambda x: x.__subclasses__(),
    }


    ### Memory like
    def _bytes_decode(b, encoding='utf-8', errors='strict'):
        # Convert Java String/PyObject to Python str
        return bytes.decode(b, str(encoding) if encoding else 'utf-8', str(errors) if errors else 'strict')

    def _bytearray_decode(b, encoding='utf-8', errors='strict'):
        # Convert Java String/PyObject to Python str
        return bytearray.decode(b, str(encoding) if encoding else 'utf-8', str(errors) if errors else 'strict')

    _PyBufferMethods: MutableMapping[str, Callable] = {}
    _PyBytesMethods: MutableMapping[str, Callable] = {
        ".decode": _bytes_decode,
        ".translate": bytes.translate,
    }

    _PyByteArrayMethods: MutableMapping[str, Callable] = {
        ".decode": _bytearray_decode,
        ".translate": bytearray.translate,
    }

    _PyMemoryViewMethods: MutableMapping[str, Callable] = {
        ".getBuffer": _attr("obj"),
        ".getFormat": _attr("format"),
        ".getShape": _attr("shape"),
        ".getSlice": lambda x,s,e: x[s:e],
        ".getStrides": _attr("strides"),
        ".getSubOffsets": _attr("suboffsets"),
        ".isReadOnly": _attr("readonly"),
        ".release": memoryview.release,
    }

    # python.io (and every other SPI provider) is no longer hand-wired here
    # at all - see the Installer implementation below (_installer_register_*)
    # and the .pyspi resources under native/.../resources/python/io/spi/.

    ### String
    def _count_occurrences(x, sub, start=None, end=None):
        if start is None and end is None:
            return x.count(str(sub))
        if end is None:
            return x.count(str(sub), start)
        return x.count(str(sub), start, end)

    def _ends_with_suffix(x, suffix, start=None, end=None):
        suffix = str(suffix)
        if start is None and end is None:
            return x.endswith(suffix)
        if end is None:
            return x.endswith(suffix, start)
        return x.endswith(suffix, start, end)

    def _find_last_substring(x, sub, start=None, end=None):
        sub = str(sub)
        if start is None and end is None:
            return x.rfind(sub)
        if end is None:
            return x.rfind(sub, start)
        return x.rfind(sub, start, end)

    def _find_substring(x, sub, start=None, end=None):
        sub = str(sub)
        if start is None and end is None:
            return x.find(sub)
        if end is None:
            return x.find(sub, start)
        return x.find(sub, start, end)

    def _format_using_mapping(x, mapping):
        return x.format_map(mapping)

    def _format_with(x, args, kwargs):
        if args is None:
            args = ()
        if kwargs is None:
            kwargs = {}
        return x.format(*args, **kwargs)

    def _index_of_last_substring(x, sub, start=None, end=None):
        sub = str(sub)
        if start is None and end is None:
            return x.rindex(sub)
        if end is None:
            return x.rindex(sub, start)
        return x.rindex(sub, start, end)

    def _index_of_substring(x, sub, start=None, end=None):
        sub = str(sub)
        if start is None and end is None:
            return x.index(sub)
        if end is None:
            return x.index(sub, start)
        return x.index(sub, start, end)

    def _padded_center(x, width, fill=' '):
        return x.center(width, str(fill)[0])

    def _replace_substring(x, old, new, count=None):
        old = str(old)
        new = str(new)
        if count is None:
            return x.replace(old, new)
        return x.replace(old, new, count)

    def _split_into(x, sep=None, maxsplit=-1):
        if sep is None:
            return x.split(None, maxsplit)
        return x.split(str(sep), maxsplit)

    def _split_into_lines(x, keepends=False):
        return x.splitlines(keepends)

    def _split_into_reverse(x, sep=None, maxsplit=-1):
        if sep is None:
            return x.rsplit(None, maxsplit)
        return x.rsplit(str(sep), maxsplit)

    def _starts_with_prefix(x, prefix, start=None, end=None):
        prefix = str(prefix)
        if start is None and end is None:
            return x.startswith(prefix)
        if end is None:
            return x.startswith(prefix, start)
        return x.startswith(prefix, start, end)

    def _strip_characters(x, chars):
        return x.strip(None if chars is None else str(chars))

    def _strip_leading(x, chars=None):
        if chars is None:
            return x.lstrip()
        return x.lstrip(str(chars))

    def _strip_trailing(x, chars=None):
        if chars is None:
            return x.rstrip()
        return x.rstrip(str(chars))

    def _to_encoded(x, encoding=None, errorHandling=None):
        if encoding is None and errorHandling is None:
            return x.encode()
        if errorHandling is None:
            return x.encode(str(encoding))
        return x.encode(str(encoding), str(errorHandling))


    _PyStringMethods: MutableMapping[str, Callable] = {
        ".charAt": lambda x,i: x[i],
        ".containsSubstring": lambda x, s: str(s) in x,
        ".countOccurrences": _count_occurrences,
        ".endsWithSuffix": _ends_with_suffix,
        ".expandTabs": str.expandtabs,
        ".findLastSubstring": _find_last_substring,
        ".findSubstring": _find_substring,
        ".formatUsingMapping": _format_using_mapping,
        ".formatWith": _format_with,
        ".getCharacterAt": lambda x,i: x[i],
        ".indexOfLastSubstring": _index_of_last_substring,
        ".indexOfSubstring": _index_of_substring,
        ".isAlphabetic": str.isalpha,
        ".isAlphanumeric": str.isalnum,
        ".isAsciiCharacters": str.isascii,
        ".isDecimalNumber": str.isdecimal,
        ".isDigitCharacters": str.isdigit,
        ".isLowercase": str.islower,
        ".isNumericCharacters": str.isnumeric,
        ".isPrintableCharacters": str.isprintable,
        ".isTitleCase": str.istitle,
        ".isUppercase": str.isupper,
        ".isValidIdentifier": str.isidentifier,
        ".isWhitespace": str.isspace,
        ".join": str.join,
        ".length": len,
        ".paddedCenter": _padded_center,
        ".removePrefix": lambda x, s: x.removeprefix(str(s)),
        ".removeSuffix": lambda x, s: x.removesuffix(str(s)),
        ".replaceSubstring": _replace_substring,
        ".splitInto": _split_into,
        ".splitIntoLines": _split_into_lines,
        ".splitIntoPartition": lambda x, s:  x.partition(str(s)),
        ".splitIntoReverse": _split_into_reverse,
        ".splitIntoReversePartition": lambda x, s:  x.rpartition(str(s)),
        ".startsWithPrefix": _starts_with_prefix,
        ".stripCharacters": _strip_characters,
        ".stripLeading": _strip_leading,
        ".stripTrailing": _strip_trailing,
        ".stripWhitespace": lambda x: x.strip(),
        ".subSequence": lambda x,s,e: x[s:e],
        ".swapCaseCharacters": lambda x: x.swapcase(),
        ".toCapitalized": lambda x: x.capitalize(),
        ".toCaseFolded": lambda x: x.casefold(),
        ".toEncoded": _to_encoded,
        ".toTitleCase": lambda x: x.title(),
        ".toUppercase": lambda x: x.upper(),
        ".translateUsingMapping": lambda x, m: x.translate(m),
        ".translateUsingSequence": lambda x, m: x.translate(m),
        ".zeroFill": str.zfill,
    }


    ### Collections
    def _c_add(x, *args):
        if len(args)==1:
            x.append(args[0])
            return True
        x.insert(args[0], args[1])
        return None

    def _c_remove_index(x, i):
        try:
            return x.pop(i)
        except (IndexError, KeyError):
            raise IndexError("Index out of range")

    def _c_remove_object(x, v):
        try:
            x.remove(v)
            return True
        except ValueError:
            return False

    def _c_set(x, i, v):
        if i<0:
            raise ValueError()
        out = x[i]
        x[i] = v
        return out

    def _removeall(x, c):
        c = set(c)
        nl = [i for i in x if not i in c]
        changed = len(nl) != len(x)
        x.clear()
        x.extend(nl)
        return changed

    def _retainall(x, c):
        c = set(c)
        nl = [i for i in x if i in c]
        changed = len(nl) != len(x)
        x.clear()
        x.extend(nl)
        return changed

    def _c_insert_all(x, i, c):
        for j, v in enumerate(c):
            x.insert(i + j, v)

    def _indexof(x, v):
        try:
            return x.index(v)
        except ValueError:
            return -1

    _PyCollectionMethods: MutableMapping[str, Callable] = {}
    _PyContainerMethods: MutableMapping[str, Callable] = {}
    _PySizedMethods: MutableMapping[str, Callable] = {}

    _PyIterableMethods: MutableMapping[str, Callable] = {
        ".allMatch": all,
        ".anyMatch": any,
        ".iter": iter,
        ".mapElements": _map,
        ".findMin": min,
        ".findMax": max,
        ".getSorted": sorted,
        ".computeSum": sum,
    }

    _PySequenceMethods: MutableMapping[str, Callable] = {
        ".remove": _delitem_return,
        ".set": _setitem_return,
        ".setAny": _setitem_return,
    }

    _PyListMethods: MutableMapping[str, Callable] = {
        ".add": _c_add,
        ".addAny": _c_add,

        ".clear": list.clear,
        ".contains": lambda x,v: v in x,
        ".extend": list.extend,
        ".get": lambda x,i: x[i],
        ".indexOf": _indexof,
        ".insert": _c_insert_all,
        ".remove": _c_remove_index,
        ".removeAny": _c_remove_object,
        ".removeAll": _removeall,
        ".retainAll": _retainall,
        ".set": _c_set,
        ".setAny": _setitem,
        ".size": len,
        ".subList": lambda x,s,e: x[s:e],
    }

    _PyTupleMethods = {
        ".contains": lambda x,v: v in x,
        ".get": lambda x,i: x[i],
        ".indexOf": _indexof,
        ".size": len,
        ".subList": lambda x,s,e: x[s:e],
    }


    ### Maps

    # Map specialized
    def _mapping_clear_noargs():
        raise TypeError("mappingClear() requires an object on the Python backend side")

    def _dict_setdefault(x, k, default):
        return x.setdefault(k, default)

    def _dict_update(x, other):
        if hasattr(other, "entrySet"):
            for k, v in other.entrySet():
                x[k] = v
            return
        x.update(other)

    def _dict_put(x, k, v):
        out = x.get(k)
        x[k] = v
        return out

    def _dict_remove_key_value(x, k, value):
        if k in x and x[k] == value:
            del x[k]
            return True
        return False

    # PyDict declares both remove(Object) and remove(Object,Object); both
    # cross the bridge under the same name (there is no overload-aware name
    # mangling on the Java side), so dispatch on arity here.
    def _dict_remove(x, k, *value):
        if value:
            return _dict_remove_key_value(x, k, value[0])
        return _delitem_return(x, k)

    def _putall(x, m):
        for p,v in m.entrySet():
            x[p] = v


    _PyDictMethods: MutableMapping[str, Callable] = {
        ".clear": lambda x: x.clear(),
        ".containsKey": lambda x,v: v in x,
        ".containsValue": lambda x,v: v in x.values(),
        ".get": lambda x, k: x.get(k),
        ".getOrDefault": lambda x,k,d: x.get(k,d),
        ".pop": lambda x, k, d: x.pop(k, d),
        ".put": _dict_put,
        ".putAny": _setitem_from_object,
        ".putAll": _putall,
        ".remove": _dict_remove,
        ".setDefault": _dict_setdefault,
        ".update": _dict_update,
    }

    _PyMappingMethods: MutableMapping[str, Callable] = {
        ".containsKey": lambda x,v: v in x,
        ".containsValue": lambda x,v: v in x.values(),
        ".putAll": _putall,
        # PyMapping itself only declares remove(Object), but this dispatch
        # table can end up merged over PyDict's (which also declares
        # remove(Object,Object)) depending on interface merge order, so it
        # must handle both arities too - see _dict_remove.
        ".remove": _dict_remove,
    }

    ### Sets
    def _set_add(s, v):
        if v in s:
            return False
        s.add(v)
        return True

    def _set_add_any(s, v):
        before = len(s)
        s.add(v)
        return len(s) != before

    # NOTE: PySet/PyFrozenSet declare their multi-set operations as Java
    # varargs (Collection<?>... set). When called with exactly one argument
    # (the overwhelmingly common case, e.g. `a.difference(b)`), the bridge
    # delivers that argument directly as `v` rather than wrapped in an
    # array - so these lambdas must NOT unpack `v`'s own elements via
    # `tuple(v)` (that was a real bug: it silently discarded the intended
    # operand and fell through to Python's iterable-of-characters behavior
    # on the operand's own contents). Multiple varargs arguments are not
    # exercised anywhere in this codebase; if that path is added later it
    # will need its own handling here.
    _PySetMethods = {
        ".add": _set_add,
        ".addAny": _set_add_any,
        ".clear": set.clear,
        ".contains": lambda x,v: v in x,
        ".copy": set.copy,
        ".difference": lambda x,v: x.difference(v),
        ".differenceUpdate": lambda x,v: x.difference_update(v),
        ".discard": set.discard,
        ".intersection": lambda x,v: x.intersection(v),
        ".intersectionUpdate": lambda x,s: x.intersection_update(s),
        ".isDisjoint": set.isdisjoint,
        ".isSubset": set.issubset,
        ".isSuperset": set.issuperset,
        ".size": len,
        ".pop": set.pop,
        ".symmetricDifference": lambda x,s: x.symmetric_difference(s),
        ".symmetricDifferenceUpdate": lambda x,s: x.symmetric_difference_update(s),
        ".toList": list,
        ".union": lambda x,s: x.union(s),
        ".unionUpdate": lambda x,s: x.update(s),
        ".update": set.update,
    }
    _PyAbstractSetMethods: MutableMapping[str, Callable] = {}
    _PyMutableSetMethods: MutableMapping[str, Callable] = {}
    _PyFrozenSetMethods: MutableMapping[str, Callable] = {
        ".contains": lambda x,v: v in x,
        ".copy": frozenset.copy,
        ".difference": lambda x,v: x.difference(v),
        ".intersection": lambda x,v: x.intersection(v),
        ".isDisjoint": frozenset.isdisjoint,
        ".isSubset": frozenset.issubset,
        ".isSuperset": frozenset.issuperset,
        ".size": len,
        ".symmetricDifference": lambda x,s: x.symmetric_difference(s),
        ".union": lambda x,v: x.union(v),
    }


    ### Generators
    _PyIterMethods: MutableMapping[str, Callable] = {
        ".tee": _tee_iterator,
        ".filter": lambda x,f : filter(f,x),
        ".toList": list,
        ".toSet": set,
    }

    # enumerate, zip, range
    _PyGeneratorMethods: MutableMapping[str, Callable] = {
        ".iter": iter,
        ".toList": list
    }

    _PyRangeMethods: MutableMapping[str, Callable] = {
        ".getStart": _attr("start"),
        ".getStop": _attr("stop"),
        ".getStep": _attr("step"),
        ".getLength": len,
        ".getItem": lambda x,i: x[i],
        ".getSlice": lambda x,s,e: x[s:e],
        ".contains": lambda x,v: v in x,
    }

    _PyCombinableMethods: MutableMapping[str, Callable] = {
        ".or": lambda x,y: x|y
    }

    def _pyexc_resolve(exc):
        if isinstance(exc, BaseException):
            cls = exc.__class__
        elif isinstance(exc, type) and issubclass(exc, BaseException):
            cls = exc
        else:
            return None
        if cls in _jpype._exc:
            return _jpype._exc[cls]
        for m in cls.__mro__:
            if issubclass(m, BaseException) and m in _jpype._exc:
                return _jpype._exc[m]
        return None
    _jpype._pyexc_resolve = _pyexc_resolve



    # Install the handler
    bridge = _jpype.interpreter()
    Backend = JClass("org.jpype.Backend")

    # DEBUG: Log dict state BEFORE Backend creation
    backend = Backend@JProxy(Backend, dict=_PyJPBackendMethods)

    #############################################################################
    # Populate the concrete dictionaries (created as placeholders in _core.py)
    # Add all of the concrete types to the _concrete interfaces list.
    _jpype._concrete[bytearray] = _PyByteArray
    _jpype._concrete[bytes] = _PyBytes
    _jpype._concrete[complex] = _PyComplex
    _jpype._concrete[dict] = _PyDict
    _jpype._concrete[enumerate] = _PyEnumerate
    _jpype._concrete[float] = _PyFloat
    _jpype._concrete[frozenset] = _PyFrozenSet
    _jpype._concrete[BaseException] = _PyExc
    _jpype._concrete[int] = _PyInt
    _jpype._concrete[list] = _PyList
    _jpype._concrete[memoryview] =  _PyMemoryView
    _jpype._concrete[object] = _PyObject
    _jpype._concrete[range] =  _PyList
    _jpype._concrete[set] =  _PySet
    _jpype._concrete[slice] = _PySlice
    _jpype._concrete[str] = _PyString
    _jpype._concrete[tuple] = _PyTuple
    _jpype._concrete[type] =  _PyType
    _jpype._concrete[zip] = _PyZip
    _jpype._concrete[range] = _PyRange
    _jpype._concrete[type] = _PyType
    _jpype._concrete[type(Ellipsis)] = _PyEllipsis
    _jpype._concrete[type(NotImplemented)] = _PyNotImplemented
    _jpype._concrete[types.FunctionType] = _PyFunction
    _jpype._concrete[filter] = _PyFilter

    # python.io and every other SPI provider's classes are no longer
    # hardcoded here - see the Installer wiring below, which drives
    # SpiLoader (Java) reading each provider's .pyspi resources and calling
    # back into _installer_register_class/_installer_register_backend.

    #############################################################################
    # Add all of the abstract types to the _protocol interfaces list
    # The key must be a string and the value a Java class
    _jpype._protocol["abstract_set"] = _PyAbstractSet
    _jpype._protocol["awaitable"] = _PyAwaitable
    _jpype._protocol["buffer"] = _PyBuffer
    _jpype._protocol["callable"] = _PyCallable
    _jpype._protocol["collection"] = _PyCollection
    _jpype._protocol["container"] = _PyContainer
    _jpype._protocol["coroutine"] = _PyCoroutine
    _jpype._protocol["generator"] = _PyGenerator
    _jpype._protocol["index"] = _PyIndex
    _jpype._protocol["subscript"] = _PySubscript
    _jpype._protocol["iter"] = _PyIter
    _jpype._protocol["iterable"] = _PyIterable
    _jpype._protocol["mapping"] = _PyMapping
    _jpype._protocol["mutable_set"] = _PyMutableSet
    _jpype._protocol["number"] = _PyNumber
    _jpype._protocol["sequence"] = _PySequence
    _jpype._protocol["sized"] = _PySized
    _jpype._protocol["combinable"] = _PyCombinable

    ###################################################################################
    # Bind the method tables

    # Define the method tables for each type here
    _jpype._methods[_PyBytes] = _PyBytesMethods
    _jpype._methods[_PyByteArray] = _PyByteArrayMethods
    _jpype._methods[_PyDict] = _PyDictMethods
    _jpype._methods[_PyEnumerate] = _PyGeneratorMethods
    _jpype._methods[_PyGenerator] = _PyGeneratorMethods
    _jpype._methods[_PyIter] = _PyIterMethods
    _jpype._methods[_PyList] = _PyListMethods
    _jpype._methods[_PyMemoryView] = _PyMemoryViewMethods
    _jpype._methods[_PyObject] = _PyObjectMethods
    _jpype._methods[_PyRange] = _PyRangeMethods
    _jpype._methods[_PySlice] = _PySliceMethods
    _jpype._methods[_PyString] = _PyStringMethods
    _jpype._methods[_PyTuple] = _PyTupleMethods
    _jpype._methods[_PyType] = _PyTypeMethods
    _jpype._methods[_PyZip] = _PyGeneratorMethods
    _jpype._methods[_PyComplex] = _PyComplexMethods
    _jpype._methods[_PyExc] = _PyExcMethods
    _jpype._methods[_PyIterable] = _PyIterableMethods
    _jpype._methods[_PyCallable] = _PyCallableMethods
    _jpype._methods[_PyMapping] = _PyMappingMethods
    _jpype._methods[_PyNumber] = _PyNumberMethods
    _jpype._methods[_PySequence] = _PySequenceMethods
    _jpype._methods[_PyAwaitable] = _PyAwaitableMethods
    _jpype._methods[_PyBuffer] = _PyBufferMethods
    _jpype._methods[_PyCollection] = _PyCollectionMethods
    _jpype._methods[_PyContainer] = _PyContainerMethods
    _jpype._methods[_PyCoroutine] = _PyCoroutineMethods
    _jpype._methods[_PyIndex] = _PyIndexMethods
    _jpype._methods[_PySubscript] = _PySubscriptMethods
    _jpype._methods[_PySized] = _PySizedMethods
    _jpype._methods[_PyInt] = _PyIntMethods
    _jpype._methods[_PyFloat] = _PyFloatMethods

    _jpype._methods[_PySet] = _PySetMethods
    _jpype._methods[_PyAbstractSet] = _PyAbstractSetMethods
    _jpype._methods[_PyMutableSet] = _PyMutableSetMethods
    _jpype._methods[_PyFrozenSet] = _PyFrozenSetMethods
    _jpype._methods[_PyCombinable] = _PyCombinableMethods

    ###################################################################################
    # Bind the exception types

    _jpype._exc = {}
    jexc = JClass("python.exceptions.PyBaseException")
    jpkg = JPackage("python.exceptions")
    for i in dir(jpkg):
        clz = getattr(jpkg, i)
        if issubclass(clz, jexc):
            exc = getattr(builtins,i[2:])
            _jpype._exc[exc] = clz

    # This is always called with exception from toJava() so it must be an exception type
    def _pyexc_convert(value):
        cls = value.__class__
        if cls in _jpype._exc:
            return _jpype._exc[cls](_jpype.pyobject(_PyExc, value))
        for m in cls.__mro__:
            if m in _jpype._exc:
                return _jpype._exc[m](_jpype.pyobject(_PyExc, value))
        return _pyexc_convert(AssertionError(f"JPype Internal Error: Exception type '{type(value).__name__}' bypassed upstream guards but matches no registered Java exception proxy."))
    _jpype._pyexc_convert = _pyexc_convert

    # We have everything setup
    _jpype.ready()
    bridge.setBackend(_jpype.context(), backend)

    ###################################################################################
    # SPI: implement Installer and hand it to Java, which immediately walks
    # every discovered WrapperService's .pyspi resources and calls back into
    # _installer_register_class/_installer_register_lazy_class/
    # _installer_register_backend (module-level, above) - see plan/SPI.md.
    # This replaces hand-written per-provider dicts in this file
    # (python.io was the first, now removed from here entirely).

    def _installer_register_backend(javaInterface, methodsSource):
        ns: MutableMapping[str, object] = {}
        exec(str(methodsSource), ns)
        methods = ns["METHODS"]
        iface = JClass(str(javaInterface))
        proxy = iface@JProxy(iface, dict=methods)
        # Registered on this interpreter's NativeContext (not a JVM-wide
        # static) so a second interpreter in the same JVM can't see or
        # clobber this one's mini-backends.
        _jpype.context().registerBackend(iface, proxy)

    _PyInstallerMethods: MutableMapping[str, Callable] = {
        "registerClass": _installer_register_class,
        "registerLazyClass": _installer_register_lazy_class,
        "registerBackend": _installer_register_backend,
    }

    Installer = JClass("org.jpype.Installer")
    installer = Installer@JProxy(Installer, dict=_PyInstallerMethods)
    bridge.setInstaller(installer)
