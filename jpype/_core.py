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

import _jpype

from . import _jclass
from . import _jobject
from . import _jstring
from . import _jtypes
from . import _classpath

# Import all the class customizers
# Customizers are applied in the order that they are defined currently.
#from . import _properties
from . import _jarray
from . import _jboxed
from . import _jexception
from . import _jcollection
from . import _jcomparable
from . import _jio
from ._jvmfinder import JVMNotFoundException, JVMNotSupportedException

__all__ = [
    'isJVMStarted', 'startJVM', 'shutdownJVM',
    'getDefaultJVMPath', 'getJVMVersion', 'isThreadAttachedToJVM', 'attachThreadToJVM',
    'detachThreadFromJVM', 'synchronized', 'get_default_jvm_path',
    'JVMNotFoundException', 'JVMNotSupportedException',
    'JBoolean', 'JByte', 'JChar', 'JShort',
    'JInt', 'JLong', 'JFloat', 'JDouble']

if _sys.version_info < (3,):
    raise ImportException("Python 2 is not supported")


# Activate jedi tab completion
try:
    import jedi as _jedi
    _jedi.evaluate.compiled.access.ALLOWED_DESCRIPTOR_ACCESS += \
        (_jpype.PyJPMethod, _jpype.PyJPField)
except:
    pass

# See http://scottlobdell.me/2015/04/decorators-arguments-python/


def deprecated(*args):
    """ Marks a function a deprecated when used as decorator.

    Be sure to start python with -Wd to see warnings.
    """
    def func2(*args, **kwargs):
        import warnings
        if not func2._warned:
            warnings.warn(func2._warning % (func2._real.__module__, func2._real.__name__),
                          category=DeprecationWarning, stacklevel=2)
        func2._warned = True
        return func2._real(*args, **kwargs)

    if isinstance(args[0], str):
        def decorate(func):
            func2.__name__ = func.__name__
            func2.__doc__ = func.__doc__
            func2._warned = False
            func2._real = func
            func2._warning = "%s.%s is deprecated, use {0} instead".format(
                args[0])
            return func2
        return decorate
    else:
        func = args[0]
        func2.__name__ = func.__name__
        func2.__doc__ = func.__doc__
        func2._warned = False
        func2._real = func
        func2._warning = "%s.%s is deprecated"
        return func2


def _hasClassPath(args):
    for i in args:
        if i.startswith('-Djava.class.path'):
            return True
    return False


def _handleClassPath(clsList):
    out = []
    for s in clsList:
        if not isinstance(s, str):
            raise TypeError("Classpath elements must be strings")
        if s.endswith('*'):
            import glob
            out.extend(glob.glob(s+'.jar'))
        else:
            out.append(s)
    return _classpath._SEP.join(out)


def synchronized(obj):
    """ Creates a resource lock for a Java object.

    Produces a monitor object. During the lifespan of the monitor the Java
    will not be able to acquire a thread lock on the object. This will
    prevent multiple threads from modifying a shared resource.

    This should always be used as part of a Python ``with`` startment.

    Arguments:
        obj: A valid Java object shared by multiple threads.

    Example:

    .. code-block:: python

      with synchronized(obj):
         # modify obj values

      # lock is freed when with block ends

    """
    try:
        return _jpype.PyJPMonitor(obj.__javavalue__)
    except AttributeError as ex:
        pass
    raise TypeError("synchronized only applies to java objects")


def getDefaultJVMPath():
    """
    Retrieves the path to the default or first found JVM library

    Returns:
      The path to the JVM shared library file

    Raises:
      JVMNotFoundException: If there was no JVM found in the search path.
      JVMNotSupportedException: If the JVM was found was not compatible with
        Python due to cpu architecture.

    """
    if _sys.platform == "cygwin":
        # Cygwin
        from ._cygwin import WindowsJVMFinder
        finder = WindowsJVMFinder()
    elif _sys.platform == "win32":
        # Windows
        from ._windows import WindowsJVMFinder
        finder = WindowsJVMFinder()
    elif _sys.platform == "darwin":
        # Mac OS X
        from ._darwin import DarwinJVMFinder
        finder = DarwinJVMFinder()
    else:
        # Use the Linux way for other systems
        from ._linux import LinuxJVMFinder
        finder = LinuxJVMFinder()

    return finder.get_jvm_path()


# Naming compatibility
@deprecated("getDefaultJVMPath")
def get_default_jvm_path(*args, **kwargs):
    return getDefaultJVMPath(*args, **kwargs)

