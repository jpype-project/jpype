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
from . import _jcustomizer

# This contains a customizer for java.lang.reflect.Method so that a specific,
# already-resolved overload (e.g. obtained via Class.getDeclaredMethod) can be
# turned into a plain Python callable bound to just that one signature,
# skipping jpype's normal per-call overload search. Instance methods take the
# instance as their first argument (there is no attribute to bind self to),
# e.g. ``m.toPython()(instance, *args)``; static methods are called directly
# as ``m.toPython()(*args)``.


@_jcustomizer.JImplementationFor("java.lang.reflect.Method")
class _JReflectMethod(object):
    def toPython(self):
        return _jpype._reflectMethod(self)
