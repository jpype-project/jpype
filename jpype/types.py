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

from ._jclass import *
from ._jtypes import *
from ._jobject import *
from ._jarray import *
from ._jexception import *
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
