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
from . import types as _jtypes
from . import _jcustomizer
from collections.abc import Mapping, Sequence, MutableSequence

JOverride = _jclass.JOverride


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
    Java Collisions to allow for Python syntax.
    """

    def __len__(self):
        return self.size()

    def __delitem__(self, i):
        raise TypeError(
            "'%s' does not support item deletion, use remove() method" % type(self).__name__)

    def __contains__(self, i):
        try:
            return self.contains(i)
        except TypeError:
            return False


def _sliceAdjust(slc, size):
    start = slc.start
    stop = slc.stop
    if slc.step and (slc.step > 1 or slc.step < 0):
        raise TypeError("Stride not supported")
    if start is None:
        start = 0
    if stop is None:
        stop = size
    if start < 0:
        start += size
    if stop < 0:
        stop += size
    return slice(start, stop)


@_jcustomizer.JImplementationFor('java.util.List')
class _JList(object):
    """ Customizer for ``java.util.List``

    This customizer adds the Python list operator to function on classes
    that implement the Java List interface.
    """

    def __jclass_init__(self):
        Sequence.register(self)
        MutableSequence.register(self)

    def __getitem__(self, ndx):
        if isinstance(ndx, slice):
            ndx = _sliceAdjust(ndx, self.size())
            return self.subList(ndx.start, ndx.stop)
        else:
            if ndx < 0:
                ndx += self.size()
            return self.get(ndx)

    def __setitem__(self, ndx, v):
        if isinstance(ndx, slice):
            ndx = _sliceAdjust(ndx, self.size())
            self[ndx.start:ndx.stop].clear()
            self.addAll(ndx.start, v)
        else:
            if ndx < 0:
                ndx += self.size()
            self.set(ndx, v)

    def __delitem__(self, ndx):
        if isinstance(ndx, slice):
            ndx = _sliceAdjust(ndx, self.size())
            self[ndx.start:ndx.stop].clear()
        elif hasattr(ndx, '__index__'):
            if ndx < 0:
                ndx += self.size()
            return self.remove_(_jtypes.JInt(ndx))
        else:
            raise TypeError("Incorrect arguments to del")

    def __reversed__(self):
        iterator = self.listIterator(self.size())
        while iterator.hasPrevious():
            yield iterator.previous()

    def index(self, obj):
        try:
            return self.indexOf(obj)
        except TypeError:
            raise ValueError("%s is not in list" % repr(obj))

    def count(self, obj):
        try:
            jo = _jpype.JObject(obj)
            c = 0
            for i in self:
                if i.equals(jo):
                    c += 1
            return c
        except TypeError:
            return 0

    def insert(self, idx, obj):
        if idx < 0:
            idx += self.size()
        return self.add(idx, obj)

    def append(self, obj):
        return self.add(obj)

    def reverse(self):
        _jpype.JClass("java.util.Collections").reverse(self)

    def extend(self, lst):
        self.addAll(lst)

    def pop(self, idx=-1):
        if idx < 0:
            idx += self.size()
        return self.remove_(_jtypes.JInt(idx))

    def __iadd__(self, obj):
        self.add(obj)
        return self

    def __add__(self, obj):
        new = self.clone()
        new.extend(obj)
        return new

    @JOverride(sticky=True, rename='remove_')
    def remove(self, obj):
        try:
            rc = self.remove_(_jpype.JObject(obj, _jpype.JObject))
            if rc is True:
                return
        except TypeError:
            pass
        raise ValueError("item not in list")


@_jcustomizer.JImplementationFor('java.util.Map')
class _JMap(object):
    """ Customizer for ``java.util.Map``

    This customizer adds the Python list and len operators to classes
    that implement the Java Map interface.
    """

    def __jclass_init__(self):
        Mapping.register(self)

    def __len__(self):
        return self.size()

    def __iter__(self):
        return self.keySet().iterator()

    def __delitem__(self, i):
        return self.remove(i)

    def __getitem__(self, ndx):
        try:
            item = self.get(ndx)
            if item is not None or self.containsKey(ndx):
                return item
        except TypeError:
            pass
        raise KeyError('%s' % ndx)

    def __setitem__(self, ndx, v):
        self.put(ndx, v)

    def items(self):
        return self.entrySet()

    def keys(self):
        return list(self.keySet())

    def __contains__(self, item):
        try:
            return self.containsKey(item)
        except TypeError:
            return False


@_jcustomizer.JImplementationFor('java.util.Set')
class _JSet(object):
    def __delitem__(self, i):
        return self.remove(i)


@_jcustomizer.JImplementationFor("java.util.Map.Entry")
class _JMapEntry(object):
    def __len__(self):
        return 2

    def __getitem__(self, x):
        if x == 0:
            return self.getKey()
        if x == 1:
            return self.getValue()
        raise IndexError("Pairs are always length 2")


@_jcustomizer.JImplementationFor('java.util.Iterator')
class _JIterator(object):
    """ Customizer for ``java.util.Iterator``

    This customizer adds the Python iterator concept to classes
    that implement the Java Iterator interface.
    """

    def __next__(self):
        if self.hasNext():
            return self.next()
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
