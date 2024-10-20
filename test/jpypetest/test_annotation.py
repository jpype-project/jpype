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
import common
from jpype import JParameterAnnotation
from jpype._jclass import *
from jpype.types import *
from jpype.imports import *

# NOTE: Testing whether or not they are actually applied will be done in the extension tests

class JAnnotationTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testMissingElement(self):
        TestAnnotation = JClass("jpype.annotation.TestAnnotation")
        with self.assertRaises(KeyError):
            TestAnnotation()

    def testInvalidElement(self):
        TestAnnotation = JClass("jpype.annotation.TestAnnotation")
        with self.assertRaises(KeyError):
            TestAnnotation(test="test")

    def testConstructAnnotation(self):
        TestAnnotation = JClass("jpype.annotation.TestAnnotation")
        TestAnnotation(name="test")

    def testConstructSimpleAnnotation(self):
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")
        # chosen by fair dice roll.
        # guaranteed to be random.
        TestSimpleAnnotation(4)

    def testConstructSimpleAnnotationExplicit(self):
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")
        TestSimpleAnnotation(value=4)

    def testSimpleAnnotationInvalidElement(self):
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")
        with self.assertRaises(KeyError):
            TestSimpleAnnotation(test=4)

    def testParameterAnnotation(self):
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")

        @JParameterAnnotation("arg1", TestSimpleAnnotation(4))
        def fun(arg1):
            ...

    def testChainedAnnotations(self):
        TestAnnotation = JClass("jpype.annotation.TestAnnotation")
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")

        @TestAnnotation(name="test")
        @TestSimpleAnnotation(4)
        def fun():
            ...

    def testMarkerAnnotation(self):
        TestMarkerAnnotation = JClass("jpype.annotation.TestMarkerAnnotation")

        @TestMarkerAnnotation
        def fun():
            ...
