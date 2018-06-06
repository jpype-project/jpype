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
import sys

import _jpype
from . import _jclass
from . import _jwrapper

if sys.version > '3':
    unicode = str
    irange = range

_CLASSES = {}
_CUSTOMIZERS = []

def _initialize():
    _jpype.setResource('JavaArrayClass',_JavaArrayClass)
    _jpype.setResource('GetJavaArrayClassMethod',_getClassFor)

    registerArrayCustomizer(CharArrayCustomizer())
    registerArrayCustomizer(ByteArrayCustomizer())

def registerArrayCustomizer(c):
    _CUSTOMIZERS.append(c)

class _JavaArrayClass(object):
    def __init__(self, jo):
        self.__javaobject__ = jo

    def __str__(self):
        return str(tuple(self))

    def __len__(self):
        return _jpype.getArrayLength(self.__javaobject__)

    def __iter__(self):
        return _JavaArrayIter(self)

    def __getitem__(self, ndx):
        if isinstance(ndx, slice):
            start, stop, step = ndx.indices(len(self))
            if step != 1:
                raise RuntimeError("Slicing with step unimplemented")
            return self.__getslice__(start, stop)
        return _jpype.getArrayItem(self.__javaobject__, ndx)

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
        _jpype.setArrayItem(self.__javaobject__, ndx, val)

    def __getslice__(self, i, j):
        if j == sys.maxsize:
            j = _jpype.getArrayLength(self.__javaobject__)
        return _jpype.getArraySlice(self.__javaobject__, i, j)

    def __setslice__(self, i, j, v):
        if j == sys.maxsize:
            j = _jpype.getArrayLength(self.__javaobject__)
        _jpype.setArraySlice(self.__javaobject__, i, j, v)

    def __setattr__(self, attr, value):
        if attr.startswith('_') \
               or callable(value) \
               or isinstance(getattr(self.__class__, attr), property):
            object.__setattr__(self, attr, value)
        else:
            raise AttributeError("%s does not have field %s"%(self.__name__, attr), self)


def _jarrayInit(self, *args):
    if len(args) == 2 and args[0] == _jclass._SPECIAL_CONSTRUCTOR_KEY:
        _JavaArrayClass.__init__(self, args[1])
    elif len(args) != 1:
        raise TypeError(
            "Array classes only take 2 parameters, {0} given"
            .format(len(args) + 1))
    else:
        values = None
        if isinstance(args[0], collections.Sequence):
            sz = len(args[0])
            values = args[0]
        else:
            sz = args[0]

        _JavaArrayClass.__init__(
            self, _jpype.newArray(self.__class__.__javaclass__, sz))

        if values is not None:
            _jpype.setArraySlice(self.__javaobject__, 0, sz, values)


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

class _JavaArray(type):
    pass

def _defineArrayClass(name, jt):
    members = {
            "__init__": _jarrayInit,
            "__javaclass__": jt,
            "__name__": name,
    }

    bases = [_JavaArrayClass]

    for i in _CUSTOMIZERS:
        if i.canCustomize(name, jt):
            i.customize(name, jt, bases, members)

    return _JavaArray(name, tuple(bases), members)

def _getClassFor(name):
    if name not in _CLASSES:
        jc = _jpype.findArrayClass(name)
        _CLASSES[name] = _defineArrayClass(name, jc)

    return _CLASSES[name]


def JArray(t, ndims=1):
    if issubclass(t, _jwrapper._JWrapper):
        t = t.typeName

    elif isinstance(t, _JavaArray):
        t = t.typeName

    elif issubclass(t, _jclass._JAVAOBJECT):
        t = t.__name__

    elif not isinstance(t, str) and not isinstance(t, unicode):
        raise TypeError("Argument must be a java class, java array class, "
                        "java wrapper or string representing a java class")

    arrayTypeName = t + ('[]' * ndims)

    return _getClassFor(arrayTypeName)

def _charArrayStr(self):
    return ''.join(self)

def _charArrayUnicode(self):
    return u''.join(self)

class CharArrayCustomizer(object):
    def canCustomize(self, name, jc):
        if name == 'char[]':
            return True
        return False

    def customize(self, name, jc, bases, members):
        members['__str__'] = _charArrayStr
        members['__unicode__'] = _charArrayUnicode

def _byteArrayStr(self):
    s = _jclass.JClass('java.lang.String')(self)
    return s.toString()


class ByteArrayCustomizer(object):
    def canCustomize(self, name, jc):
        if name == 'byte[]':
            return True
        return False

    def customize(self, name, jc, bases, members):
        members['__str__'] = _byteArrayStr