# Primitive types are their own special classes as they do not tie to the JVM
JBoolean = _jtypes._JPrimitiveClass("boolean", 'Z', int)
JByte = _jtypes._JPrimitiveClass("byte", 'B', int)
JChar = _jtypes._JPrimitiveClass("char", 'C', int)
JShort = _jtypes._JPrimitiveClass("short", 'S', int)
JInt = _jtypes._JPrimitiveClass("int", 'I', int)
JLong = _jtypes._JPrimitiveClass("long", 'J', _long)
JFloat = _jtypes._JPrimitiveClass("float", 'F', float)
JDouble = _jtypes._JPrimitiveClass("double", 'D', float)


class JContext(_jpype.PyJPContext):

    def __init__(self):
        self._started = False

        # Create factories for these items
        self.JString = type("JString", (_jstring.JString,), {'__jvm__': self})
        self.JArray = type("JArray", (_jarray.JArray,), {'__jvm__': self})
        self.JClass = type("JClass", (_jclass.JClass,), {'__jvm__': self})
        self.JObject = type("JObject", (_jobject.JObject,), {'__jvm__': self})
        self.JException = type(
            "JException", (_jexception.JException,), {'__jvm__': self})

        # Basic types
        self.JBoolean = JBoolean
        self.JByte = JByte
        self.JChar = JChar
        self.JShort = JShort
        self.JInt = JInt
        self.JLong = JLong
        self.JFloat = JFloat
        self.JDouble = JDouble

        # Forward declarations
        self._type_classes = {}
        self._object_classes = {}
        self._primitive_types = {}

        self._java_lang_Object = None
        self._java_lang_Class = None
        _jpype.PyJPContext.__init__(self)

    def _initialize(self):
        """ Completes the initialization process
        
        This method instatiates all of the required classes for operation.
        """
        self._java_lang_Object = self.JClass("java.lang.Object")
        self._java_lang_Class = self.JClass("java.lang.Class")
        self._java_lang_String = self.JClass("java.lang.String")

