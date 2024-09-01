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
import abc
import collections.abc
import typing


__all__ = ['JArray']


T = typing.TypeVar('T')


class _JArrayGeneric(collections.abc.Sequence[T]):

    @abc.abstractmethod
    def __setitem__(self, index, value):
        ...


class JArray(_JArrayGeneric[T], metaclass=abc.ABCMeta):

    def __new__(cls, tp, dims=1):
        ...

    @classmethod
    def of(cls, array, dtype=None):
        ...
