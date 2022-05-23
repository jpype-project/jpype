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
import keyword  # From the standard library.

import pytest

import jpype
import common


@pytest.mark.parametrize(
    "identifier",
    list(keyword.kwlist) + [
        '__del__',
        # Print is no longer a keyword in Python 3, but still protected to
        # avoid API breaking in JPype v1.
        'print',
        '____',  # Not allowed.
    ]
)
def testPySafe__Keywords(identifier):
    safe = jpype._pykeywords.pysafe(identifier)
    if identifier.startswith("__"):
        assert safe is None
    else:
        assert isinstance(safe, str), f"Fail on keyword {identifier}"
        assert safe.endswith("_")


@pytest.mark.parametrize(
    "identifier",
    [
        'notSpecial',
        '__notSpecial',
        'notSpecial__',
        '_notSpecial_',
        '_not__special_',
        '__', '___',  # Technically these are fine.
    ])
def testPySafe__NotKeywords(identifier):
    safe = jpype._pykeywords.pysafe(identifier)
    assert safe == identifier


class AttributeTestCase(common.JPypeTestCase):
    def testPySafe(self):
        cls = jpype.JPackage("jpype").attr.TestKeywords
        self.assertTrue(hasattr(cls, "__leading_double_underscore"))
        self.assertFalse(hasattr(cls, "__dunder_name__"))
