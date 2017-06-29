import jpype
from ._jclass import _JavaObject
from ._jclass import _isJavaCtor
from ._jwrapper import _JWrapper
import sys as _sys

#Python2/3 support
if _sys.version_info > (3,):
    _long  =  int
else:
    _long  =  long

__all__=[]
   
# Temporary JavaObject wrapper to call Java Methods in __new__
class _JTmp(_JavaObject):
    def __init__(self, jp):
        self.__javaobject__ = jp

class _JBoxedShort(int):
    __metaclass__ = int.__class__
    def __new__(cls, *args, **kwargs):
        try:
            if isinstance(args[0], (int, _long)):
                return int.__new__(cls, args[0])
            if isinstance(args[0], _JWrapper):
                return int.__new__(cls, args[0]._pyv)
        except OverflowError:
            raise TypeError("Overflow")
        if _isJavaCtor(args):
            return int.__new__(cls, cls.shortValue(_JTmp(args[0][1])))
        raise ValueError("Invalid arguments %s"%args[0])

class _JBoxedInteger(int):
    __metaclass__ = int.__class__
    def __new__(cls, *args, **kwargs):
        try:
            if isinstance(args[0], (int, _long)):
                return int.__new__(cls, args[0])
            if isinstance(args[0], _JWrapper):
                return int.__new__(cls, args[0]._pyv)
        except OverflowError:
            raise TypeError("Overflow")
        if _isJavaCtor(args):
            return int.__new__(cls, cls.intValue(_JTmp(args[0][1])))
        raise ValueError("Invalid arguments")

class _JBoxedLong(_long):
    __metaclass__ = _long.__class__
    def __new__(cls, *args, **kwargs):
        if isinstance(args[0], (int, _long)):
            return _long.__new__(cls, args[0])
        if isinstance(args[0], _JWrapper):
            return _long.__new__(cls, args[0]._pyv)
        if _isJavaCtor(args):
            return _long.__new__(cls, cls.longValue(_JTmp(args[0][1])))
        raise ValueError("Invalid arguments")

class _JBoxedFloat(float):
    __metaclass__ = float.__class__
    def __new__(cls, *args, **kwargs):
        if isinstance(args[0], (int, _long,float)):
            return float.__new__(cls, args[0])
        if isinstance(args[0], _JWrapper):
            return int.__new__(cls, args[0]._pyv)
        if _isJavaCtor(args):
            return float.__new__(cls, cls.floatValue(_JTmp(args[0][1])))
        raise ValueError("Invalid arguments")

class _JBoxedDouble(float):
    __metaclass__ = float.__class__
    def __new__(cls, *args, **kwargs):
        if isinstance(args[0], (int, _long,float)):
            return float.__new__(cls, args[0])
        if isinstance(args[0], _JWrapper):
            return int.__new__(cls, args[0]._pyv)
        if _isJavaCtor(args):
            return float.__new__(cls, cls.doubleValue(_JTmp(args[0][1])))
        raise ValueError("Invalid arguments")

_BOXED = {
        'java.lang.Short': _JBoxedShort,
        'java.lang.Integer': _JBoxedInteger,
        'java.lang.Long': _JBoxedLong,
        'java.lang.Float': _JBoxedFloat,
        'java.lang.Double': _JBoxedDouble,
        }

class _BoxedCustomizer(object):
    def canCustomize(self, name, jc):
        if name in _BOXED:
            return True
        return False

    def customize(self, name, jc, bases, members):
        cls=_BOXED[name]
        bases.append(cls)
        members['__eq__']=cls.__eq__
        members['__ne__']=cls.__ne__
        if hasattr(cls, '__cmp__'):
            members['__cmp__']=cls.__cmp__

def _initialize():
    from ._jclass import registerClassCustomizer
    registerClassCustomizer(_BoxedCustomizer())

_initialize()
