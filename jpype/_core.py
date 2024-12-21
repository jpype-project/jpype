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
from __future__ import annotations

import atexit
import os
from pathlib import Path
import sys
import typing

import _jpype
from . import types as _jtypes
from . import _classpath
from . import _jcustomizer
from . import _jinit
from . import _pykeywords

from ._jvmfinder import *

# This import is required to bootstrap importlib, _jpype uses importlib.util
# but on some systems it may not load properly from C.  To make sure it gets
# loaded properly we are going to force the issue even through we don't
# use it in this module.  (Do not remove)
from importlib import util as _util

__all__ = [
    'isJVMStarted', 'startJVM', 'shutdownJVM',
    'getDefaultJVMPath', 'getJVMVersion', 'isThreadAttachedToJVM', 'attachThreadToJVM',
    'detachThreadFromJVM', 'synchronized',
    'JVMNotFoundException', 'JVMNotSupportedException', 'JVMNotRunning'
]


class JVMNotRunning(RuntimeError):
    pass


# Activate jedi tab completion
try:
    from jedi import __version__ as _jedi_version
    import jedi.access as _jedi_access
    _jedi_access.ALLOWED_DESCRIPTOR_ACCESS += _jpype._JMethod, _jpype._JField
except ModuleNotFoundError:
    pass
except AttributeError:
    import warnings as _w
    _w.warn(f"provided Jedi seems out of date. Version is {_jedi_version}.")


if typing.TYPE_CHECKING:
    _PathOrStr = typing.Union[str, os.PathLike]


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


def isJVMStarted():
    """ True if the JVM is currently running."""
    # TODO This method is horribly named.  It should be named isJVMRunning as
    # isJVMStarted would seem to imply that the JVM was started at some
    # point without regard to whether it has been shutdown.
    #
    return _jpype.isStarted()


def _getOption(args, var, sep=None, keep=False):
    """ Get an option and remove it from the current jvm arguments list """
    for i, v in enumerate(args):
        if not isinstance(v, str):
            continue
        _, _, value = v.partition('%s=' % var)
        if value:
            if not keep:
                del args[i]
            if sep is not None:
                return value.split(sep)
            return value
    return []


def _expandClassPath(
    classpath: typing.Union[typing.Sequence[_PathOrStr],
                            _PathOrStr, None] = None
) -> typing.List[str]:
    """
    Return a classpath which represents the given tuple of classpath specifications
    """
    out: list[str] = []
    if isinstance(classpath, (str, os.PathLike)):
        classpath = (classpath,)
    try:
        # Convert anything iterable into a tuple.
        classpath = tuple(classpath)  # type: ignore[arg-type]
    except TypeError:
        raise TypeError("Unknown class path element")

    for element in classpath:
        try:
            pth = os.fspath(element)
        except TypeError as err:
            raise TypeError(
                "Classpath elements must be strings or Path-like") from err

        if isinstance(pth, bytes):
            # In the future we may allow this to support Paths which are undecodable.
            # https://docs.python.org/3/howto/unicode.html#unicode-filenames.
            raise TypeError("Classpath elements must be strings or Path-like")

        if pth.endswith('*'):
            import glob
            out.extend(glob.glob(pth + '.jar'))
        else:
            out.append(pth)
    return out


_JVM_started = False


def interactive():
    return bool(getattr(sys, 'ps1', sys.flags.interactive))


def _findTemp():
    dirlist = []
    # Mirror Python tempfile with a check for ascii
    for envname in 'TMPDIR', 'TEMP', 'TMP':
        dirname = os.getenv(envname)
        if dirname and dirname.isascii():
            dirlist.append(dirname)
    if os.name == 'nt':
        for dirname in [os.path.expanduser(r'~\AppData\Local\Temp'),
                        os.path.expandvars(r'%SYSTEMROOT%\Temp'),
                        r'c:\temp', r'c:\tmp', r'\temp', r'\tmp']:
            if dirname and dirname.isascii():
                dirlist.append(dirname)
    else:
        dirlist.extend(['/tmp', '/var/tmp', '/usr/tmp'])

    name = str(os.getpid())
    for d in dirlist:
        p = Path("%s/%s" % (d, name))
        try:
            p.touch()
            p.unlink()
        except Exception as ex:
            continue
        return d
    raise SystemError("Unable to find non-ansii path")


