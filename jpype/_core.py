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

__all__ = [
    'isJVMStarted', 'startJVM', 'attachToJVM', 'shutdownJVM',
    'getDefaultJVMPath', 'getJVMVersion', 'isThreadAttachedToJVM', 'attachThreadToJVM',
    'detachThreadFromJVM', 'synchronized'
]

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


def startJVM(*args, **kwargs):
    """
    Starts a Java Virtual Machine.  Without options it will start
    the JVM with the default classpath and jvm.  The default classpath
    will be determined by ``jpype.getClassPath()``.  The default JVM is 
    determined by ``jpype.getDefaultJVMPath()``.

    Parameters:
     *args (Optional, str[]): Arguments to give to the JVM.
        The first argument may be the path the JVM.

    Keyword Arguments:
      jvm (str):  Path to the jvm library file,
        Typically one of (``libjvm.so``, ``jvm.dll``, ...) 
        Default of None will use ``jpype.getDefaultJVMPath()``
      classpath ([string]): Set the classpath for the jvm.
        This will override any classpath supplied in the arguments
        list. Default will use ``jpype.getClassPath``
      ignoreUnrecognized (bool): Option to JVM to ignore
        invalid JVM arguments. Default is False.

    Raises:
      OSError: if the JVM cannot be started or is already running.
      TypeError: if an invalid keyword argument is supplied 
        or a keyword argument conflicts with the arguments.

     """
    args = list(args)

    # JVM path
    jvm = None
    if args and not args[0].startswith('-'):
        # jvm is the first argument the first argument is a path
        jvm = args.pop(0)
    if 'jvm' in kwargs:
        if jvm:
            raise TypeError('jvm path specified twice')
        jvm = kwargs['jvm']
        del kwargs['jvm']
    if not jvm:
        jvm = getDefaultJVMPath()

    # Classpath handling
    classpath = None
    if 'classpath' in kwargs:
        if _hasClassPath(args):
            raise TypeError('classpath specified twice')
        classpath = kwargs['classpath']
        del kwargs['classpath']
    elif not _hasClassPath(args):
        classpath = _classpath.getClassPath()

    if classpath:
        if isinstance(classpath, (str, _jtypes._unicode)):
            args.append('-Djava.class.path=%s' % classpath)
        else:
            args.append('-Djava.class.path=%s' %
                        (_classpath._SEP.join(classpath)))

    # Handle ignoreUnrecognized
    ignoreUnrecognized = False
    if 'ignoreUnrecognized' in kwargs:
        ignoreUnrecognized = kwargs['ignoreUnrecognized']
        del kwargs['ignoreUnrecognized']

    if kwargs:
        raise TypeError("startJVM() got an unexpected keyword argument '%s'"
                        % (','.join([str(i) for i in kwargs])))

    _jpype.startup(jvm, tuple(args), ignoreUnrecognized)
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
get_default_jvm_path = getDefaultJVMPath


def getJVMVersion():
    """ Get the JVM version if the JVM is started.

    This function can be used to determine the version of the JVM. It is 
    useful to help determine why a Jar has failed to load.  

    Returns:
      A typle with the (major, minor, revison) of the JVM if running.
    """
    if not _jpype.isStarted():
        return (0, 0, 0)
    version = str(_jclass.JClass(
        'java.lang.Runtime').class_.getPackage().getImplementationVersion())
    if version.find('_') != -1:
        parts = version.split('_')
        version = parts[0]
    return tuple([int(i) for i in version.split('.')])
