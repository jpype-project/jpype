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
from . import common

from jpype.compiler import JCompiler, JSyntaxError

try:
    import unittest2 as unittest
except ImportError:
    import unittest

class CompilerTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testCompileSuccess(self):
        compiler = JCompiler()
        if not compiler.available():
            self.skipTest("compiler is not available")

        Test = compiler.compile('my.Test',r'''
            package my;
            public class Test
            {
                public String get()
                {
                    return "word";
                }
            }
        ''')
        test = Test()
        self.assertEquals(test.get(), "word")

    def testCompileFail(self):
        compiler = JCompiler()
        if not compiler.available():
            self.skipTest("compiler is not available")
        with self.assertRaises(JSyntaxError):
            Test = compiler.compile('my.Test',r'''
                package my;
                public class Test
                {
                    public Sting get()
                    {
                        return "word";
                    }
                }
            ''')

