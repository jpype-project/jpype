import _jpype
import datetime
from typing import Protocol, Sequence, Mapping, runtime_checkable, abstractmethod

import sys
import _jpype
from . import _jclass
from . import _jcustomizer
from . import types as _jtypes

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

# Types we need
@runtime_checkable
class SupportsPath(Protocol):
    """An ABC with one abstract method __fspath__."""
    __slots__ = ()

    @abstractmethod
    def __fspath__(self) -> str:
        pass

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


@_jcustomizer.JConversion("java.nio.file.Path", protocol=SupportsPath)
def _JPathConvert(jcls, obj):
    Paths = _jpype.JClass("java.nio.file.Paths")
    return Paths.get(obj.__fspath__())


@_jcustomizer.JConversion("java.io.File", protocol=SupportsPath)
def _JFileConvert(jcls, obj):
    return jcls(obj.__fspath__())


@_jcustomizer.JConversion("java.util.Collection", protocol=Sequence)
def _JSequenceConvert(jcls, obj):
    return _jclass.JClass('java.util.Arrays').asList(obj)


@_jcustomizer.JConversion("java.util.Map", protocol=Mapping)
def _JMapConvert(jcls, obj):
    hm = _jclass.JClass('java.util.HashMap')()
    for p, v in obj.items():
        hm[p] = v
    return hm
