#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
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
from jpype import java, JObject, JPackage, JString
from . import common

class ObjectWrapperTestCase(common.JPypeTestCase) :
    def testCallOverloads(self) :
        # build the harness
        h = JPackage("jpype.objectwrapper").Test1()

        o = java.lang.Integer(1)
        self.assertEqual(h.Method1(JObject(o, java.lang.Number)), 1)
        self.assertEqual(h.Method1(o), 2)
        self.assertEqual(h.Method1(JObject(java.lang.Integer(1),
                                           java.lang.Object)), 3)
        self.assertEqual(h.Method1(JString("")), 4)

    def testDefaultTypeNameString(self):
        self.assertEqual(JObject("123").typeName, "java.lang.String")

    def testDefaultTypeNameBoolean(self):
        self.assertEqual(JObject(True).typeName, "java.lang.Boolean")
        self.assertEqual(JObject(False).typeName, "java.lang.Boolean")

    @unittest.skip("This seems to be a bug in _jwrapper.py _getDefaultTypeName")
    def testDefaultTypeNameJavaClass(self):
        o = java.lang.String
        self.assertEqual(JObject(o).typeName, "java.lang.Class")
