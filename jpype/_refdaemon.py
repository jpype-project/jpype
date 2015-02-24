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
import threading

import _jpype

def startJava():
    _jpype.startReferenceQueue(1)

def startPython():
    def _run():
        _jpype.attachThreadToJVM()
        _jpype.startReferenceQueue(0)

        threading.Thread(target=_run).start()

def stop():
    _jpype.stopReferenceQueue()
