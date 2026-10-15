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
# codeql[py/equals-hash-mismatch]
class _JStringProto:
    # No __eq__ here deliberately: JImplementationFor mixes this class's
    # __hash__ into the real wrapped-String type alongside _jpype._JObject,
    # which supplies a content-based (java.lang.String.equals) __eq__ -
    # confirmed consistent by direct test: two distinct String objects with
    # equal content (`a is b` False) compare equal and hash equal, including
    # against a plain Python str of the same content.
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
        # Deliberately == not is: this checks whether the wrapped Java
        # string is null, not Python identity (self can never literally be
        # the Python None singleton here).
        # codeql[py/test-equals-none]
        if self == None:
            return hash(None)
        return self.__str__().__hash__()

    def __repr__(self):
        return "'%s'" % self.__str__()


_jpype.JString = JString
