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
try:
    import unittest2 as unittest
except ImportError:
    import unittest


class LambdasTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        try:
            self.lambdas = jpype.JClass("jpype.lambda.Test1")()
            return
        except:
            pass
        raise unittest.SkipTest

    def testLambdasFunction(self):
        self.assertEqual(self.lambdas.getFunction().apply(1.0), 2.0)

    def testLambdasLambda(self):
        self.assertEqual(self.lambdas.getLambda().apply(1.0), 2.0)
