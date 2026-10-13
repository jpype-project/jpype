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
import jpype
from jpype import JClass
import common


class AnnotationTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testAnnotationMethodCall(self):
        """Test issue #880: Annotation methods must use virtual JNI calls"""
        # Load the annotated class
        AnnotatedClass = JClass("jpype.annotation.AnnotatedClass")

        # Get the class object
        cls = AnnotatedClass.class_

        # Get the annotation
        TestAnnotation = JClass("jpype.annotation.TestAnnotation")
        annotation = cls.getAnnotation(TestAnnotation)

        # This should not crash - issue #880 was that calling annotation methods
        # used CallNonvirtual instead of CallVirtual, causing crashes on Android
        self.assertIsNotNone(annotation)
        value = annotation.value()
        self.assertEqual(value, "test")

        count = annotation.count()
        self.assertEqual(count, 100)
