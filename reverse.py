import jpype
import jpype.imports
from jpype import JImplements, JOverride, JConversion, JProxy

jpype.startJVM()


# First we need to establish a front end so that Java can create Python objects from Java ones
@JImplements("python.lang.PyBuiltins")
class _PyJBuiltins(obj):
    @JOverride
    def tuple(self, args):
        return tuple(*args)

     @JOverride
    def list(self, args):
        out = list()
        for i in args:
            out.append(i)
        return out

    @JOverride
    def dict(self, args):
        out = dict()
        for p,v in args.entrySet()
            out[p]=v
        return out

    @JOverride
    def str(self, args)
        return str(args)

# Install the handler
JClass("python.lang.PyBuiltins").builtins = _PyJBuiltins()

###################################################################################
# Name our types into local scope

_PyObject = JClass("python.lang.PyObject")
_PyObjectJava = JClass("python.lang.PyObjectJava")
_PyTuple = JClass("python.lang.PyType")
_PyDict = JClass("python.lang.PyDict")
_PyString = JClass("python.lang.PyString")

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


# Now we need to set up the dictionary for the proxy object
#  This seems trivial but the name binds the Java interface to existing functions
_PyJObjectMethods = { 
    "hasAttr", hasattr,
    "getAttr", getattr,
    "delAttr", delattr,
    "setItem", _setitem,
    "getItem", _getitem,
    "isInstance", isinstance,
    "call": _call,  
    "not": _not,
    "isTrue": _isTrue,
    "asFloat": float,
    "asInt": int,
    "hash": hash,
    "dir": dir,
    "len": len,
    "iter": iter,
    "type": type,
    "bytes": bytes, 
    "len": len, 
    "str": str, 
    "repr": repr,
}

###################################################################################
# Next we need to set up a conversion factory so that Python objects automatically 
# construct Java proxies
#
# This is a trivial task as we have JProxy which can add an interface to any Python
# object.  We just need to make it tag in such a way that it automatically unwraps
# back to Python when passed from Java.
# 


# We can add special behaviors here by given them more methods
JConversion(_PyTuple, instanceof=tuple)
def _pytuple(jcls, obj):
   return JProxy(jcls, dict=_PyJObjectMethods, inst=x)

JConversion(_PyDict, instanceof=dict)
def _pydict(jcls, obj):
   return JProxy(jcls, dict=_PyJObjectMethods, inst=x)

JConversion(_PyDict, instanceof=dict)
def _pylist(jcls, obj):
   return JProxy(jcls, dict=_PyJObjectMethods, inst=x)

JConversion(_PyString, instanceof=str)
def _pylist(jcls, obj):
   return JProxy(jcls, dict=_PyJObjectMethods, inst=x)


# Creat a dispatch which will bind Python concrete types to Java.
# Most of the time people won't see them, but we can add Java interfaces to 
# make those objects become Java like.
_conversionDispatch = {
    list: lambda x: _pylist(_PyList, x),
    tuple: lambda x: _pytuple(_PyTyple, x),
    str:  lambda x: _pystr(_PyString, x),
    dict: lambda x: _pydict(_PyDict, x),
    _jpype._JObject: lambda x: _PyObjectJava(obj),
   
}

# Set the default conversion if nothing found in the dispatch
def _conversionDefault(x):
   return JProxy(_PyObject, dict=_PyJObjectMethods, inst=x)

# Next we bind the dispatch to the Java types using the dispatch
JConversion(_PyObject, instanceof=object)
def _pyobject(jcls, obj):
    # Watch for the special case of a pure object
    if len(obj.__class__.__mro__) < 2:
        return _conversionDefault(obj)
    # Otherwise use the dispatch
    return _conversionDispatch.get(obj.__class__.__mro__[-2], _conversionDefault)


