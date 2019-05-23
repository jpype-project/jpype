# *****************************************************************************
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
# *****************************************************************************
import jpype
from . import common

# Test the methods in JClass
#   __getattribute__
#   __setattr__
#   mro (not tested)
#   class_
class JClassTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testGetAttrPyMethod(self):
        cls = jpype.JClass('java.util.Iterator')
        obj = jpype.JClass('java.util.ArrayList')()
        obj.add(123)
        # Static python methods should be the same when accessed as classes or objects
        self.assertEqual(obj.iterator().next(), cls.next(obj.iterator()))

    def testGetAttrStaticMethod(self):
        cls = jpype.JClass('java.lang.Long')
        obj = cls(10)
        # Static java methods should be the same when accessed as classes or objects
        self.assertEqual(cls.bitCount(123), obj.bitCount(123))

    def testGetAttrStaticField(self):
        cls = jpype.JClass('java.lang.String')
        obj = cls("foo")
        # Static fields should be the same when accessed as classes or objects
        self.assertEqual(cls.CASE_INSENSITIVE_ORDER, obj.CASE_INSENSITIVE_ORDER)

    def testSetAttrField(self):
        cls = jpype.JClass('java.lang.String')
        # Setting a private field on a Java class is allowed
        cls._allowed = 1
        with self.assertRaises(AttributeError):
            # Setting a public field on a Java class is forbidden
            cls.forbidden = 1

    def testSetAttrFinal(self):
        cls = jpype.JClass('java.lang.Long')
        with self.assertRaises(AttributeError):
            # Setting a final field is forbidden
            cls.SIZE = 1

    def testClass(self):
        cls = jpype.JClass('java.lang.Long')
        clsType = jpype.JClass('java.lang.Class')
        # Get class must return a java.lang.Class instance
        self.assertIsInstance(cls.class_, clsType)
        self.assertEqual(cls.class_.getSimpleName(), "Long")

