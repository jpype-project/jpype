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
import typing

import _jpype
from . import _jcustomizer

__all__ = ['JString']


class JString(_jpype._JObject, internal=True):  # type: ignore[call-arg]
    """ Base class for ``java.lang.String`` objects

    When called as a function, this class will produce a ``java.lang.String``
    object.  It can be used to test if an object is a Java string
    using ``isinstance(obj, JString)``.

    """
    def __new__(cls, *args, **kwargs):
        if cls != JString:
            raise TypeError("JString factory cannot be used as base class")
        cls = _jpype.JClass("java.lang.String")
        return cls(*args)


@_jcustomizer.JImplementationFor("java.lang.String")
class _JStringProto:
    def __add__(self, other: str) -> str:
        return self.concat(other)  # type: ignore[attr-defined]

    def __len__(self) -> int:
        return self.length()  # type: ignore[attr-defined]

    def __getitem__(self, i: typing.Union[slice, int]):
        if isinstance(i, slice):
            return str(self)[i]

        if i < 0:
            i += len(self)
            if i < 0:
                raise IndexError("Array index is negative")
        if i >= len(self):
            raise IndexError("Array index exceeds length")
        return self.charAt(i)  # type: ignore[attr-defined]

    def __contains__(self, other: str) -> bool:
        return self.contains(other)  # type: ignore[attr-defined]

    def __hash__(self):
        if self == None:  # lgtm [py/test-equals-none]
            return hash(None)
        return self.__str__().__hash__()

    def __repr__(self):
        return "'%s'" % self.__str__()


_jpype.JString = JString
