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
import _jpype

__all__ = ['onJVMStart']

JInitializers = []


def onJVMStart(func):
    """Decorator to register a function to be called after JVM is started.

    This can be used to load module resources that depend on the JVM such as loading
    classes.  If the JVM is not started, the user supplied function is held in a list until
    the JVM starts.  When startJVM is called, all functions on the deferred list are called
    and the list is cleared.  If the JVM is already started, then the function is called
    immediately.

    Errors from the function will either be raised immediately if the JVM is started, or 
    from startJVM if the JVM is not yet started.

    Args:
       func (callable): a function to call when the JVM is started.
    """
    registerJVMInitializer(func)
    return func


def registerJVMInitializer(func):
    if not _jpype.isStarted():
        JInitializers.append(func)
    else:
        # JVM is already started so we are safe to execute immediately.
        func()


def runJVMInitializers():
    for func in JInitializers:
        func()
    JInitializers.clear()
