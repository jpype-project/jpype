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

# This is a superset of the keywords in Python.
# We use this so that jpype is a bit more version independent.
# Removing keywords from this list impacts the exposed interfaces, and therefore is a breaking change. 
_KEYWORDS = set((
    'False', 'None', 'True', 'and', 'as', 'assert', 'async',
    'await', 'break', 'class', 'continue', 'def', 'del', 'elif', 'else',
    'except', 'exec', 'finally', 'for', 'from', 'global', 'if', 'import',
    'in', 'is', 'lambda', 'nonlocal', 'not', 'or', 'pass', 'print',
    'raise', 'return', 'try', 'while', 'with', 'yield'
))


def pysafe(s):
    if s.startswith("__"):
        return None
    if s in _KEYWORDS:
        return s + "_"
    return s
