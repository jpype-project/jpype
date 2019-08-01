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

import collections
import sys

__all__ = ["JProxy", "JImplements"]

import _jpype
from . import _jclass

if sys.version > '3':
    unicode = str

# FIXME the java.lang.method we are overriding should be passes to the lookup function
# so we can properly handle name mangling on the override.


def _createJProxy(cls, *intf, **kwargs):
    """ (internal) Create a proxy from a python class with
    @JOverride notation on methods.
    """
    # Convert the interfaces list
    actualIntf = _convertInterfaces(intf)

    # Find all class defined overrides
    overrides = {}
    for k, v in cls.__dict__.items():
        try:
            attr = object.__getattribute__(v, "__joverride__")
            overrides[k] = (v, attr)
        except AttributeError:
            pass

    # Verify all methods are overriden
    for interface in actualIntf:
        for method in interface.class_.getMethods():
            if method.getModifiers() & 1024 == 0:
                continue
            if not str(method.getName()) in overrides:
                raise NotImplementedError("Interface '%s' requires method '%s' to be implemented." % (
                    interface.class_.getName(), method.getName()))

    # Define a lookup interface
    def lookup(self, name):
        # Get the override from the override dictionary
        over = overrides[name]

        # We need convert the method to a bound method using descriptor interface
        return over[0].__get__(self)

    # Construct a new init method
    init = cls.__dict__.get('__init__', None)
    if init:
        def init2(self, *args, **kwargs):
            init(self, *args, **kwargs)
            self.__javaproxy__ = _jpype.PyJPProxy(self, lookup, actualIntf)
    else:
        def init2(self, *args, **kwargs):
            self.__javaproxy__ = _jpype.PyJPProxy(self, lookup, actualIntf)

    # Replace the init with the proxy init
    type.__setattr__(cls, '__init__', init2)

    # Return the augmented class
    return cls


def JImplements(*args, **kwargs):
    """ Annotation for creating a new proxy that implements a list of
    Java interfaces.

    This annotation is placed on an ordinary Python class.  The annotation
    requires a list of interfaces.  It must implement all of the java
    methods for each of the interfaces.  Each implemented method
    should have a @JOverride annotation.

    Args:
      interfaces (str*,JClass*): Strings or JClasses for each Java interface
        this proxy is to implement.

    Example:

      .. code-block:: python

          @JImplement("java.lang.Runnable")
          class MyImpl(object):
             @JOverride
             def run(self, arg):
               pass

          @JImplement("org.my.Interface1", "org.my.Interface2")
          class MyImpl(object):
             @JOverride
             def method(self, arg):
               pass

    """
    def JProxyCreator(cls):
        return _createJProxy(cls, *args, **kwargs)
    return JProxyCreator


def _convertInterfaces(intf):
    """ (internal) Convert a list of interface names into
    a list of interfaces suitable for a proxy.
    """
    # Flatten the list
    intflist = []
    for item in intf:
        if isinstance(item, (str, unicode)) or not hasattr(item, '__iter__'):
            intflist.append(item)
        else:
            intflist.extend(item)

    # Look up the classes if given as a string
    actualIntf = set()
    for item in intflist:
        if isinstance(item, (str, unicode)):
            actualIntf.add(_jclass.JClass(item))
        else:
            actualIntf.add(item)

    # Check that all are interfaces
    if not actualIntf:
        raise TypeError("At least one Java interface must be specified")

    for cls in actualIntf:
        # If it isn't a JClass, then it cannot be a Java interface
        if not isinstance(cls, _jclass.JClass):
            raise TypeError("'%s' is not a Java interface"%type(cls).__name__)
        # Java concrete and abstract classes cannot be proxied
        if not issubclass(cls, _jclass.JInterface):
            raise TypeError("'%s' is not a Java interface"%cls.__name__)

    return tuple(actualIntf)


class JProxy(object):
    """ Define a proxy for a Java interface.

    This is an older style JPype proxy interface that uses either a
    dictionary or an object instance to implement methods defined
    in java.  The python object can be held by java and its lifespan
    will continue as long as java holds a reference to the object
    instance.  New code should use ``@JImplements`` annotation as
    it will support improved type safety and error handling.

    Name lookups can either made using a dictionary or an object
    instance.  One of these two options must be specified.

    Args:
        intf: either a single interface or a list of java interfaces.
            The interfaces can either be defined by strings or
            JClass instance.  Only interfaces may be used in a
            proxy,
        dict (dict[string, callable], optional): specifies a dictionary
            containing the methods to be called when executing the
            java interface methods.
        inst (object, optional): specifies an object with methods
            whose names matches the java interfaces methods.
    """

    def __init__(self, intf, dict=None, inst=None):
        # Convert the interfaces
        actualIntf = _convertInterfaces([intf])

        # Verify that one of the options has been selected
        if dict is not None and inst is not None:
            raise RuntimeError("Specify only one of dict and inst")

        if dict is not None:
            # Define the lookup function based for a dict
            def lookup(d, name):
                return d[name]
            # create a proxy
            self.__javaproxy__ = _jpype.PyJPProxy(dict, lookup, actualIntf)
            return

        if inst is not None:
            # Define the lookup function based for a object instance
            def lookup(d, name):
                return getattr(d, name)
            # create a proxy
            self.__javaproxy__ = _jpype.PyJPProxy(inst, lookup, actualIntf)
            return
        raise TypeError("a dict or inst must be specified")
