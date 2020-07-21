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
import sys
import jpype
from jpype import JPackage, JArray, JByte, java
import common


class ReflectCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.test1 = jpype.JClass('jpype.overloads.Test1')()
        self.Reflect = jpype.JClass('jpype.reflect.ReflectionTest')
        self.Annotation = jpype.JClass('jpype.reflect.Annotation')

    def testClass(self):
        t = jpype.JClass('java.lang.Object')
        obj = self.Reflect()
        self.assertEqual('Class', self.test1.testClassVsObject(self.Reflect))
        self.assertEqual(
            'Class', self.test1.testClassVsObject(self.Reflect.class_))
        self.assertEqual(
            'Class', self.test1.testClassVsObject(obj.getClass()))
        self.assertEqual(
            'Class', self.test1.testClassVsObject(obj.__class__.class_))

    def testAnnotation(self):
        method = self.Reflect.class_.getMethod('annotatedMethod')
        annotation = method.getAnnotation(self.Annotation)
        self.assertEqual('annotation', annotation.value())

    def testCallPublicMethod(self):
        method = self.Reflect.class_.getMethod('publicMethod')
        obj = self.Reflect()
        self.assertIsNotNone(obj)
        self.assertIsNotNone(method)
        self.assertEqual('public', method.invoke(obj))

    def testCallPrivateMethod(self):
        method = self.Reflect.class_.getDeclaredMethod('privateMethod')
        method.setAccessible(True)
        obj = self.Reflect()
        self.assertIsNotNone(obj)
        self.assertIsNotNone(method)
        self.assertEqual('private', method.invoke(obj))

    def testAccessPublicField(self):
        field = self.Reflect.class_.getField('publicField')
        obj = self.Reflect()
        self.assertIsNotNone(obj)
        self.assertIsNotNone(field)
        self.assertEqual('public', field.get(obj))

    def testAccessPrivateField(self):
        field = self.Reflect.class_.getDeclaredField('privateField')
        obj = self.Reflect()
        field.setAccessible(True)
        self.assertIsNotNone(obj)
        self.assertIsNotNone(field)
        self.assertEqual('private', field.get(obj))
