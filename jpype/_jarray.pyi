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


__all__ = ['JArray']


T = typing.TypeVar('T')
U = typing.TypeVar('U')


class JArray(typing.Generic[T]):

    def __new__(cls, tp: typing.Type[T], dims=1) -> JArray[T]:
        ...

    @classmethod
    def of(cls, array, dtype: typing.Optional[typing.Type[U]] = None) -> JArray[U]:
        ...

    def __len__(self) -> int:
        ...

    def __iter__(self) -> typing.Iterator[T]:
        ...

    def __reversed__(self) -> typing.Iterator[T]:
        ...

    @typing.overload
    def __getitem__(self, key: int) -> T:
        ...

    @typing.overload
    def __getitem__(self, key: slice) -> JArray[T]:
        ...

    @typing.overload
    def __setitem__(self, index: int, value: T):
        ...

    @typing.overload
    def __setitem__(self, index: slice, value: typing.Sequence[T]):
        ...
