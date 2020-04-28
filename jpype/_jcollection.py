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
import datetime
from collections.abc import Sequence
from collections.abc import Mapping

import sys
import _jpype
from . import _jclass
from . import _jcustomizer
from . import types as _jtypes

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
    Java Collions to allow for Python syntax.
    """

    def __len__(self):
        return self.size()

    def __delitem__(self, i):
        raise TypeError(
            "'%s' does not support item deletion, use remove() method" % type(self).__name__)


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

    def __getitem__(self, ndx):
        if isinstance(ndx, slice):
            ndx = _sliceAdjust(ndx, self.size())
            return self.subList(ndx.start, ndx.stop)
        else:
            if ndx < 0:
                ndx = self.size() + ndx
            return self.get(ndx)

    def __setitem__(self, ndx, v):
        if isinstance(ndx, slice):
            ndx = _sliceAdjust(ndx, self.size())
            self[ndx.start:ndx.stop].clear()
            self.addAll(ndx.start, v)
        else:
            if ndx < 0:
                ndx = self.size() + ndx
            self.set(ndx, v)

    def __delitem__(self, ndx):
        if isinstance(ndx, slice):
            ndx = _sliceAdjust(ndx, self.size())
            self[ndx.start:ndx.stop].clear()
        elif hasattr(ndx, '__index__'):
            return self.remove(_jtypes.JInt(ndx))
        else:
            raise TypeError("Incorrect arguments to del")


def isPythonMapping(v):
    if isinstance(v, Mapping):
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

    def __jclass_init__(self):
        Mapping.register(self)

    def __len__(self):
        return self.size()

    def __iter__(self):
        return self.keySet().iterator()

    def __delitem__(self, i):
        return self.remove(i)

    def __getitem__(self, ndx):
        item = self.get(ndx)
        if item is None:
            if not self.containsKey(ndx):
                raise KeyError('%s' % ndx)
        return item

    def __setitem__(self, ndx, v):
        self.put(ndx, v)

    def items(self):
        return self.entrySet()

    def keys(self):
        return list(self.keySet())

    def __contains__(self, item):
        return self.containsKey(item)


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


# These methods need a home.
@_jcustomizer.JConversion("java.time.Instant", exact=datetime.datetime)
def _JInstantConversion(jcls, obj):
    utc = obj.replace(tzinfo=datetime.timezone.utc).timestamp()
    sec = int(utc)
    nsec = int((utc-sec)*1e9)
    return jcls.ofEpochSecond(sec, nsec)


if sys.version_info < (3, 6):
    import pathlib
    @_jcustomizer.JConversion("java.nio.file.Path", instanceof=pathlib.PurePath)
    def _JPathConvert(jcls, obj):
        Paths = _jpype.JClass("java.nio.file.Paths")
        return Paths.get(str(obj))

    @_jcustomizer.JConversion("java.io.File", instanceof=pathlib.PurePath)
    def _JFileConvert(jcls, obj):
        return jcls(str(obj))


@_jcustomizer.JConversion("java.nio.file.Path", attribute="__fspath__")
def _JPathConvert(jcls, obj):
    Paths = _jpype.JClass("java.nio.file.Paths")
    return Paths.get(obj.__fspath__())


@_jcustomizer.JConversion("java.io.File", attribute="__fspath__")
def _JFileConvert(jcls, obj):
    return jcls(obj.__fspath__())


@_jcustomizer.JConversion("java.util.Collection", instanceof=Sequence)
def _JSequenceConvert(jcls, obj):
    return _jclass.JClass('java.util.Arrays').asList(obj)


@_jcustomizer.JConversion("java.util.Map", instanceof=Mapping)
def _JMapConvert(jcls, obj):
    hm = _jclass.JClass('java.util.HashMap')()
    for p, v in obj.items():
        hm[p] = v
    return hm