def startJVM(
    *jvmargs: str,
    jvmpath: typing.Optional[_PathOrStr] = None,
    classpath: typing.Union[typing.Sequence[_PathOrStr],
                            _PathOrStr, None] = None,
    ignoreUnrecognized: bool = False,
    convertStrings: bool = False,
    interrupt: bool = not interactive(),
) -> None:
    """
    Starts a Java Virtual Machine.  Without options it will start
    the JVM with the default classpath and jvmpath.

    The default classpath is determined by ``jpype.getClassPath()``.
    The default jvmpath is determined by ``jpype.getDefaultJVMPath()``.

    Parameters:
     *jvmargs (Optional, str[]): Arguments to give to the JVM.
        The first argument may be the path to the JVM.

    Keyword Arguments:
      jvmpath (str, PathLike):  Path to the jvm library file,
        Typically one of (``libjvm.so``, ``jvm.dll``, ...)
        Using None will apply the default jvmpath.
      classpath (str, PathLike, [str, PathLike]): Set the classpath for the JVM.
        This will override any classpath supplied in the arguments
        list. A value of None will give no classpath to JVM.
      ignoreUnrecognized (bool): Option to ignore
        invalid JVM arguments. Default is False.
      convertStrings (bool): Option to force Java strings to
        cast to Python strings. This option is to support legacy code
        for which conversion of Python strings was the default. This
        will globally change the behavior of all calls using
        strings, and a value of True is NOT recommended for newly
        developed code.
      interrupt (bool): Option to install ^C signal handlers.
        If True then ^C will stop the process, else ^C will
        transfer control to Python rather than halting.  If
        not specified will be False if Python is started as
        an interactive shell.

    Raises:
      OSError: if the JVM cannot be started or is already running.
      TypeError: if a keyword argument conflicts with the positional arguments.

     """

# Code for 1.6
#    modulepath: typing.Union[typing.Sequence[_PathOrStr], _PathOrStr, None] = None,

    if _jpype.isStarted():
        raise OSError('JVM is already started')
    global _JVM_started
    if _JVM_started:
        raise OSError('JVM cannot be restarted')

    # Convert to list
    jvm_args: list[str] = list(jvmargs)

    # JVM path
    if jvm_args:
        # jvm is the first argument the first argument is a path or None
        if jvm_args[0] is None or (isinstance(jvm_args[0], str) and not jvm_args[0].startswith('-')):
            if jvmpath:
                raise TypeError('jvmpath specified twice')
            jvmpath = jvm_args[0]
            jvm_args = jvm_args[1:]

    if not jvmpath:
        jvmpath = getDefaultJVMPath()
    else:
        # Allow the path to be a PathLike.
        jvmpath = os.fspath(jvmpath)

    # Handle strings and list of strings.
    extra_jvm_args: list[str] = []

    # Classpath handling
    old_classpath = _getOption(jvm_args, "-Djava.class.path", _classpath._SEP)
    if old_classpath:
        # Old style, specified in the arguments
        if classpath is not None:
            # Cannot apply both styles, conflict
            raise TypeError('classpath specified twice')
        classpath = old_classpath
    elif classpath is None:
        # Not specified at all, use the default classpath.
        classpath = _classpath.getClassPath()

