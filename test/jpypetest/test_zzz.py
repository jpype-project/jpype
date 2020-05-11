import sys
import _jpype
import jpype
from jpype.types import *
from jpype import JPackage, java
import common
import pytest
try:
    import numpy as np
except ImportError:
    pass


class ZZZTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testShutdown(self):
        # Install a coverage hook
        instance = JClass("org.jpype.JPypeContext").getInstance()
        JClass("jpype.common.OnShutdown").addCoverageHook(instance)

        # Shutdown
        jpype.shutdownJVM()

        # Check that shutdown does not raise
        jpype._core._JTerminate()
