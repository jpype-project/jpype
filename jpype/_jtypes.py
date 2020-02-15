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

__all__ = ['JBoolean', 'JByte', 'JChar', 'JShort',
           'JInt', 'JLong', 'JFloat', 'JDouble']


class JBoolean(_jpype._JValueLong):
    pass


class JByte(_jpype._JValueLong):
    pass


class JChar(_jpype._JValueChar):
    pass


class JInt(_jpype._JValueLong):
    pass


class JShort(_jpype._JValueLong):
    pass


class JLong(_jpype._JValueLong):
    pass


class JFloat(_jpype._JValueFloat):
    pass


class JDouble(_jpype._JValueFloat):
    pass
