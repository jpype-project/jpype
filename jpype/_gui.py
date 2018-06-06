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

from . import _jproxy
from . import _jclass

def setupGuiEnvironment(cb):
    if sys.platform == 'darwin':
        from PyObjCTools import AppHelper
        m = {'run': cb}
        proxy = _jproxy.JProxy('java.lang.Runnable', m)
        cbthread = _jclass.JClass("java.lang.Thread")(proxy)
        cbthread.start()
        AppHelper.runConsoleEventLoop()
    else:
        cb()

def shutdownGuiEnvironment():
    if sys.platform == 'darwin':
        from PyObjCTools import AppHelper
        AppHelper.stopEventLoop()
