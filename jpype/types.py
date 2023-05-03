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


class JBoolean(_jpype._JBoolean, internal=True):  # type: ignore[call-arg]
    pass


class JByte(_jpype._JNumberLong, internal=True):  # type: ignore[call-arg]
    pass


class JChar(_jpype._JChar, internal=True):  # type: ignore[call-arg]
    pass


class JInt(_jpype._JNumberLong, internal=True):  # type: ignore[call-arg]
    pass


class JShort(_jpype._JNumberLong, internal=True):  # type: ignore[call-arg]
    pass


class JLong(_jpype._JNumberLong, internal=True):  # type: ignore[call-arg]
    pass


class JFloat(_jpype._JNumberFloat, internal=True):  # type: ignore[call-arg]
    pass


class JDouble(_jpype._JNumberFloat, internal=True):  # type: ignore[call-arg]
    pass


_jpype.JChar = JChar
