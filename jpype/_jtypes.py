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
import sys as _sys

import _jpype
from . import _jclass
from . import _jobject
from . import _jcustomizer

__all__ = ['JBoolean', 'JByte', 'JChar', 'JShort',
           'JInt', 'JLong', 'JFloat', 'JDouble']

if _sys.version_info > (3,):
    _unicode = str
    _long = int
else:
    _unicode = unicode
    _long = long

# FIXME python2 and python3 get different conversions on int and long.  Likely we should
# unify to got the the same types regardless of version.

# Set up all the tables
_maxFloat = 0
_maxDouble = 0


def _initialize():
    _JP_TYPE_CLASSES = _jclass._JP_TYPE_CLASSES
    _JCLASSES = _jclass._JCLASSES
    global _maxFloat, _maxDouble

    # Place the jvalue types for primitives
    _JPrimitiveLoad(JBoolean, _jclass.JClass("java.lang.Boolean"))
    _JPrimitiveLoad(JByte, _jclass.JClass("java.lang.Byte"))
    _JPrimitiveLoad(JChar, _jclass.JClass("java.lang.Character"))
    _JPrimitiveLoad(JShort, _jclass.JClass("java.lang.Short"))
    _JPrimitiveLoad(JInt, _jclass.JClass("java.lang.Integer"))
    _JPrimitiveLoad(JLong, _jclass.JClass("java.lang.Long"))
    _JPrimitiveLoad(JFloat, _jclass.JClass("java.lang.Float"))
    _JPrimitiveLoad(JDouble, _jclass.JClass("java.lang.Double"))

    # Set up table of automatic conversions
    _JP_TYPE_CLASSES[bool] = JBoolean
    _JP_TYPE_CLASSES[int] = JLong
    _JP_TYPE_CLASSES[_long] = JLong
    _JP_TYPE_CLASSES[float] = JDouble
    _JP_TYPE_CLASSES[str] = _jclass.JClass("java.lang.String")
    _JP_TYPE_CLASSES[_unicode] = _jclass.JClass("java.lang.String")
    _JP_TYPE_CLASSES[type] = _jclass.JClass("java.lang.Class")
    _JP_TYPE_CLASSES[object] = _jclass.JClass("java.lang.Object")

    _maxFloat = _jclass.JClass("java.lang.Float").MAX_VALUE
    _maxDouble = _jclass.JClass("java.lang.Double").MAX_VALUE


def _JPrimitiveLoad(cls, boxedType):
    type.__setattr__(cls, '__javaclass__', _jpype.PyJPClass(cls.__name__))
    type.__setattr__(cls, '_java_boxed_class', boxedType)


class _JPrimitiveClass(_jclass.JClass):
    """ A wrapper specifying a specific java type.

    These objects have three fields:

     - __javaclass__ - the class for this object when matching arguments.
     - _java_boxed_class - the class to convert to when converting to an 
        object.
     - __javavalue__ - the instance of the java value.

    """
    def __new__(cls, name, basetype):
        members = {
            "__init__": _JPrimitive.init,
            "__setattr__": object.__setattr__,
            "__javaclass__": None,
            "_java_boxed_class": None,
        }
        return super(_JPrimitiveClass, cls).__new__(cls, name, (basetype, _JPrimitive), members)

    def __init__(self, *args):
        _jclass._JCLASSES[args[0]] = self
        super(_JPrimitive, self).__init__(self)

    def _load(self, boxed):
        type.__setattr__(self, '__javaclass__',
                         _jpype.PyJPClass(self.__name__))
        type.__setattr__(self, '_java_boxed_class', boxed)


class _JPrimitive(object):
    def __setattr__(self, attr, value):
        raise AttributeError("%s does not have field %s" %
                             (self.__name__, attr))

    def init(self, v):
        if v is not None:
            self._pyv = v
            self.__javavalue__ = _jpype.PyJPValue(self.__javaclass__, v)
        else:
            self.__javavalue__ = None

    def byteValue(self):
        if self._pyv < -128 or self._pyv > 127:
            raise OverFlowError("Cannot convert to byte value")
        return int(self._pyv)

    def shortValue(self):
        if self._pyv < -32768 or self._pyv > 32767:
            raise OverFlowError("Cannot convert to short value")
        return int(self._pyv)

    def intValue(self):
        if self._pyv < -2147483648 or self._pyv > 2147483647:
            raise OverFlowError("Cannot convert to int value")
        return int(self._pyv)

    def longValue(self):
        if self._pyv < -9223372036854775808 or self._pyv > 9223372036854775807:
            raise OverFlowError("Cannot convert to long value")
        return _long(self._pyv)

    def floatValue(self):
        if self._pyv < -_maxFloat or self._pyv > _maxFloat:
            raise OverFlowError("Cannot convert to long value")
        return float(self._pyv)

    def doubleValue(self):
        if self._pyv < -_maxDouble or self._pyv > _maxDouble:
            raise OverFlowError("Cannot convert to double value")
        return float(self._pyv)


JBoolean = _JPrimitiveClass("boolean", int)
JByte = _JPrimitiveClass("byte", int)
JChar = _JPrimitiveClass("char", int)
JShort = _JPrimitiveClass("short", int)
JInt = _JPrimitiveClass("int", int)
JLong = _JPrimitiveClass("long", _long)
JFloat = _JPrimitiveClass("float", float)
JDouble = _JPrimitiveClass("double", float)
