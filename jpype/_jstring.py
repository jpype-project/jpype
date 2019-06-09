# *****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
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

__all__ = ['JString']


class _JString(object):
    """ Base class for ``java.lang.String`` objects

    When called as a function, this class will produce a ``java.lang.String`` 
    object.  It can be used to test if an object is a Java string
    using ``isinstance(obj, JString)``.

    """
    def __new__(cls, *args, **kwargs):
        if cls == JString:
            cls = _jclass.JClass(JString.__javaclass__)
            return cls.__new__(cls, *args)
        return super(JString, cls).__new__(cls, *args, **kwargs)

    def __add__(self, other):
        return self.concat(other)

    def __eq__(self, other):
        if isinstance(other, JString):
            return self.equals(other)
        return str(self) == other

    def __ne__(self, other):
        if isinstance(other, JString):
            return not self.equals(other)
        return str(self) != other

    def __len__(self):
        return self.length()

    def __getitem__(self, i):
        return self.charAt(i)

    def __lt__(self, other):
        return self.compareTo(other) < 0

    def __le__(self, other):
        return self.compareTo(other) <= 0

    def __gt__(self, other):
        return self.compareTo(other) > 0

    def __ge__(self, other):
        return self.compareTo(other) >= 0

    def __contains__(self, other):
        return self.contains(other)

    def __hash__(self):
        return self.__str__().__hash__()

    def __repr__(self):
        return "'%s'"%self.__str__()


JString = _jobject.defineJObjectFactory(
    "JString", "java.lang.String", _JString)
_jcustomizer.registerClassBase('java.lang.String', JString)
