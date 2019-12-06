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
    def __new__(cls, *args, **kwargs):
         # Called from super
        if cls != JObject:
            # Control will pass to __init__
            return super(JObject, cls).__new__(cls)

        # Create a null pointer object for JObject()
        if len(args) == 0:
            args = [None]

        # Called to create or cast.
        cls = _JObjectFactory(*args, **kwargs)
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

    def __hash__(self):
        return self.hashCode()

    def __eq__(self, other):
        return self.equals(other)

    def __ne__(self, other):
        return not self.equals(other)


# Post load dependencies
_jpype.JObject = JObject


def _getDefaultAJavaObject(tp):
    """ Determine the type of the object based the type of a value.
        
        Python primitives - lookup the type in the table
        Java primitive - lookup boxed type in the table
        Java objects - just use their type directly


    """
    # handle Python types and Java primitives
    try:
        return _jpype._object_classes[tp]
    except KeyError:
        pass

    if issubclass(obj, _jpype.PyJPClass):
        return _jpype._java_lang_Class
  
    # JObject(JClass("str"))
    if issubclass(obj, _jpype.PyJPClassMeta):
        return _jpype._java_lang_Class

    if issubclass(obj, _jpype.PyJPValueBase):
        return obj.__javaclass__

    raise TypeError(
        "Unable to determine the default type of `{0}`".format(obj.__class__))


def _JObjectFactory(v=None, tp=None):
    """ Creates a Java object.

    If not specified type is determined based on the object.
    If type type is specified then then it tried to box it.
    """
    if tp is None:
        # Automatically determine based on the value
        tp = _getDefaultAJavaObject(type(v))

    # Java primitive conversion to boxed type
    if isinstance(tp, _jpype._JPrimitive):
        return _getDefaultAJavaObject(type(tp))(v)

    # Given a Already an object
    if isinstance(tp, _jpype.PyJPClassMeta):
        return tp.__javaclass__._cast(v)

    # Given a Java class
        return tp._cast(v)

    raise TypeError("Invalid type conversion to %s requested." % tp)


