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
import inspect

import _jpype
from . import _jclass
from . import _jcustomizer

__all__ = ['JObject']

if _sys.version > '3':
    _unicode = str
    _long = int
else:
    _unicode = unicode
    _long = long


class JObject(_jpype.PyJPValue):
    """ Base class for all object instances.

    It can be used to test if an object is a java object instance with
    ``isinstance(obj, JObject)``.

    Calling ``JObject`` as a function can be used to covert or cast to 
    specific Java type.  It will box primitive types and supports an 
    option type to box to.

    This wrapper functions four ways. 

      - If the no type is given the object is automatically 
        cast to type best matched given the value.  This can be used 
        to create a boxed primitive.  ``JObject(JInt(i))``

      - If the type is a primitve, the object will be the boxed type of that
        primitive.  ``JObject(1, JInt)``

      - If the type is a Java class and the value is a Java object, the
        object will be cast to the Java class and will be an exact match to 
        the class for the purposes of matching arguments. If the object 
        is not compatible, an exception will be raised.

      - If the value is a python wrapper for class it will create a class
        instance.  This is aliased to be much more obvious as the ``class_`` 
        member of each Java class.

    Args:
       value: The value to be cast into an Java object.
       type(Optional, type): The type to cast into.  

    Raises:
       TypeError: If the object cannot be cast to the specified type, or
         the requested type is not a Java class or primitive.

    """
    __jvm__ = None
    def __new__(cls, *args, **kwargs):
         # Called from super
        if cls != cls.__jvm__.JObject:
            # Control will pass to __init__
            return super(JObject, cls).__new__(cls)

        # Create a null pointer object for JObject()
        if len(args) == 0:
            args = [None]

        # Called to create or cast.
        cls = _JObjectFactory(cls.__jvm__, *args, **kwargs)
        return cls.__javaclass__._cast(*args)

    def __setattr__(self, name, value):
        if name.startswith('_'):
            return object.__setattr__(self, name, value)

        if not hasattr(self, name):
            raise AttributeError("Field '%s' not found on Java '%s' object" %
                                 (name, self.__name__))

        try:
            attr = _jclass.typeLookup(type(self), name)
            if hasattr(attr, '__set__'):
                return attr.__set__(self, value)
        except AttributeError:
            pass
        raise AttributeError("Field '%s' is not settable on Java '%s' object" %
                             (name, self.__name__))

    def __unicode__(self):
        return self._toUnicode()

    def __hash__(self):
        return self.hashCode()

    def __eq__(self, other):
        return self.equals(other)

    def __ne__(self, other):
        return not self.equals(other)


# Post load dependencies
_jclass._JObject = JObject
_jcustomizer._JObject = JObject


def _JObjectFactory(jvm, v=None, tp=None):
    """ Creates a Java object.

    If not specified type is determined based on the object.
    If type type is specified then then it tried to box it.
    """
    cls = None

    if isinstance(v, type):
        if hasattr(v, '__javaclass__'):
            cls = _jclass.JClass("java.lang.Class").__javaclass__
        else:
            raise TypeError("%s is not a java class." % v)

    # Automatically look up the type if not specified,
    if tp is None:
        return _jclass._getDefaultJavaObject(jvm, v)

    # If it is a string look up the class name,
    elif isinstance(tp, (str, _unicode)):
        return jvm.JClass(tp)

    # Check if we are to box it,
    elif isinstance(tp, type):
        if hasattr(tp, '_java_boxed_class'):
            # FIXME we need to lookup the name in the jvm table.  
            return tp._java_boxed_class
        elif hasattr(tp, '__javaclass__'):
            return _jclass.JClass(tp.__javaclass__)

    raise TypeError("Invalid type conversion to %s requested." % tp)