# Code for 1.6 release when we add module support
#    # Modulepath handling
#    old_modulepath = _getOption(jvm_args, "--module-path", _classpath._SEP)
#    if old_modulepath:
#        # Old style, specified in the arguments
#        if modulepath is not None:
#            # Cannot apply both styles, conflict
#            raise TypeError('modulepath specified twice')
#        modulepath = old_modulepath
#    if modulepath is not None:
#        mp = _classpath._SEP.join(_expandClassPath(modulepath))
#        extra_jvm_args += ['--module-path=%s'%mp ]

    # Get the support library
    support_lib = os.path.join(os.path.dirname(
        os.path.dirname(__file__)), "org.jpype.jar")
    if not os.path.exists(support_lib):
        raise RuntimeError(
            "Unable to find org.jpype.jar support library at " + support_lib)

    system_class_loader = _getOption(
        jvm_args, "-Djava.system.class.loader", keep=True)

    java_class_path = _expandClassPath(classpath)
    java_class_path.append(support_lib)
    java_class_path = list(filter(len, java_class_path))
    classpath = _classpath._SEP.join(java_class_path)
    tmp = None

    # Make sure our module is always on the classpath
    if not classpath.isascii():
        if system_class_loader:
            # https://bugs.openjdk.org/browse/JDK-8079633?jql=text%20~%20%22ParseUtil%22
            raise ValueError(
                "system classloader cannot be specified with non ascii characters in the classpath")

        # If we are not installed on an ascii path then we will need to copy the jar to a new location
        if not support_lib.isascii():
            import tempfile
            import shutil
            fd, path = tempfile.mkstemp(dir=_findTemp())
            if not path.isascii():
                raise ValueError("Unable to find ascii temp directory.")
            shutil.copyfile(support_lib, path)
            support_lib = path
            tmp = path
            os.close(fd)
            # Don't remove

        # ok, setup the jpype system classloader and add to the path after startup
        # this guarentees all classes have the same permissions as they did in the past
        from urllib.parse import quote
        extra_jvm_args += [
            '-Djava.system.class.loader=org.jpype.JPypeClassLoader',
            '-Djava.class.path=%s' % support_lib,
            '-Djpype.class.path=%s' % quote(classpath),
            '-Xshare:off'
        ]
    else:
        # no problems
        extra_jvm_args += ['-Djava.class.path=%s' % classpath]

    try:
        import locale
        # Gather a list of locale settings that Java may override (excluding LC_ALL)
        categories = [getattr(locale, i) for i in dir(
            locale) if i.startswith('LC_') and i != 'LC_ALL']
        # Keep the current locale settings, else Java will replace them.
        prior = [locale.getlocale(i) for i in categories]
        # Start the JVM
        _jpype.startup(jvmpath, tuple(jvm_args + extra_jvm_args),
                       ignoreUnrecognized, convertStrings, interrupt, tmp)
        # Collect required resources for operation
        initializeResources()
        # Restore locale
        for i, j in zip(categories, prior):
            try:
                locale.setlocale(i, j)
            except locale.Error:
                pass
    except RuntimeError as ex:
        source = str(ex)
        if "UnsupportedClassVersion" in source:
            import re
            match = re.search(r"([0-9]+)\.[0-9]+", source)
            if match:
                version = int(match.group(1)) - 44
                raise RuntimeError(
                    f"{jvmpath} is older than required Java version{version}") from ex
        raise