#        self._java_lang_ClassLoader = self.JClass("java.lang.ClassLoader")
#        self._java_ClassLoader = self.JClass(
#            'java.lang.ClassLoader').getSystemClassLoader()
#        if not self._java_ClassLoader:
#            raise RuntimeError("Unable to load Java SystemClassLoader")

        self._java_lang_RuntimeException = self.JClass(
            "java.lang.RuntimeException")

        # Preload needed classes
        self._java_lang_Boolean = self.JClass("java.lang.Boolean")
        self._java_lang_Byte = self.JClass("java.lang.Byte")
        self._java_lang_Character = self.JClass("java.lang.Character")
        self._java_lang_Short = self.JClass("java.lang.Short")
        self._java_lang_Integer = self.JClass("java.lang.Integer")
        self._java_lang_Long = self.JClass("java.lang.Long")
        self._java_lang_Float = self.JClass("java.lang.Float")
        self._java_lang_Double = self.JClass("java.lang.Double")

        # Table for automatic conversion to objects "JObject(value, type)"
        self._object_classes[bool] = self._java_lang_Boolean
        self._object_classes[int] = self._java_lang_Long
        self._object_classes[float] = self._java_lang_Double
        self._object_classes[str] = self._java_lang_String
        self._object_classes[type] = self._java_lang_Class
        self._object_classes[_jpype.PyJPClass] = self._java_lang_Class
        self._object_classes[object] = self._java_lang_Object
        self._object_classes[self.JBoolean] = self._java_lang_Boolean
        self._object_classes[self.JByte] = self._java_lang_Byte
        self._object_classes[self.JChar] = self._java_lang_Character
        self._object_classes[self.JShort] = jself._ava_lang_Short
        self._object_classes[self.JInteger] = self._java_lang_Integer
        self._object_classes[self.JLong] = self._java_lang_Long
        self._object_classes[self.JFloat] = self._java_lang_Float
        self._object_classes[self.JDouble] = self._java_lang_Double
        self._object_classes[type(None)] = self._java_lang_Object

        # Set up table of automatic conversions of Python primitives
        # this table supports "JArray(type)"
        self._type_classes[bool] = JBoolean
        self._type_classes[int] = JLong
        self._type_classes[float] = JDouble
        self._type_classes[str] = self._java_lang_String
        self._type_classes[type] = self._java_lang_Class
        self._type_classes[object] = self._java_lang_Object
        self._type_classes[self.JString] = self._java_lang_String
        self._type_classes[self.JObject] = self._java_lang_Object

    def startJVM(self, *args, **kwargs):
        """
        Starts a Java Virtual Machine.  Without options it will start
        the JVM with the default classpath and jvmpath.

        The default classpath is determined by ``jpype.getClassPath()``.
        The default jvmpath is determined by ``jpype.getDefaultJVMPath()``.

        Parameters:
         *args (Optional, str[]): Arguments to give to the JVM.
            The first argument may be the path the JVM.

        Keyword Arguments:
          jvmpath (str):  Path to the jvm library file,
            Typically one of (``libjvm.so``, ``jvm.dll``, ...)
            Using None will apply the default jvmpath.
          classpath (str,[str]): Set the classpath for the jvm.
            This will override any classpath supplied in the arguments
            list. A value of None will give no classpath to JVM.
          ignoreUnrecognized (bool): Option to JVM to ignore
            invalid JVM arguments. Default is False.
          convertStrings (bool): Option to JPype to force Java strings to
            cast to Python strings. This option is to support legacy code
            for which conversion of Python strings was the default. This
            will globally change the behavior of all calls using
            strings, and a value of True is NOT recommended for newly 
            developed code.

            The default value for this option during 0.7 series is 
            True.  The option will be False starting in 0.8. A
            warning will be issued if this option is not specified
            during the transition period.


        Raises:
          OSError: if the JVM cannot be started or is already running.
          TypeError: if an invalid keyword argument is supplied
            or a keyword argument conflicts with the arguments.

         """
        if self.isStarted():
            raise OSError('JVM is already started')

        if self._started:
            raise OSError('JVM cannot be restarted')
        self._started = True

        args = list(args)

        # JVM path
        jvmpath = None
        if args:
            # jvm is the first argument the first argument is a path or None
            if not args[0] or not args[0].startswith('-'):
                jvmpath = args.pop(0)
        if 'jvmpath' in kwargs:
            if jvmpath:
                raise TypeError('jvmpath specified twice')
            jvmpath = kwargs.pop('jvmpath')
        if not jvmpath:
            jvmpath = getDefaultJVMPath()

        # Classpath handling
        if _hasClassPath(args):
            # Old style, specified in the arguments
            if 'classpath' in kwargs:
                # Cannot apply both styles, conflict
                raise TypeError('classpath specified twice')
            classpath = None
        elif 'classpath' in kwargs:
            # New style, as a keywork
            classpath = kwargs.pop('classpath')
        else:
            # Not speficied at all, use the default classpath
            classpath = _classpath.getClassPath()

        # Handle strings and list of strings.
        if classpath:
            if isinstance(classpath, str):
                args.append('-Djava.class.path=%s' %
                            _handleClassPath([classpath]))
            elif hasattr(classpath, '__iter__'):
                args.append('-Djava.class.path=%s' %
                            _handleClassPath(classpath))
            else:
                raise TypeError("Unknown class path element")

        ignoreUnrecognized = kwargs.pop('ignoreUnrecognized', False)

        if not "convertStrings" in kwargs:
            import warnings
            warnings.warn("""
-------------------------------------------------------------------------------
Deprecated: convertStrings was not specified when starting the JVM. The default
behavior in JPype will be False starting in JPype 0.8. The recommended setting
for new code is convertStrings=False.  The legacy value of True was assumed for
this session. If you are a user of an application that reported this warning,
please file a ticket with the developer.
-------------------------------------------------------------------------------
""")

        convertStrings = kwargs.pop('convertStrings', True)

        if kwargs:
            raise TypeError("startJVM() got an unexpected keyword argument '%s'"
                            % (','.join([str(i) for i in kwargs])))

        self._startup(jvmpath, tuple(args), ignoreUnrecognized, convertStrings)
        self._initialize()
        return self

    def getJVMVersion(self):
        """ Get the JVM version if the JVM is started.

        This function can be used to determine the version of the JVM. It is
        useful to help determine why a Jar has failed to load.

        Returns:
          A typle with the (major, minor, revison) of the JVM if running.
        """
        if not self.isStarted():
            return (0, 0, 0)

        import re
        runtime = _jclass.JClass('java.lang.Runtime')
        version = runtime.class_.getPackage().getImplementationVersion()

        # Java 9+ has a version method
        if not version:
            version = runtime.version()
        version = (re.match("([0-9.]+)", str(version)).group(1))
        return tuple([int(i) for i in version.split('.')])


# Connect the default JVM to the types.
_jpype._jvm = JContext()
_jobject.JObject.__jvm__ = _jpype._jvm
_jarray.JArray.__jvm__ = _jpype._jvm
_jclass.JClass.__jvm__ = _jpype._jvm
_jexception.JException.__jvm__ = _jpype._jvm

# Export symbols from the default JVM 
startJVM = _jpype._jvm.startJVM
shutdownJVM = _jpype._jvm.shutdown
isJVMStarted = _jpype._jvm.isStarted
getJVMVersion = _jpype._jvm.getJVMVersion
attachThreadToJVM = _jpype._jvm.attachThread
detachThreadFromJVM = _jpype._jvm.detachThread
isThreadAttachedToJVM = _jpype._jvm.isThreadAttached

