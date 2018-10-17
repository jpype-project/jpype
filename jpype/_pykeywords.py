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

# This is a super set of the keywords in Python2 and Python3.
# We use this so that jpype is a bit more version independent.
_KEYWORDS = set((
    "del", "for", "is", "raise",
    "assert", "elif", "from", "lambda", "return",
    "break", "else", "global", "not", "try",
    "class", "except", "if", "or", "while",
    "continue", "exec", "import", "pass", "yield",
    "def", "finally", "in", "print", "as", "None"
))


def pysafe(s):
    if s in _KEYWORDS:
        return s+"_"
    return s