def initializeResources():
    global _JVM_started
    _jpype._java_lang_Class = None
    _jpype._java_lang_Object = _jpype.JClass("java.lang.Object")
    _jpype._java_lang_Throwable = _jpype.JClass("java.lang.Throwable")
    _jpype._java_lang_Exception = _jpype.JClass("java.lang.Exception")
    _jpype._java_lang_Class = _jpype.JClass("java.lang.Class")
    _jpype._java_lang_String = _jpype.JClass("java.lang.String")

    _jpype._java_lang_RuntimeException = _jpype.JClass(
        "java.lang.RuntimeException")

    # Preload needed classes
    _jpype._java_lang_Boolean = _jpype.JClass("java.lang.Boolean")
    _jpype._java_lang_Byte = _jpype.JClass("java.lang.Byte")
    _jpype._java_lang_Character = _jpype.JClass("java.lang.Character")
    _jpype._java_lang_Short = _jpype.JClass("java.lang.Short")
    _jpype._java_lang_Integer = _jpype.JClass("java.lang.Integer")
    _jpype._java_lang_Long = _jpype.JClass("java.lang.Long")
    _jpype._java_lang_Float = _jpype.JClass("java.lang.Float")
    _jpype._java_lang_Double = _jpype.JClass("java.lang.Double")

    # Bind types
    _jpype.JString.class_ = _jpype._java_lang_String
    _jpype.JObject.class_ = _jpype._java_lang_Object
    _jtypes.JBoolean.class_ = _jpype._java_lang_Boolean.TYPE
    _jtypes.JByte.class_ = _jpype._java_lang_Byte.TYPE
    _jtypes.JChar.class_ = _jpype._java_lang_Character.TYPE
    _jtypes.JShort.class_ = _jpype._java_lang_Short.TYPE
    _jtypes.JInt.class_ = _jpype._java_lang_Integer.TYPE
    _jtypes.JLong.class_ = _jpype._java_lang_Long.TYPE
    _jtypes.JFloat.class_ = _jpype._java_lang_Float.TYPE
    _jtypes.JDouble.class_ = _jpype._java_lang_Double.TYPE
    _jtypes.JBoolean._hints = _jcustomizer.getClassHints("boolean")
    _jtypes.JByte._hints = _jcustomizer.getClassHints("byte")
    _jtypes.JChar._hints = _jcustomizer.getClassHints("char")
    _jtypes.JShort._hints = _jcustomizer.getClassHints("short")
    _jtypes.JInt._hints = _jcustomizer.getClassHints("int")
    _jtypes.JLong._hints = _jcustomizer.getClassHints("long")
    _jtypes.JFloat._hints = _jcustomizer.getClassHints("float")
    _jtypes.JDouble._hints = _jcustomizer.getClassHints("double")

    # Table for automatic conversion to objects "JObject(value, type)"
    _jpype._object_classes = {}
    # These need to be deprecated so that we can support casting Python objects
    _jpype._object_classes[bool] = _jpype._java_lang_Boolean
    _jpype._object_classes[int] = _jpype._java_lang_Long
    _jpype._object_classes[float] = _jpype._java_lang_Double
    _jpype._object_classes[str] = _jpype._java_lang_String
    _jpype._object_classes[type] = _jpype._java_lang_Class
    _jpype._object_classes[object] = _jpype._java_lang_Object
    _jpype._object_classes[_jpype._JClass] = _jpype._java_lang_Class
    _jpype._object_classes[_jtypes.JBoolean] = _jpype._java_lang_Boolean
    _jpype._object_classes[_jtypes.JByte] = _jpype._java_lang_Byte
    _jpype._object_classes[_jtypes.JChar] = _jpype._java_lang_Character
    _jpype._object_classes[_jtypes.JShort] = _jpype._java_lang_Short
    _jpype._object_classes[_jtypes.JInt] = _jpype._java_lang_Integer
    _jpype._object_classes[_jtypes.JLong] = _jpype._java_lang_Long
    _jpype._object_classes[_jtypes.JFloat] = _jpype._java_lang_Float
    _jpype._object_classes[_jtypes.JDouble] = _jpype._java_lang_Double
    _jpype._object_classes[type(None)] = _jpype._java_lang_Object
    _jpype._object_classes[_jpype.JString] = _jpype._java_lang_String

    # Set up table of automatic conversions of Python primitives
    # this table supports "JArray(type)"
    _jpype._type_classes = {}
    _jpype._type_classes[bool] = _jtypes.JBoolean
    _jpype._type_classes[int] = _jtypes.JLong
    _jpype._type_classes[float] = _jtypes.JDouble
    _jpype._type_classes[str] = _jpype._java_lang_String
    _jpype._type_classes[type] = _jpype._java_lang_Class
    _jpype._type_classes[object] = _jpype._java_lang_Object
    _jpype._type_classes[_jpype.JString] = _jpype._java_lang_String
    _jpype._type_classes[_jpype.JObject] = _jpype._java_lang_Object
    _jpype._type_classes[_jpype.JClass] = _jpype._java_lang_Class
    _jinit.runJVMInitializers()

    _jpype.JClass('org.jpype.JPypeKeywords').setKeywords(
        list(_pykeywords._KEYWORDS))

    _jpype.JPypeContext = _jpype.JClass('org.jpype.JPypeContext').getInstance()
    _jpype.JPypeClassLoader = _jpype.JPypeContext.getClassLoader()

    # Everything succeeded so started is now true.
    _JVM_started = True


