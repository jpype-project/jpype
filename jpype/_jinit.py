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
import inspect

import _jpype

__all__ = ['registerJVMInitializer']

JInitializers = []


def registerJVMInitializer(func):
    """Register a function to be called after jvm is started"""
    if not _jpype.isStarted():
        JInitializers.append(func)
    else:
        # JVM is already started so we are safe to execute immediately.
        func()


def runJVMInitializers():
    for func in JInitializers:
        func()


def registerJVMInitializer(func):
    """Register a function to be called after jvm is started"""
    if not _jpype.isStarted():
        JInitializers.append(func)
    else:
        # JVM is already started so we are safe to execute immediately.
        func()


def runJVMInitializers():
    for func in JInitializers:
        func()
