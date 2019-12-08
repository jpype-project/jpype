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


#class _JPrimitiveClass(_jclass.JClass):
#    """ A wrapper specifying a specific java type.
#
#    These objects have three fields:
#
#     - __javaclass__ - the class for this object when matching arguments.
#     - _java_boxed_class - the class to convert to when converting to an 
#        object.
#     - __javavalue__ - the instance of the java value.
#
#    """
#    def __new__(cls, name, code, basetype, jc):
def _JPrimitiveClass(name, basetype, jc):
    members = {
        "__init__": JPrimitive.init,
        "__setattr__": object.__setattr__,
        '__javaclass__': jc,
    }
    return _jclass.JClass(name, (basetype, JPrimitive), members)


class JPrimitive(object):
    def init(self, v):
        if v is not None:
            jc = _jpype._primitive_types[self.__class__.__name__]
            self._pyv = v
            self.__javavalue__ = _jpype.PyJPValue(jc, v)
        else:
            self.__javavalue__ = None

    def __setattr__(self, attr, value):
        raise AttributeError("%s does not have field %s" %
                             (self.__name__, attr))

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
        return int(self._pyv)

    def floatValue(self):
        if self._pyv < -_maxFloat or self._pyv > _maxFloat:
            raise OverFlowError("Cannot convert to long value")
        return float(self._pyv)

    def doubleValue(self):
        if self._pyv < -_maxDouble or self._pyv > _maxDouble:
            raise OverFlowError("Cannot convert to double value")
        return float(self._pyv)

_jpype.JPrimitive = JPrimitive

# Primitive types are their own special classes as they do not tie to the JVM
print(dir(_jpype))
JBoolean = _JPrimitiveClass("boolean", int, _jpype._jboolean)
JByte = _JPrimitiveClass("byte",  int, _jpype._jbyte)
JChar = _JPrimitiveClass("char",  int, _jpype._jchar)
JShort = _JPrimitiveClass("short", int, _jpype._jshort)
JInt = _JPrimitiveClass("int",  int, _jpype._jint)
JLong = _JPrimitiveClass("long",  int, _jpype._jlong)
JFloat = _JPrimitiveClass("float",  float, _jpype._jfloat)
JDouble = _JPrimitiveClass("double",  float, _jpype._jdouble)