def shutdownJVM():
    """ Shuts down the JVM.

    This method shuts down the JVM and disables access to existing
    Java objects. Due to limitations in the JPype, it is not possible to
    restart the JVM after being terminated.
    """
    import threading
    import jpype.config
    if threading.current_thread() is not threading.main_thread():
        raise RuntimeError("Shutdown must be called from main thread")
    if _jpype.isStarted():
        _jpype.JPypeContext.freeResources = jpype.config.free_resources
    _jpype.shutdown(jpype.config.destroy_jvm, False)


# In order to shutdown cleanly we need the reference queue stopped
# otherwise, we can experience a crash if a Java thread is waiting
# for the GIL.
def _JTerminate():
    try:
        import jpype.config
        # We are exiting anyway so no need to free resources
        if jpype.config.onexit:
            _jpype.shutdown(jpype.config.destroy_jvm, False)
    except RuntimeError:
        pass


atexit.register(_JTerminate)


@deprecated("java.lang.Thread.isAttached")
def isThreadAttachedToJVM():
    """ Checks if a thread is attached to the JVM.

    Python automatically attaches threads when a Java method is called.
    This creates a resource in Java for the Python thread. This method
    can be used to check if a Python thread is currently attached so that
    it can be disconnected prior to thread termination to prevent leaks.

    Returns:
      True if the thread is attached to the JVM, False if the thread is
      not attached or the JVM is not running.
    """
    return _jpype.isThreadAttachedToJVM()


@deprecated("java.lang.Thread.attach")
def attachThreadToJVM():
    """ Attaches a thread to the JVM.

    The function manually connects a thread to the JVM to allow access to
    Java objects and methods. JPype automatically attaches when a Java
    resource is used, so a call to this is usually not needed.

    Raises:
      RuntimeError: If the JVM is not running.
    """
    _jpype.attachThreadToJVM()


@deprecated("java.lang.Thread.detach")
def detachThreadFromJVM():
    """ Detaches a thread from the JVM.

    This function detaches the thread and frees the associated resource in
    the JVM. For codes making heavy use of threading this should be used
    to prevent resource leaks. The thread can be reattached, so there
    is no harm in detaching early or more than once. This method cannot fail
    and there is no harm in calling it when the JVM is not running.
    """
    _jpype.detachThreadFromJVM()


def synchronized(obj):
    """ Creates a resource lock for a Java object.

    Produces a monitor object. During the lifespan of the monitor Java
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
    return _jpype._JMonitor(obj)


def getJVMVersion():
    """ Get the JVM version if the JVM is started.

    This function can be used to determine the version of the JVM. It is
    useful to help determine why a Jar has failed to load.

    Returns:
      A typle with the (major, minor, revison) of the JVM if running.
    """
    if not _jpype.isStarted():
        return (0, 0, 0)

    import re
    runtime = _jpype.JClass('java.lang.Runtime')
    version = runtime.class_.getPackage().getImplementationVersion()

    # Java 9+ has a version method
    if not version:
        version = runtime.version()
    version = (re.match("([0-9.]+)", str(version)).group(1))
    return tuple([int(i) for i in version.split('.')])


@_jcustomizer.JImplementationFor("java.lang.Runtime")
class _JRuntime(object):
    # We need to redirect hooks so that we control the order
    def addShutdownHook(self, thread):
        return _jpype.JClass("org.jpype.JPypeContext").getInstance().addShutdownHook(thread)

    def removeShutdownHook(self, thread):
        return _jpype.JClass("org.jpype.JPypeContext").getInstance().removeShutdownHook(thread)


_jpype.JVMNotRunning = JVMNotRunning
