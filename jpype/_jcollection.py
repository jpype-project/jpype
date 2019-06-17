# *****************************************************************************
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
# *****************************************************************************
try:
    from collections.abc import Sequence
except ImportError:
    from collections import Sequence

from . import _jclass
from . import _jcustomizer
from . import _jtypes

JOverride = _jclass.JOverride


def isPythonSequence(v):
    if isinstance(v, Sequence):
        if not hasattr(v.__class__, '__metaclass__') \
           or v.__class__.__metaclass__ is _jclass._JavaClass:
            return True
    return False


@_jcustomizer.JImplementationFor("java.lang.Iterable")
class _JIterable(object):
    """ Customizer for ``java.util.Iterable``

    This customizer adds the Python iterator syntax to classes that 
    implement Java Iterable.
    """

    def __iter__(self):
        return self.iterator()


@_jcustomizer.JImplementationFor("java.util.Collection")
class _JCollection(object):
    """ Customizer for ``java.util.Collection``

    This customizer adds the Python functions ``len()`` and ``del`` to
    Java Collions to allow for Python syntax.
    """

    def __len__(self):
        return self.size()

    def __delitem__(self, i):
        return self.remove(i)

    @JOverride(sticky=True)
    def addAll(self, v):
        if isPythonSequence(v):
            v = _jclass.JClass('java.util.Arrays').asList(v)
        return self._addAll(v)

    @JOverride(sticky=True)
    def removeAll(self, v):
        if isPythonSequence(v):
            v = _jclass.JClass('java.util.Arrays').asList(v)
        return self._removeAll(v)

    @JOverride(sticky=True)
    def retainAll(self, v):
        if isPythonSequence(v):
            v = _jclass.JClass('java.util.Arrays').asList(v)
        return self._retainAll(v)


@_jcustomizer.JImplementationFor('java.util.List')
class _JList(object):
    """ Customizer for ``java.util.List``

    This customizer adds the Python list operator to function on classes
    that implement the Java List interface.
    """

    def __getitem__(self, ndx):
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

    def __setitem__(self, ndx, v):
        if isinstance(ndx, slice):
            if isPythonSequence(v):
                v = _jclass.JClass('java.util.Arrays').asList(v)
            self[ndx.start:ndx.stop].clear()
            self.addAll(start, v)
        else:
            if ndx < 0:
                ndx = self.size() + ndx
            self.set(ndx, v)

    def __delitem__(self, ndx):
        if isinstance(ndx, slice):
            self[ndx.start:ndx.stop].clear()
        elif hasattr(ndx, '__index__'):
            return self.remove(_jtypes.JInt(ndx))
        else:
            raise TypeError("Incorrect arguments to del")

    @JOverride(sticky=True)
    def addAll(self, *args):
        if len(args) == 1:
            v = args[0]
            if isPythonSequence(v):
                v = _jclass.JClass('java.util.Arrays').asList(v)
            self._addAll(v)
        elif len(args) == 2 and hasattr(args[0], '__index__'):
            v = args[1]
            if isPythonSequence(v):
                v = _jclass.JClass('java.util.Arrays').asList(v)
            self._addAll(args[0], v)
        else:
            raise TypeError("Incorrect arguments to addAll")


def isPythonMapping(v):
    if isinstance(v, collections.Mapping):
        if not hasattr(v.__class__, '__metaclass__') or \
           v.__class__.__metaclass__ is _jclass._JavaClass:
            return True
    return False


@_jcustomizer.JImplementationFor('java.util.Map')
class _JMap(object):
    """ Customizer for ``java.util.Map``

    This customizer adds the Python list and len operators to classes
    that implement the Java Map interface.
    """
    #    def __jclass_init__(cls):
    #        type.__setattr__(cls, 'putAll', _JMap.putAll)

    def __len__(self):
        return self.size()

    def __iter__(self):
        return self.keySet().iterator()

    def __delitem__(self, i):
        return self.remove(i)

    def __getitem__(self, ndx):
        return self.get(ndx)

    def __setitem__(self, ndx, v):
        self.put(ndx, v)

    @JOverride(sticky=True)
    def putAll(self, v):
        if isPythonMapping(v):
            for i in v:
                self.put(i, v[i])
        else:
            # do the regular method ...
            self._putAll(v)


@_jcustomizer.JImplementationFor('java.util.Iterator')
class _JIterator(object):
    """ Customizer for ``java.util.Iterator``

    This customizer adds the Python iterator concept to classes
    that implement the Java Iterator interface.
    """
#    def __jclass_init__(cls):
    #        type.__setattr__(cls, '_next', cls.next)
    #        type.__setattr__(cls, 'next', _JIterator.__next__)

    # Python 2 requires next to function as python next(), thus
    # we have a conflict in behavior. Java next is renamed.
    @JOverride(sticky=True, rename="_next")
    def next(self):
        if self.hasNext():
            return self._next()
        raise StopIteration

    def __next__(self):
        if self.hasNext():
            return self._next()
        raise StopIteration

    def __iter__(self):
        return self


@_jcustomizer.JImplementationFor('java.util.Enumeration')
class _JEnumeration(object):
    """ Customizer for ``java.util.Enumerator``

    This customizer adds the Python iterator concept to classes
    that implement the Java Enumerator interface.
    """

    def __next__(self):
        if self.hasMoreElements():
            return self.nextElement()
        raise StopIteration

    def __iter__(self):
        return self

    next = __next__
