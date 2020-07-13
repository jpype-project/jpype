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

__all__ = ['convertToDirectBuffer']


def convertToDirectBuffer(obj):
    __doc__ = '''Efficiently convert all array.array and numpy ndarray types, string and unicode to java.nio.Buffer objects.'''

    memoryview_of_obj = memoryview(obj)

    if memoryview_of_obj.readonly:
        raise ValueError(
            "Memoryview must be writable for wrapping in a byte buffer")

    return _jpype.convertToDirectBuffer(memoryview_of_obj)
