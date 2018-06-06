#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
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
#*****************************************************************************
import collections

from . import _jclass

class _WrappedIterator(object):
    """
    Wraps a Java iterator to respect the Python 3 iterator API
    """
    def __init__(self, iterator):
        self.iterator = iterator

    def __iter__(self):
        return self.iterator

    def __next__(self):
        return next(self.iterator)

    # Compatibility name
    next = __next__

def _initialize():
    _jclass.registerClassCustomizer(CollectionCustomizer())
    _jclass.registerClassCustomizer(ListCustomizer())
    _jclass.registerClassCustomizer(MapCustomizer())
    _jclass.registerClassCustomizer(IteratorCustomizer())
    _jclass.registerClassCustomizer(EnumerationCustomizer())

def isPythonSequence(v):
    if isinstance(v, collections.Sequence):
        if not hasattr(v.__class__, '__metaclass__') \
           or v.__class__.__metaclass__ is _jclass._JavaClass:
            return True
    return False

def _colLength(self):
    return self.size()

def _colIter(self):
    return _WrappedIterator(self.iterator())

def _colDelItem(self, i):
    return self.remove(i)

def _colAddAll(self, v):
    if isPythonSequence(v):
        r = False
        for i in v:
            r = self.add(i) or r
        return r
    else:
        return self._addAll(v)

def _colRemoveAll(self, v):
    if isPythonSequence(v):
        r = False
        for i in v:
            r = self.remove(i) or r
        return r
    else:
        return self._removeAll(v)

def _colRetainAll(self, v):
    if isPythonSequence(v):
        r = _jclass.JClass("java.util.ArrayList")(len(v))
        for i in v:
            r.add(i)
    else:
        r = v

    return self._retainAll(r)

class CollectionCustomizer(object):
    _METHODS = {
        '__len__': _colLength,
        '__iter__': _colIter,
        '__delitem__': _colDelItem,
    }

    def canCustomize(self, name, jc):
        if name == 'java.util.Collection':
            return True
        return jc.isSubclass('java.util.Collection')

    def customize(self, name, jc, bases, members):
        if name == 'java.util.Collection':
            members.update(CollectionCustomizer._METHODS)
        else:
            # AddAll is handled by List
            if (not jc.isSubclass("java.util.List")) and 'addAll' in members:
                members['_addAll'] = members['addAll']
                members['addAll'] = _colAddAll
            if 'removeAll' in members:
                members['_removeAll'] = members['removeAll']
                members['removeAll'] = _colRemoveAll
            if 'retainAll' in members:
                members['_retainAll'] = members['retainAll']
                members['retainAll'] = _colRetainAll

def _listGetItem(self, ndx):
    if isinstance(ndx, slice):
        start = ndx.start
        stop = ndx.stop
        if start < 0:
            start = self.size() + start
        if stop < 0:
            stop = self.size() + stop
        return self.subList(start, stop)
    else:
        if ndx < 0:
            ndx = self.size() + ndx
        return self.get(ndx)

def _listSetItem(self, ndx, v):
    if isinstance(ndx, slice):
        start = ndx.start
        stop = ndx.stop
        if start < 0:
            start = self.size() + start
        if stop < 0:
            stop = self.size() + stop
        for i in range(start, stop):
            self.remove(start)
        if isinstance(v, collections.Sequence):
            ndx = start
            for i in v:
                self.add(ndx, i)
                ndx += 1
    else:
        if ndx < 0:
            ndx = self.size() + ndx
        self.set(ndx, v)

def _listAddAll(self, v, v2=None):
    if isPythonSequence(v):
        r = False
        if v2 is not None: # assume form (int, values)
            for i in range(len(v2)):
                r = r or self.add(v + i, v2[i])
        else:
            for i in v:
                r = self.add(i) or r
        return r
    else:
        return self._addAll(v)

class ListCustomizer(object):
    _METHODS = {
        '__setitem__': _listSetItem,
        '__getitem__': _listGetItem,
    }

    def canCustomize(self, name, jc):
        if name == 'java.util.List':
            return True
        return jc.isSubclass('java.util.List')

    def customize(self, name, jc, bases, members):
        if name == 'java.util.List':
            members.update(ListCustomizer._METHODS)
        else:
            if 'addAll' in members:
                members['_addAll'] = members['addAll']
                members['addAll'] = _listAddAll

def isPythonMapping(v):
    if isinstance(v, collections.Mapping):
        if not hasattr(v.__class__, '__metaclass__') or \
           v.__class__.__metaclass__ is _jclass._JavaClass:
            return True
    return False

def _mapLength(self):
    return self.size()

def _mapIter(self):
    return _WrappedIterator(self.keySet().iterator())

def _mapDelItem(self, i):
    return self.remove(i)

def _mapGetItem(self, ndx):
    return self.get(ndx)

def _mapSetItem(self, ndx, v):
    self.put(ndx, v)

def _mapPutAll(self, v):
    if isPythonMapping(v):
        for i in v:
            self.put(i, v[i])
    else:
        # do the regular method ...
        self._putAll(v)

class MapCustomizer(object):
    _METHODS = {
        '__len__': _mapLength,
        '__iter__': _mapIter,
        '__delitem__': _mapDelItem,
        '__getitem__': _mapGetItem,
        '__setitem__': _mapSetItem,
    }

    def canCustomize(self, name, jc):
        if name == 'java.util.Map':
            return True
        return jc.isSubclass('java.util.Map')

    def customize(self, name, jc, bases, members):
        if name == 'java.util.Map':
            members.update(MapCustomizer._METHODS)
        else:
            if "putAll" in members:
                members["_putAll"] = members["putAll"]
                members["putAll"] = _mapPutAll

def _iterCustomNext(self):
    if self.hasNext():
        return self._next()
    raise StopIteration


def _iterIteratorNext(self):
    if self.hasNext():
        return next(self)
    raise StopIteration

def _iterIter(self):
    return self

class IteratorCustomizer(object):
    _METHODS = {
        '__iter__': _iterIter,
        '__next__': _iterCustomNext,
    }

    def canCustomize(self, name, jc):
        if name == 'java.util.Iterator':
            return True
        return jc.isSubclass('java.util.Iterator')

    def customize(self, name, jc, bases, members):
        if name == 'java.util.Iterator':
            members.update(IteratorCustomizer._METHODS)
        elif jc.isSubclass('java.util.Iterator'):
            __next__ = 'next' if 'next' in members else '__next__'
            members['_next'] = members[__next__]
            members[__next__] = _iterCustomNext

def _enumNext(self):
    if self.hasMoreElements():
        return self.nextElement()
    raise StopIteration

def _enumIter(self):
    return self


class EnumerationCustomizer(object):
    _METHODS = {
        'next': _enumNext,
        '__next__': _enumNext,
        '__iter__': _enumIter,
    }

    def canCustomize(self, name, jc):
        return name == 'java.util.Enumeration'

    def customize(self, name, jc, bases, members):
        members.update(EnumerationCustomizer._METHODS)
