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
from . import _jinit
from ._jvmfinder import JVMNotFoundException, JVMNotSupportedException

__all__ = [
    'isJVMStarted', 'startJVM', 'attachToJVM', 'shutdownJVM',
    'getDefaultJVMPath', 'getJVMVersion', 'isThreadAttachedToJVM', 'attachThreadToJVM',
    'detachThreadFromJVM', 'synchronized', 'get_default_jvm_path',
    'JVMNotFoundException', 'JVMNotSupportedException'
]


# Activate jedi tab completion
try:
    import jedi as _jedi
    _jedi.evaluate.compiled.access.ALLOWED_DESCRIPTOR_ACCESS += \
       ( _jpype.PyJPMethod, _jpype.PyJPField)
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


def _initialize():
    _jclass._initialize()
    _jobject._initialize()
    _jtypes._initialize()
    _jinit.runJVMInitializers()


def isJVMStarted():
    return _jpype.isStarted()


def _hasClassPath(args):
    for i in args:
        if i.startswith('-Djava.class.path'):
            return True
    return False

def _handleClassPath(clsList):
    out = []
    for s in clsList:
        if not isinstance(s, (str, _jtypes._unicode)):
            raise TypeError("Classpath elements must be strings")
        if s.endswith('*'):
            import glob
            out.extend(glob.glob(s+'.jar'))
        else:
            out.append(s)
    return _classpath._SEP.join(out)


_JVM_started = False

def startJVM(*args, **kwargs):
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
    if _jpype.isStarted():
        raise OSError('JVM is already started')
    global _JVM_started
    if _JVM_started:
        raise OSError('JVM cannot be restarted')
    _JVM_started = True

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
        if isinstance(classpath, (str, _jtypes._unicode)):
            args.append('-Djava.class.path=%s' % _handleClassPath([classpath]))
        elif hasattr(classpath, '__iter__'):
            args.append('-Djava.class.path=%s' % _handleClassPath(classpath))
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

    _jpype.startup(jvmpath, tuple(args), ignoreUnrecognized, convertStrings)
    _initialize()


def attachToJVM(jvm):
    _jpype.attach(jvm)
    _initialize()


def shutdownJVM():
    """ Shuts down the JVM.

    This method shuts down the JVM and thus disables access to existing
    Java objects. Due to limitations in the JPype, it is not possible to
    restart the JVM after being terminated.
    """
    _jpype.shutdown()


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


def attachThreadToJVM():
    """ Attaches a thread to the JVM.

    The function manually connects a thread to the JVM to allow access to
    Java objects and methods. JPype automaticatlly attaches when a Java
    resource is used, so a call to this is usually not needed.

    Raises:
      RuntimeError: If the JVM is not running.
    """
    _jpype.attachThreadToJVM()


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
    runtime = _jclass.JClass('java.lang.Runtime')
    version = runtime.class_.getPackage().getImplementationVersion()

    # Java 9+ has a version method 
    if not version:
        version = runtime.version()
    version = (re.match("([0-9.]+)", str(version)).group(1))
    return tuple([int(i) for i in version.split('.')])

