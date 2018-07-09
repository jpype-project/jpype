#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#*****************************************************************************
import collections
import sys as _sys

import _jpype
from . import _jclass
from . import _jobject
from . import _jstring
from . import _jcustomizer

if _sys.version > '3':
    unicode = str
    irange = range

__all__ = ['JArray']

_JARRAY_TYPENAME_MAP = {
  'boolean':'Z', 
  'byte':'B', 
  'char':'C', 
  'short':'S', 
  'int':'I', 
  'long':'J',
  'float':'F',
  'double':'D',
}

class _JArray(object):
    """ Create a java array class for a given component with a specified number 
    of dimensions.

    Class can be specified in three ways:
      - as a string with the name of a java class.
      - as a java type wrapper such as jpype.JInt.
      - as a java class type such as java.lang.String.
    
    Args:
      componentClass (str,type): type of element in to hold in the array.
      ndims (int): the number of dimensions of the array (default=1)

    Returns:
      JavaArrayClass: a new wrapper for a Java array class

    Raises:
      TypeError: if the component class is invalid or could not be found.


    """
    def __new__(cls, *args,**kwargs):
        if cls == JArray:
            return _JArrayNewClass(*args,**kwargs)
        return super(JArray, cls).__new__(cls)

    def __init__(self, *args, **kwargs):
        if hasattr(self, '__javavalue__'):
            self.__javaarray__ = _jpype.PyJPArray(self.__javavalue__)
            return

        if len(args) != 1:
            raise TypeError(
                "Array classes only take 2 parameters, {0} given"
                .format(len(args) + 1))

        if isinstance(args[0], _jpype.PyJPValue):
            self.__javavalue__ = args[0]
            self.__javaarray__ = _jpype.PyJPArray(self.__javavalue__)
            return

        values = None
        if _isIterable(args[0]):
            sz = len(args[0])
            values = args[0]
        else:
            sz = args[0]

        self.__javavalue__ = self.__class__.__javaclass__.newInstance(sz)
        self.__javaarray__ = _jpype.PyJPArray(self.__javavalue__)

        if values is not None:
            self.__javaarray__.setArraySlice(0, sz, values)

    def __str__(self):
        return str(tuple(self))

    def __len__(self):
        return self.__javaarray__.getArrayLength()

    def __iter__(self):
        return _JavaArrayIter(self)

    def __getitem__(self, ndx):
        if isinstance(ndx, slice):
            start, stop, step = ndx.indices(len(self))
            if step != 1:
                raise NotImplementedError("Slicing with step unimplemented")
            return self.__getslice__(start, stop)
        return self.__javaarray__.getArrayItem(ndx)

    def __setitem__(self, ndx, val):
        if isinstance(ndx, slice):
            start, stop, step = ndx.indices(len(self))
            if step != 1:
                # Iterate in python if we need to step
                indices = irange(start, stop, step)
                for index, value in zip(indices, val):
                    self[index] = value
            else:
                self.__setslice__(start, stop, val)
            return
        self.__javaarray__.setArrayItem(ndx, val)

    def __getslice__(self, i, j):
        if j == _sys.maxsize:
            j = self.__javaarray__.getArrayLength()
        return self.__javaarray__.getArraySlice(i, j)

    def __setslice__(self, i, j, v):
        if j == _sys.maxsize:
            j = self.__javaarray__.getArrayLength()
        self.__javaarray__.setArraySlice(i, j, v)

    def __eq__(self, other):
        if hasattr(other, '__javavalue__'):
            return self.equals(other)
        try:
            return self.equals(self.__class__(other))
        except TypeError:
            return False

    def __ne__(self, other):
        if hasattr(other, '__javavalue__'):
            return not self.equals(other)
        try:
            return self.equals(self.__class__(other))
        except TypeError:
            return True

JArray = _jobject.defineJObjectFactory("JArray", None, _JArray)


def _JArrayNewClass(cls, ndims=1):
    """ Convert a array class description into a JArray class."""
    jc = _jclass._toJavaClass(cls)
    
    if jc.isPrimitive():
        # primitives need special handling
        typename =('['*ndims)+_JARRAY_TYPENAME_MAP[jc.getCanonicalName()]
    else:
        typename = ('['*ndims)+'L'+str(_jobject.JObject(jc).getName())+';'

    return _jclass.JClass(typename)


# FIXME JavaArrayClass likely should be exposed for isinstance, issubtype
# FIXME are these not sequences?  They act like sequences but are they 
# connected to collections.Sequence
# has: __len__, __iter__, __getitem__ 
# missing: __contains__ (required for in)
# Cannot be Mutable because java arrays are fixed in length

def _isIterable(obj):
    if isinstance(obj, collections.Sequence):
        return True
    if hasattr(obj,'__len__') and hasattr(obj,'__iter__'):
        return True
    return False

class _JavaArrayIter(object):
    def __init__(self, a):
        self._array = a
        self._ndx = -1

    def __iter__(self):
        return self

    def __next__(self):
        self._ndx += 1
        if self._ndx >= len(self._array):
            raise StopIteration
        return self._array[self._ndx]

    next = __next__

#**********************************************************
# Char array customizer

def _charArrayStr(self):
    return str(_jstring.JString(self))

def _charArrayUnicode(self):
    return unicode(_jstring.JString(self))

def _charArrayEqual(self, other):
    if hasattr(other, '__javavalue__'):
        return self.equals(other)
    elif isinstance(other, (str, unicode)):
      return self[:]==other
    try:
        return self.equals(self.__class__(other))
    except TypeError:
        return False

class _JCharArrayCustomizer(object):
    def canCustomize(self, name, jc):
        return name == 'char[]' or name == "byte[]"

    def customize(self, name, jc, bases, members):
        members['__str__'] = _charArrayStr
        members['__unicode__'] = _charArrayUnicode
        members['__eq__'] = _charArrayEqual

_jcustomizer.registerClassCustomizer(_JCharArrayCustomizer())
