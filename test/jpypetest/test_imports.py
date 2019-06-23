# *****************************************************************************
#   Copyright 2017 Karl Einar Nelson
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# *****************************************************************************
import jpype
import sys
import logging
import time
import common

def haveJImports():
    try:
        import jpype.imports
        return True
    except ImportError:
        return False


def isJavaClass(tp):
    return isinstance(tp, jpype.JClass)


def isJavaEnum(tp):
    return issubclass(tp, jpype.JClass('java.lang.Enum'))


class ImportsTestCase(common.JPypeTestCase):
    def setUp(self):
        #        logger = logging.getLogger(__name__)
        #        logger.info("TEST:JImports")
        common.JPypeTestCase.setUp(self)
        self.__jp = self.jpype.attr

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportPackage(self):
        import java.lang
        self.assertTrue(isJavaClass(java.lang.String))

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportClass(self):
        from java.lang import String
        self.assertTrue(isJavaClass(String))

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportClassAs(self):
        from java.lang import String as Str
        self.assertTrue(isJavaClass(Str))

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportClassMultiple(self):
        from java.lang import Number, Integer, Double
        self.assertTrue(isJavaClass(Number))
        self.assertTrue(isJavaClass(Integer))
        self.assertTrue(isJavaClass(Double))

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportStatic(self):
        from java.lang.ProcessBuilder import Redirect
        self.assertTrue(isJavaClass(Redirect))

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportInner(self):
        from java.lang import Character
        self.assertTrue(isJavaClass(Character.UnicodeScript))

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportInnerEnum(self):
        from java.lang import Character
        self.assertTrue(isJavaEnum(Character.UnicodeScript))

# FIXME add test for static member variable imports and other edge cases.
