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
"""
JPype Types Module
------------------

Optional module containing only the Java types and factories used by
JPype.  Classes in this module include ``JArray``, ``JClass``,
``JBoolean``, ``JByte``, ``JChar``, ``JShort``, ``JInt``, ``JLong``,
``JFloat``, ``JDouble``, ``JString``, ``JObject``, and ``JException``.

Example:

    .. code-block:: python

        from jpype.types import *

"""
# import package to get minimum types needed to use module.
import _jpype
from ._jclass import *
from ._jobject import *
from ._jarray import *
from ._jexception import JException
from ._jstring import *

__all__ = [
    'JArray',
    'JClass',
    'JBoolean',
    'JByte',
    'JChar',
    'JShort',
    'JInt',
    'JLong',
    'JFloat',
    'JDouble',
    'JString',
    'JObject',
    'JException',
]


# JBoolean/JByte/JChar/JInt/JShort/JLong/JFloat/JDouble are built directly
# in C (see PyJPNumber_initType in native/python/pyjp_number.cpp and
# PyJPChar_initType in native/python/pyjp_char.cpp) rather than declared
# here as `class JXxx(_jpype._JYyy, internal=True): pass` statements: an
# ordinary Python class statement -- even through the internal metaclass --
# unconditionally picks up GC tracking from CPython's type_new, which these
# classes can never need (tp_dictoffset == 0, inherited from their non-GC
# family root, so they can never hold an arbitrary Python reference and so
# can never participate in a reference cycle).
JBoolean = _jpype.JBoolean
JByte = _jpype.JByte
JChar = _jpype.JChar
JInt = _jpype.JInt
JShort = _jpype.JShort
JLong = _jpype.JLong
JFloat = _jpype.JFloat
JDouble = _jpype.JDouble
