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
        jpype.shutdownJVM()
