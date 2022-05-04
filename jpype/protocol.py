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
import _jpype
import datetime
import decimal
import sys
import _jpype
from . import _jclass
from . import _jcustomizer

# Copies of all private base types for reference
_JClass = _jpype._JClass
_JObject = _jpype._JObject
_JException = _jpype._JException
_JNumberLong = _jpype._JNumberLong
_JNumberFloat = _jpype._JNumberFloat
_JComparable = _jpype._JComparable
_JChar = _jpype._JChar
_JBoolean = _jpype._JBoolean
_JArray = _jpype._JArray
_JBuffer = _jpype._JBuffer

if sys.version_info < (3, 8):  # pragma: no cover
    from typing_extensions import Protocol, runtime_checkable
    from typing import Sequence, Mapping, Set  # lgtm [py/unused-import]
    from typing import SupportsFloat, Callable  # lgtm [py/unused-import]

    @runtime_checkable
    class SupportsIndex(Protocol):
        def __index__(self) -> int: ...


else:
    # 3.8 onward
    from typing import Protocol, runtime_checkable
    from typing import SupportsIndex, SupportsFloat  # lgtm [py/unused-import]
    from typing import Sequence, Mapping, Set, Callable  # lgtm [py/unused-import]

# Types we need


@runtime_checkable
class SupportsPath(Protocol):
    def __fspath__(self) -> str: ...


@_jcustomizer.JConversion("java.nio.file.Path", instanceof=SupportsPath)
def _JPathConvert(jcls, obj):
    Paths = _jpype.JClass("java.nio.file.Paths")
    return Paths.get(obj.__fspath__())


@_jcustomizer.JConversion("java.io.File", instanceof=SupportsPath)
def _JFileConvert(jcls, obj):
    return jcls(obj.__fspath__())

# To be added in 1.1.x


@_jcustomizer.JConversion("java.lang.Iterable", instanceof=Sequence, excludes=str)
@_jcustomizer.JConversion("java.util.Collection", instanceof=Sequence, excludes=str)
def _JSequenceConvert(jcls, obj):
    return _jclass.JClass('java.util.Arrays').asList(obj)


@_jcustomizer.JConversion("java.lang.Iterable", instanceof=Set)
@_jcustomizer.JConversion("java.util.Collection", instanceof=Set)
def _JSetConvert(jcls, obj):
    # set does not satisfy PySequence_Check and collection is too broad as it
    # would let dict be converted, so we are going to have to convert twice
    # for now
    return _jclass.JClass('java.util.Arrays').asList(list(obj))


@_jcustomizer.JConversion("java.util.Map", instanceof=Mapping)
def _JMapConvert(jcls, obj):
    hm = _jclass.JClass('java.util.HashMap')()
    for p, v in obj.items():
        hm[p] = v
    return hm

# Converters start here


@_jcustomizer.JConversion("java.time.Instant", exact=datetime.datetime)
def _JInstantConversion(jcls, obj):
    utc = obj.replace(tzinfo=datetime.timezone.utc).timestamp()
    sec = int(utc)
    nsec = int((utc - sec) * 1e9)
    return jcls.ofEpochSecond(sec, nsec)


# Types needed for SQL


@_jcustomizer.JImplementationFor('java.sql.Date')
class _JSQLDate:
    def _py(self):
        return datetime.date(self.getYear() + 1900, self.getMonth() + 1, self.getDate())


@_jcustomizer.JImplementationFor('java.sql.Time')
class _JSQLTime:
    def _py(self):
        return datetime.time(self.getHours(), self.getMinutes(), self.getSeconds())


@_jcustomizer.JImplementationFor('java.sql.Timestamp')
class _JDate:
    def _py(self):
        return datetime.datetime(self.getYear() + 1900, self.getMonth() + 1, self.getDate(),
                                 self.getHours(), self.getMinutes(), self.getSeconds(), self.getNanos() // 1000)


@_jcustomizer.JImplementationFor('java.math.BigDecimal')
class _JBigDecimal:
    def _py(self):
        return decimal.Decimal(str(self))


@_jcustomizer.JConversion("java.sql.Time", instanceof=datetime.time)
def _toTime(jcls, x):
    return jcls(x.hour, x.minute, x.second)


@_jcustomizer.JConversion("java.sql.Date", instanceof=datetime.date)
def _toDate(jcls, x):
    return jcls(x.year - 1900, x.month - 1, x.day)


@_jcustomizer.JConversion("java.sql.Timestamp", instanceof=datetime.datetime)
def _toTimestamp(jcls, x):
    return jcls(x.year - 1900, x.month - 1, x.day, x.hour, x.minute, x.second, x.microsecond * 1000)


@_jcustomizer.JConversion("java.math.BigDecimal", instanceof=decimal.Decimal)
def _toBigDecimal(jcls, x):
    return jcls(str(x))
