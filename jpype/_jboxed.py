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







__all__ = []


@_jcustomizer.JImplementationFor("java.lang.Boolean", base=True)
class _JBoxedBoolean(int, _jobject.JObject):
    # Boolean is special because in python True and False are singletons,
    # thus it is not possible to make a wrapper properly act as a bool
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (bool, int)):
            return int.__new__(cls, args[0])


        if isinstance(args[0], _jpype.PyJPValue):
            return int.__new__(cls, cls.booleanValue(args[0]))
        raise TypeError("Invalid argument '%s'" % type(args[0]).__name__)
    __eq__ = int.__eq__
    __ne__ = int.__ne__




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
        if isinstance(args[0], int):
            return int.__new__(cls, args[0])


        if isinstance(args[0], _jpype.PyJPValue):
            return int.__new__(cls, cls.byteValue(args[0]))
        raise TypeError("Invalid argument '%s'" % type(args[0]).__name__)
    __eq__ = int.__eq__
    __ne__ = int.__ne__




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
        if isinstance(args[0], int):
            return int.__new__(cls, args[0])


        if isinstance(args[0], _jpype.PyJPValue):
            return int.__new__(cls, cls.shortValue(args[0]))
        raise TypeError("Invalid argument '%s'" % type(args[0]).__name__)
    __eq__ = int.__eq__
    __ne__ = int.__ne__




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
        if isinstance(args[0], int):
            return int.__new__(cls, args[0])


        if isinstance(args[0], _jpype.PyJPValue):
            return int.__new__(cls, cls.intValue(args[0]))
        raise TypeError("Invalid argument '%s'" % type(args[0]).__name__)
    __eq__ = int.__eq__
    __ne__ = int.__ne__




    __lt__ = int.__lt__
    __gt__ = int.__gt__
    __le__ = int.__le__
    __ge__ = int.__ge__

    def __hash__(self):
        return int.__hash__(self)


@_jcustomizer.JImplementationFor("java.lang.Long", base=True)
class _JBoxedLong(int, _jobject.JObject):
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], int):
            return int.__new__(cls, args[0])


        if isinstance(args[0], _jpype.PyJPValue):
            return int.__new__(cls, cls.longValue(args[0]))
        raise TypeError("Invalid argument '%s'" % type(args[0]).__name__)
    __eq__ = int.__eq__
    __ne__ = int.__ne__




    __lt__ = int.__lt__
    __gt__ = int.__gt__
    __le__ = int.__le__
    __ge__ = int.__ge__

    def __hash__(self):
        return int.__hash__(self)


@_jcustomizer.JImplementationFor("java.lang.Float", base=True)
class _JBoxedFloat(float, _jobject.JObject):
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (int, float)):
            return float.__new__(cls, args[0])


        if isinstance(args[0], _jpype.PyJPValue):
            return float.__new__(cls, cls.floatValue(args[0]))
        raise TypeError("Invalid argument '%s'" % type(args[0]).__name__)
    __eq__ = float.__eq__
    __ne__ = float.__ne__

    def __hash__(self):
        return float.__hash__(self)


@_jcustomizer.JImplementationFor("java.lang.Double", base=True)
class _JBoxedDouble(float, _jobject.JObject):
    def __new__(cls, *args):
        if len(args) != 1:
            raise TypeError("Invalid arguments")
        if isinstance(args[0], (int, float)):
            return float.__new__(cls, args[0])


        if isinstance(args[0], _jpype.PyJPValue):
            return float.__new__(cls, cls.doubleValue(args[0]))
        raise TypeError("Invalid argument '%s'" % type(args[0]).__name__)
    __eq__ = float.__eq__
    __ne__ = float.__ne__

    def __hash__(self):
        return float.__hash__(self)

