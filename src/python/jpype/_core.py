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

import _jclass
import _jarray
import _jwrapper
import _jproxy
import _jexception
import _jcollection
import _jobject
import _properties
import nio
import reflect
import _refdaemon

_usePythonThreadForDaemon = False

def setUsePythonThreadForDeamon(v):
	global _usePythonThreadForDaemon
	_usePythonThreadForDaemon = v


def isJVMStarted() :
    return _jpype.isStarted()

def startJVM(jvm, *args) :
    _jpype.startup(jvm, tuple(args), True)
    _jclass._initialize()
    _jarray._initialize()
    _jwrapper._initialize()
    _jproxy._initialize()
    _jexception._initialize()
    _jcollection._initialize()
    _jobject._initialize()
    _properties._initialize()
    nio._initialize()
    reflect._initialize()
    
    # start the reference deamon thread 
    if _usePythonThreadForDaemon :
    	_refdaemon.startPython()
    else:
    	_refdaemon.startJava()
    
def attachToJVM(jvm) :
    _jpype.attach(jvm)
    
    _jclass._initialize()
    _jarray._initialize()
    _jwrapper._initialize()
    _jproxy._initialize()
    _jexception._initialize()
    _jcollection._initialize()
    _jobject._initialize()
    _properties._initialize()
        
def shutdownJVM() :
    _refdaemon.stop()
    _jpype.shutdown()
    
def isThreadAttachedToJVM() :
    return _jpype.isThreadAttachedToJVM()
    
def attachThreadToJVM() :
    _jpype.attachThreadToJVM()
    
def detachThreadFromJVM() :    
    _jpype.detachThreadFromJVM()
    
def getDefaultJVMPath() :
    if sys.platform == "win32" :
        import _windows
        return _windows.getDefaultJVMPath()
    elif sys.platform == "darwin" :
        import _darwin
        return _darwin.getDefaultJVMPath()
    else:
        import _linux
        return _linux.getDefaultJVMPath()
    
class ConversionConfigClass(object):
    def __init__(self):
        self._convertString = 1
        
    def _getConvertString(self):
        return self._convertString
    
    def _setConvertString(self, value):
        if value :
            self._convertString = 1
        else:
            self._convertString = 0
            
        _jpype.setConvertStringObjects(self._convertString)
        
    string = property(_getConvertString, _setConvertString, None )
    
ConversionConfig = ConversionConfigClass()
        