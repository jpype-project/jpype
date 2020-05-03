import _jpype
import datetime
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

if sys.version_info < (3, 8):
    from typing_extensions import Protocol, runtime_checkable
    from typing import Sequence, Mapping, SupportsFloat # lgtm [py/unused-import]

    @runtime_checkable
    class SupportsIndex(Protocol):
        def __index__(self) -> int: ...


else:
    # 3.8 onward
    from typing import Protocol, runtime_checkable
    from typing import SupportsIndex, SupportsFloat
    from typing import Sequence, Mapping

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


@_jcustomizer.JConversion("java.util.Collection", instanceof=Sequence,
        exclude=str)
def _JSequenceConvert(jcls, obj):
    return _jclass.JClass('java.util.Arrays').asList(obj)


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
    nsec = int((utc-sec)*1e9)
    return jcls.ofEpochSecond(sec, nsec)


if sys.version_info < (3, 6):
    import pathlib
    @_jcustomizer.JConversion("java.nio.file.Path", instanceof=pathlib.PurePath)
    def _JPathConvert(jcls, obj):
        Paths = _jpype.JClass("java.nio.file.Paths")
        return Paths.get(str(obj))

    @_jcustomizer.JConversion("java.io.File", instanceof=pathlib.PurePath)
    def _JFileConvert(jcls, obj):
        return jcls(str(obj))
