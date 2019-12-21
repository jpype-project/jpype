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
import jpype
import _jpype
from . import _jcustomizer
from . import _jobject

import sys as _sys

# Python2/3 support
if _sys.version_info > (3,):
    _long = int
else:
    _long = long

__all__ = []


@_jcustomizer.JImplementationFor("java.lang.Boolean", base=True)
class _JBoxedBoolean(int, _jobject.JObject):
    # Boolean is special because in python True and False are singletons,
    # thus it is not possible to make a wrapper properly act as a bool
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (bool, int, _long)):
            return int.__new__(cls, args[0])
        if hasattr(args[0], 'booleanValue'):
            return int.__new__(cls, args[0].booleanValue())
        if isinstance(args[0], _jpype.PyJPValue):
            return int.__new__(cls, cls.booleanValue(args[0]))
        raise ValueError("Invalid arguments %s" % args[0])
    __eq__ = int.__eq__
    __ne__ = int.__ne__

    if _sys.version_info < (3,):
        __cmp__ = int.__cmp__
    else:
        __lt__ = int.__lt__
        __gt__ = int.__gt__
        __le__ = int.__le__
        __ge__ = int.__ge__

    def __str__(self):
        if int(self) == 0:
            return str(False)
        return str(True)

    def __hash__(self):
        return int.__hash__(self)


@_jcustomizer.JImplementationFor("java.lang.Byte", base=True)
class _JBoxedByte(int, _jobject.JObject):
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (int, _long)):
            return int.__new__(cls, args[0])
        if hasattr(args[0], 'byteValue'):
            return int.__new__(cls, args[0].byteValue())
        if isinstance(args[0], _jpype.PyJPValue):
            return int.__new__(cls, cls.byteValue(args[0]))
        raise ValueError("Invalid arguments %s" % args[0])
    __eq__ = int.__eq__
    __ne__ = int.__ne__

    if _sys.version_info < (3,):
        __cmp__ = int.__cmp__
    else:
        __lt__ = int.__lt__
        __gt__ = int.__gt__
        __le__ = int.__le__
        __ge__ = int.__ge__

    def __hash__(self):
        return int.__hash__(self)


@_jcustomizer.JImplementationFor("java.lang.Short", base=True)
class _JBoxedShort(int, _jobject.JObject):
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (int, _long)):
            return int.__new__(cls, args[0])
        if hasattr(args[0], 'shortValue'):
            return int.__new__(cls, args[0].shortValue())
        if isinstance(args[0], _jpype.PyJPValue):
            return int.__new__(cls, cls.shortValue(args[0]))
        raise ValueError("Invalid arguments %s" % args[0])
    __eq__ = int.__eq__
    __ne__ = int.__ne__

    if _sys.version_info < (3,):
        __cmp__ = int.__cmp__
    else:
        __lt__ = int.__lt__
        __gt__ = int.__gt__
        __le__ = int.__le__
        __ge__ = int.__ge__

    def __hash__(self):
        return int.__hash__(self)


@_jcustomizer.JImplementationFor("java.lang.Integer", base=True)
class _JBoxedInteger(int, _jobject.JObject):
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (int, _long)):
            return int.__new__(cls, args[0])
        if hasattr(args[0], 'intValue'):
            return int.__new__(cls, args[0].intValue())
        if isinstance(args[0], _jpype.PyJPValue):
            return int.__new__(cls, cls.intValue(args[0]))
        raise ValueError("Invalid arguments %s" % args[0])
    __eq__ = int.__eq__
    __ne__ = int.__ne__

    if _sys.version_info < (3,):
        __cmp__ = int.__cmp__
    else:
        __lt__ = int.__lt__
        __gt__ = int.__gt__
        __le__ = int.__le__
        __ge__ = int.__ge__

    def __hash__(self):
        return int.__hash__(self)


@_jcustomizer.JImplementationFor("java.lang.Long", base=True)
class _JBoxedLong(_long, _jobject.JObject):
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (int, _long)):
            return _long.__new__(cls, args[0])
        if hasattr(args[0], 'longValue'):
            return _long.__new__(cls, args[0].longValue())
        if isinstance(args[0], _jpype.PyJPValue):
            return _long.__new__(cls, cls.longValue(args[0]))
        raise ValueError("Invalid arguments %s" % args[0])
    __eq__ = _long.__eq__
    __ne__ = _long.__ne__

    if _sys.version_info < (3,):
        __cmp__ = _long.__cmp__
    else:
        __lt__ = _long.__lt__
        __gt__ = _long.__gt__
        __le__ = _long.__le__
        __ge__ = _long.__ge__

    def __hash__(self):
        return _long.__hash__(self)


@_jcustomizer.JImplementationFor("java.lang.Float", base=True)
class _JBoxedFloat(float, _jobject.JObject):
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (int, _long, float)):
            return float.__new__(cls, args[0])
        if hasattr(args[0], 'longValue'):
            return float.__new__(cls, args[0].floatValue())
        if isinstance(args[0], _jpype.PyJPValue):
            return float.__new__(cls, cls.floatValue(args[0]))
        raise ValueError("Invalid arguments %s" % args[0])
    __eq__ = float.__eq__
    __ne__ = float.__ne__

    def __hash__(self):
        return float.__hash__(self)


@_jcustomizer.JImplementationFor("java.lang.Double", base=True)
class _JBoxedDouble(float, _jobject.JObject):
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (int, _long, float)):
            return float.__new__(cls, args[0])
        if hasattr(args[0], 'longValue'):
            return float.__new__(cls, args[0].doubleValue())
        if isinstance(args[0], _jpype.PyJPValue):
            return float.__new__(cls, cls.doubleValue(args[0]))
        raise ValueError("Invalid arguments %s" % args[0])
    __eq__ = float.__eq__
    __ne__ = float.__ne__

    def __hash__(self):
        return float.__hash__(self)

