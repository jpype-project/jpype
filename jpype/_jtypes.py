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

__all__ = ['JBoolean', 'JByte', 'JChar', 'JShort', 'JInt', 'JLong', 'JFloat', 'JDouble' ]

# FIXME python2 and python3 get different conversions on int and long.  Likely we should
# unify to got the the same types regardless of version.

# Set up all the tables
_maxFloat = 3.4028234663852886E38
_maxDouble = 1.7976931348623157E308

class JPrimitive(_jpype.PyJPValueBase):

    def byteValue(self):
        value = int(self)
        if value < -128 or value > 127:
            raise OverFlowError("Cannot convert to byte value")
        return value

    def shortValue(self):
        value = int(self)
        if value < -32768 or value > 32767:
            raise OverFlowError("Cannot convert to short value")
        return value

    def intValue(self):
        value = int(self)
        if self._pyv < -2147483648 or self._pyv > 2147483647:
            raise OverFlowError("Cannot convert to int value")
        return value

    def longValue(self):
        value = int(self)
        if value < -9223372036854775808 or value > 9223372036854775807:
            raise OverFlowError("Cannot convert to long value")
        return value

    def floatValue(self):
        value = float(self)
        if value < -_maxFloat or value > _maxFloat:
            raise OverFlowError("Cannot convert to float value")
        return value

    def doubleValue(self):
        return float(value)

class _JPrimitiveChar(JPrimitive):
    def __new__(self, v):
        if isinstance(v,str) and len(v)==1:
            return _jpype.PyJPValueLong.__new__(self, ord(v))
        if isinstance(v,int):
            return _jpype.PyJPValueLong.__new__(self, v)
        raise ValueError("Bad char for conversion")

    def __str__(self):
        return chr(int(self))

def _JPrimitiveClass(name, basetype, jc):
    members = {
        '__javaclass__': jc,
    }
    return _jclass.JClass(name, basetype, members)

_jpype.JPrimitive = JPrimitive

# Primitive types are their own special classes as they do not tie to the JVM
JBoolean = _JPrimitiveClass("boolean", (JPrimitive, _jpype.PyJPValueLong), _jpype._jboolean)
JByte = _JPrimitiveClass("byte",  (JPrimitive, _jpype.PyJPValueLong), _jpype._jbyte)
JChar = _JPrimitiveClass("char",  (_JPrimitiveChar, _jpype.PyJPValueLong), _jpype._jchar)
JShort = _JPrimitiveClass("short", (JPrimitive, _jpype.PyJPValueLong), _jpype._jshort)
JInt = _JPrimitiveClass("int",  (JPrimitive, _jpype.PyJPValueLong), _jpype._jint)
JLong = _JPrimitiveClass("long",  (JPrimitive, _jpype.PyJPValueLong), _jpype._jlong)
JFloat = _JPrimitiveClass("float",  (JPrimitive, _jpype.PyJPValueFloat), _jpype._jfloat)
JDouble = _JPrimitiveClass("double", (JPrimitive,  _jpype.PyJPValueFloat), _jpype._jdouble)


