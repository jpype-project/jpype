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
from __future__ import annotations

import typing

# This is a superset of the keywords in Python.
# We use this so that jpype is a bit more version independent.
# Adding and removing keywords from this list impacts the exposed interfaces,
# and therefore is technically a breaking change.
_KEYWORDS = {
    'False',
    'None',
    'True',
    'and',
    'as',
    'assert',
    'async',
    'await',
    'break',
    'class',
    'continue',
    'def',
    'del',
    'elif',
    'else',
    'except',
    'exec',
    'finally',
    'for',
    'from',
    'global',
    'if',
    'import',
    'in',
    'is',
    'lambda',
    'nonlocal',
    'not',
    'or',
    'pass',
    'print',  # No longer a Python 3 keyword. Kept for backwards compatibility.
    'raise',
    'return',
    'try',
    'while',
    'with',
    'yield',
}


def pysafe(s: str) -> typing.Optional[str]:
    """
    Given an identifier name in Java, return an equivalent identifier name in
    Python that is guaranteed to not collide with the Python grammar.

    """
    if s.startswith("__") and s.endswith("__") and len(s) >= 4:
        # Dunder methods should not be considered safe.
        # (see system defined names in the Python documentation
        # https://docs.python.org/3/reference/lexical_analysis.html#reserved-classes-of-identifiers
        # )
        return None
    if s in _KEYWORDS:
        return s + "_"
    return s
