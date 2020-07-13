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
from jpype.types import *
from jpype import java
import common
#import os
#import sys


class ObjectWrapperTestCase(common.JPypeTestCase):
    def testCallOverloads(self):
        # build the harness
        h = JClass("jpype.objectwrapper.Test1")()

        o = java.lang.Integer(1)
        self.assertEqual(h.Method1(JObject(o, java.lang.Number)), 1)
        self.assertEqual(h.Method1(o), 2)
        self.assertEqual(h.Method1(JObject(java.lang.Integer(1),
                                           java.lang.Object)), 3)
        self.assertEqual(h.Method1(JString("")), 4)

    def testDefaultTypeNameString(self):
        self.assertEqual(type(JObject("123")),
                         jpype.JClass("java.lang.String"))

    def testDefaultTypeNameBoolean(self):
        self.assertEqual(type(JObject(True)),
                         jpype.JClass("java.lang.Boolean"))
        self.assertEqual(type(JObject(False)),
                         jpype.JClass("java.lang.Boolean"))

    def testPassingClassTypeSucceeds(self):
        h = JClass("jpype.objectwrapper.Test1")()
        # Select a convenient java.lang.Class object
        class_obj = h.getClass()

        # Check that funneling Class obj through java doesn't convert to null
        result = h.ReturnObject(class_obj)

        self.assertEqual(class_obj, result)
        self.assertNotEqual(result, None)

    def testWrapJavaClass(self):
        o = java.lang.String
        self.assertEqual(type(JObject(o)), jpype.JClass("java.lang.Class"))

    def testWrapJavaObject(self):
        o = java.lang.String("foo")
        self.assertEqual(type(JObject(o)), jpype.JClass("java.lang.String"))

    def testWrapJavaObjectCast(self):
        o = java.lang.String("foo")
        c = java.lang.Object
        self.assertEqual(type(JObject(o, c)), jpype.JClass("java.lang.Object"))

    def testWrapJavaObjectCastFail(self):
        o = java.lang.Object()
        c = java.lang.Math
        with self.assertRaises(TypeError):
            f = JObject(o, c)

    def testWrapJavaPrimitiveCast(self):
        c = java.lang.Object
        self.assertEqual(type(JObject("foo", c)),
                         jpype.JClass("java.lang.Object"))
        self.assertEqual(type(JObject(True, c)),
                         jpype.JClass("java.lang.Object"))
        self.assertEqual(type(JObject(False, c)),
                         jpype.JClass("java.lang.Object"))
        self.assertEqual(type(JObject(1, c)), jpype.JClass("java.lang.Object"))
        self.assertEqual(type(JObject(1.0, c)),
                         jpype.JClass("java.lang.Object"))

    def testWrapJavaPrimitiveBox(self):
        self.assertEqual(type(JObject("foo", JString)),
                         jpype.JClass("java.lang.String"))
        self.assertEqual(type(JObject(1, JInt)),
                         jpype.JClass("java.lang.Integer"))
        self.assertEqual(type(JObject(1, JLong)),
                         jpype.JClass("java.lang.Long"))
        self.assertEqual(type(JObject(1.0, JFloat)),
                         jpype.JClass("java.lang.Float"))
        self.assertEqual(type(JObject(1.0, JDouble)),
                         jpype.JClass("java.lang.Double"))
        self.assertEqual(type(JObject(1, JShort)),
                         jpype.JClass("java.lang.Short"))

    def testJObjectBadType(self):
        class Fred(object):
            pass
        with self.assertRaises(TypeError):
            jpype.JObject(1, Fred)

    def testJObjectUnknownObject(self):
        class Fred(object):
            pass
        with self.assertRaises(TypeError):
            jpype.JObject(Fred())


#     def testMakeSureWeCanLoadAllClasses(self):
#         def get_system_jars():
#             for dirpath,_,files in os.walk(jpype.java.lang.System.getProperty("java.home")):
#                 for file in files:
#                     if file.endswith('.jar'):
#                         yield (os.path.join(dirpath,file))
#         for jar in get_system_jars():
#             classes = [x.getName() for x in jpype.java.util.jar.JarFile(jar).entries() if x.getName().endswith('.class')]
#             classes = [x.replace('/','.')[:-6] for x in classes]
#             for clazz in classes:
#                 try:
#                     jpype.JClass(clazz)
#                 except jpype.JavaException as exception:
#                     if not 'not found' in exception.message():
#                         print(clazz)
#                         print (exception.message())
#                         #print (exception.stacktrace())
#                 except:
#                     print(sys.exc_info()[0])
#                     pass
