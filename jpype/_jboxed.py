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

# FIXME disabling these customizers for now.

@_jcustomizer.JImplementationFor("java.lang.Boolean")
class _JBoxedBoolean(object):
    # Boolean is special because in python True and False are singletons,
    # thus it is not possible to make a wrapper properly act as a bool
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


@_jcustomizer.JImplementationFor("java.lang.Byte")
@_jcustomizer.JImplementationFor("java.lang.Short")
@_jcustomizer.JImplementationFor("java.lang.Integer")
@_jcustomizer.JImplementationFor("java.lang.Long")
class _JBoxedLong(object):
    __eq__ = int.__eq__
    __ne__ = int.__ne__
    __lt__ = int.__lt__
    __gt__ = int.__gt__
    __le__ = int.__le__
    __ge__ = int.__ge__


@_jcustomizer.JImplementationFor("java.lang.Double")
@_jcustomizer.JImplementationFor("java.lang.Float")
class _JBoxedFloat(object):
    __eq__ = float.__eq__
    __ne__ = float.__ne__
