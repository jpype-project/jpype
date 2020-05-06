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
