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
from . import _jinit

__all__ = ['JObject']

if _sys.version > '3':
    _unicode = str
    _long = int
else:
    _unicode = unicode
    _long = long


def _initialize():
    type.__setattr__(JObject, '__javaclass__',
                     _jpype.PyJPClass('java.lang.Object'))


class JObject(object):
    """ Base class for all object instances.

    It can be used to test if an object is a java object instance with
    isinstance(obj, JObject).

    Calling JObject as a function can be used to covert or cast to specific java
    type.  It will box primitive types and supports an option type to box to.

        JObject(value) - where value is a simple python primitive type like int, float, 
          str, bool, string converts to an automatically determine boxed type.

        JObject(cls) - where cls is an existing java class will produce a java.lang.Class instance
          equivalent to c.class_

        JObject(javaObj, objectClass) - specifically cast an object to another class to
          match a specified overload.  This may return a TypeError if the object is 
          not assignable to the specified class.

        JObject(value, primitiveClass) - specify a specific primitive wrapper type
          for the conversion to a boxed class.

    Using these conversion can be useful match to an otherwise ambiguous overload
    or pack a specific type into a generic container.
    """
    def __new__(cls, *args, **kwargs):
        if cls != JObject:
            return super(JObject, cls).__new__(cls)
        # Create a null pointer object
        if len(args) == 0:
            args = [None]
        cls = _JObjectFactory(*args, **kwargs)
        self = cls.__new__(cls, args[0])
        self.__javavalue__ = _jpype.PyJPValue(cls.__javaclass__, args[0])
        return self

    def __init__(self, *args):
        if len(args) == 1 and isinstance(args[0], _jpype.PyJPValue):
            object.__setattr__(self, '__javavalue__', args[0])
        elif not hasattr(self, '__javavalue__'):
            jv = self.__class__.__javaclass__.newInstance(*args)
            object.__setattr__(self, '__javavalue__', jv)
        super(JObject, self).__init__()

    def __setattr__(self, name, value):
        if name.startswith('_'):
            object.__setattr__(self, name, value)
            return
        attr = _jclass.typeLookup(type(self), name)
        if hasattr(attr, '__set__'):
            attr.__set__(self, value)
            return
        raise AttributeError("%s does not have field %s" %
                             (self.__name__, name))

    def __str__(self):
        return self.__javavalue__.toString()

    def __hash__(self):
        return self.hashCode()

    def __eq__(self, other):
        return self.equals(other)

    def __ne__(self, other):
        return not self.equals(other)


_jclass._JObject = JObject


def _JObjectFactory(v=None, tp=None):
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
        return _jclass._getDefaultJavaObject(v)

    # If it is a string look up the class name,
    elif isinstance(tp, (str, _unicode)):
        return _jclass.JClass(tp)

    # Check if we are to box it,
    elif isinstance(tp, type):
        if hasattr(tp, '_java_boxed_class'):
            return tp._java_boxed_class
        elif hasattr(tp, '__javaclass__'):
            return _jclass.JClass(tp.__javaclass__)

    raise TypeError("Invalid type conversion to %s requested." % tp)


def defineJObjectFactory(name, jclass, proto, bases=(JObject,), members=None):
    """ Create a factory type such as JObject or JArray.

    Args:
        name (str) - Name of the class to produce
        jclass (str) - Name of the java class this should shadow.
        proto (type) - Is a type from which the class methods will be based.
        bases (tuple) - Bases for this meta class factory.
        members (dict) - Any additional members for this class.

    """
    # Copy the members from the prototype
    if members == None:
        members = {}
    for p, v in proto.__dict__.items():
        if isinstance(v, (str)):
            members[p] = v
        elif callable(v):
            members[p] = v
        elif p == "__new__":
            members[p] = v

    res = None

    if jclass != None:
        members['__javaclass__'] = None

    # Create a new class
    res = _jclass.JClass(name, bases, members)

    if jclass != None:
        # Register this class to be initialized when jvm starts
        def jinit():
            type.__setattr__(res, '__javaclass__', _jpype.PyJPClass(jclass))
        _jinit.registerJVMInitializer(jinit)

    return res
