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

def startJVM(jvm=None, *args, **kwargs):
    """
    Starts a Java Virtual Machine.  Without options it will start
    the JVM with the default classpath and jvm.  The default classpath
    will be determined by jpype.getClassPath().  The default JVM is 
    determined by jpype.getDefaultJVMPath().

    Args:
      jvm (str):  Path to the jvm library file (libjvm.so, jvm.dll, ...)
        default=None will use jpype.getDefaultJVMPath()
      *args (str[]): Arguments to give to the JVM
      classpath (Optional[string]): set the classpath for the jvm.
        This will override any classpath supplied in the arguments
        list.
      ignoreUnrecognized (Optional[bool]): option to jvm to ignore
        invalid jvm arguments.  (Default False)
    """
    if jvm is None:
        jvm = getDefaultJVMPath()

        # Check to see that the user has not set the classpath
        # Otherwise use the default if not specified
        if not _hasClassPath(args) and 'classpath' not in kwargs:
           kwargs['classpath']=_classpath.getClassPath()

    if 'ignoreUnrecognized' not in kwargs:
        kwargs['ignoreUnrecognized']=False

    # Classpath handling
    args = list(args)
    if 'classpath' in kwargs and kwargs['classpath']!=None:
        args.append('-Djava.class.path=%s'%(kwargs['classpath']))

    _jpype.startup(jvm, tuple(args), kwargs['ignoreUnrecognized'])
    _initialize()


def attachToJVM(jvm):
    _jpype.attach(jvm)
    _initialize()


def shutdownJVM():
    _jpype.shutdown()


def isThreadAttachedToJVM():
    return _jpype.isThreadAttachedToJVM()


def attachThreadToJVM():
    _jpype.attachThreadToJVM()


def detachThreadFromJVM():
    _jpype.detachThreadFromJVM()


def synchronized(obj):
    return _jpype.PyJPMonitor(obj.__javavalue__)


def getDefaultJVMPath():
    """
    Retrieves the path to the default or first found JVM library

    :return: The path to the JVM shared library file
    :raise ValueError: No JVM library found
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
    """ Get the jvm version if the jvm is started.
    """
    if not _jpype.isStarted():
        return (0, 0, 0)
    version = str(_jclass.JClass(
        'java.lang.Runtime').class_.getPackage().getImplementationVersion())
    if version.find('_') != -1:
        parts = version.split('_')
        version = parts[0]
    return tuple([int(i) for i in version.split('.')])
