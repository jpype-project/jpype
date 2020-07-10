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
import jpype
from jpype.types import *
from jpype import java
import common
try:
    import numpy as np
except ImportError:
    pass


class CustomizerTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = JClass('jpype.common.Fixture')()

    def testSticky(self):
        @jpype.JImplementationFor("jpype.override.A")
        class _A:
            @jpype.JOverride(sticky=True, rename="remove_")
            def remove(self, obj):
                pass

        A = jpype.JClass("jpype.override.A")
        B = jpype.JClass("jpype.override.B")
        self.assertEqual(A.remove, _A.remove)
        self.assertEqual(B.remove, _A.remove)
        self.assertEqual(str(A.remove_), "jpype.override.A.remove")
        self.assertEqual(str(B.remove_), "jpype.override.B.remove")
