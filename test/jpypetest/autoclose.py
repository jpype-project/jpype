#*****************************************************************************
#   Copyright 2017 Karl Einar Nelson
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
#*****************************************************************************
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import jpype
from . import common

class AutoCloseCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testCloseable(self):
        AutoClose = jpype.JClass("jpype.closeable.AutoClose")
        self.assertFalse(AutoClose.closed)
        with AutoClose() as myFile:
            myFile.print_("hello")
        self.assertEqual(AutoClose.printed, "hello")
        self.assertTrue(AutoClose.closed)
