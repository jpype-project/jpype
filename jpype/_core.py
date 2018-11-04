#*****************************************************************************
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
#*****************************************************************************
import sys

import _jpype

from . import _jclass
from . import _jarray
from . import _jwrapper
from . import _jproxy
from . import _jexception
from . import _jcollection
from . import _jobject
from . import _jio
from . import _properties
from . import nio
from . import reflect
from . import _refdaemon
from . import _classpath

_usePythonThreadForDaemon = False

def setUsePythonThreadForDeamon(v):
    global _usePythonThreadForDaemon
    _usePythonThreadForDaemon = v

_initializers=[]

def registerJVMInitializer(func):
    """Register a function to be called after jvm is started"""
    _initializers.append(func)

def _initialize():
    _properties._initialize()
    _jclass._initialize()
    _jarray._initialize()
    _jwrapper._initialize()
    _jproxy._initialize()
    _jexception._initialize()
    _jcollection._initialize()
    _jobject._initialize()
    nio._initialize()
    reflect._initialize()
    for func in _initializers:
        func()

def isJVMStarted() :
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
        jvm = get_default_jvm_path()

        # Check to see that the user has not set the classpath
        # Otherwise use the default if not specified
        if not _hasClassPath(args) and 'classpath' not in kwargs:
           kwargs['classpath']=_classpath.getClassPath()
           print("Use default classpath")

    if 'ignoreUnrecognized' not in kwargs:
        kwargs['ignoreUnrecognized']=False

    # Classpath handling
    args = list(args)
    if 'classpath' in kwargs and kwargs['classpath']!=None:
        args.append('-Djava.class.path=%s'%(kwargs['classpath']))
        print("Set classpath")

    _jpype.startup(jvm, tuple(args), kwargs['ignoreUnrecognized'])
    _initialize()

    # start the reference daemon thread
    if _usePythonThreadForDaemon:
        _refdaemon.startPython()
    else:
        _refdaemon.startJava()

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


def get_default_jvm_path():
    """
    Retrieves the path to the default or first found JVM library

    :return: The path to the JVM shared library file
    :raise ValueError: No JVM library found
    """
    if sys.platform == "cygwin":
        # Cygwin
        from ._cygwin import WindowsJVMFinder
        finder = WindowsJVMFinder()
    elif sys.platform == "win32":
        # Windows
        from ._windows import WindowsJVMFinder
        finder = WindowsJVMFinder()
    elif sys.platform == "darwin":
        # Mac OS X
        from ._darwin import DarwinJVMFinder
        finder = DarwinJVMFinder()
    else:
        # Use the Linux way for other systems
        from ._linux import LinuxJVMFinder
        finder = LinuxJVMFinder()

    return finder.get_jvm_path()

# Naming compatibility
getDefaultJVMPath = get_default_jvm_path


class ConversionConfigClass(object):
    def __init__(self):
        self._convertString = 1

    def _getConvertString(self):
        return self._convertString

    def _setConvertString(self, value):
        if value:
            self._convertString = 1
        else:
            self._convertString = 0

        _jpype.setConvertStringObjects(self._convertString)

    string = property(_getConvertString, _setConvertString, None)

ConversionConfig = ConversionConfigClass()
