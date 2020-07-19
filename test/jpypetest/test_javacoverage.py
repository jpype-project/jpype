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
import _jpype
import jpype
import _jpype
from jpype.types import *
from jpype import java, JImplements, JOverride
import common


class JavaCoverageTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = JClass('jpype.common.Fixture')()
        JPypeContext = JClass('org.jpype.JPypeContext')
        self.inst = JPypeContext.getInstance()

    def testTypeFactory(self):
        self.assertNotEqual(self.inst.getTypeFactory(), None)

    def testContext(self):
        self.assertEqual(self.inst.collectRectangular(None), None)
        self.assertEqual(self.inst.collectRectangular(JString('hello')), None)
        self.assertEqual(self.inst.collectRectangular(
            JArray(JObject, 2)([JArray(JObject)(0)])), None)
        self.assertEqual(self.inst.collectRectangular(
            JArray(JObject)([None, None])), None)
        self.assertEqual(self.inst.getExcValue(None), 0)

    def testReference(self):
        JPypeReference = JClass('org.jpype.ref.JPypeReference')
        u = JPypeReference(None, None, 0, 0)
        u2 = JPypeReference(None, None, 1, 0)
        self.assertTrue(u.equals(u))
        self.assertFalse(u.equals(u2))
        self.assertFalse(u.equals(JString("a")))

    def testModifiers(self):
        cls = JClass('org.jpype.manager.ModifierCode')
        self.assertEqual(cls.get(cls.decode(1171)), 1171)

    def testTypeFactory(self):
        TypeFactory = JClass("org.jpype.manager.TypeFactory")
        TypeManager = JClass("org.jpype.manager.TypeManager")
        @JImplements(TypeFactory)
        class TF(object):
            def __init__(self):
                self.id = 0
                self.entities = {}

            def define(self, name):
                self.id += 1
                self.entities[self.id] = name
                return self.id

            @JOverride
            def newWrapper(self, context, cls):
                pass

            @JOverride
            def defineArrayClass(self, context, cls, name, superClass, componentPtr, modifiers):
                return self.define(name)

            @JOverride
            def defineObjectClass(self, context, cls, name, superClass, interfaces, modifiers):
                return self.define(name)

            @JOverride
            def definePrimitive(self, context, name, cls, boxedPtr, modifiers):
                return self.define(name)

            @JOverride
            def assignMembers(self, context, cls, ctorMethod, methodList, fieldList):
                return

            @JOverride
            def defineField(self, context, cls, name, field, fieldType, modifiers):
                return self.define(name)

            @JOverride
            def defineMethod(self, context, cls, name, method, overloadList, modifiers):
                return self.define(name)

            @JOverride
            def populateMethod(self, context, method, returnType, argumentTypes):
                return

            @JOverride
            def defineMethodDispatch(self, context, cls, name, overloadList, modifiers):
                return self.define(name)

            @JOverride
            def destroy(self, context, resources, sz):
                for i in range(sz):
                    del self.entities[resources[i]]
        manager = TypeManager()
        factory = TF()
        manager = TypeManager(62621463, factory)
        manager.init()

        # Can only be initialized once
        with self.assertRaises(JException):
            manager.init()

        self.assertEqual(
            factory.entities[manager.findClassByName('boolean')], 'boolean')
        self.assertEqual(
            factory.entities[manager.findClassByName('byte')], 'byte')
        self.assertEqual(
            factory.entities[manager.findClassByName('char')], 'char')
        self.assertEqual(
            factory.entities[manager.findClassByName('short')], 'short')
        self.assertEqual(
            factory.entities[manager.findClassByName('int')], 'int')
        self.assertEqual(
            factory.entities[manager.findClassByName('long')], 'long')
        self.assertEqual(
            factory.entities[manager.findClassByName('float')], 'float')
        self.assertEqual(
            factory.entities[manager.findClassByName('double')], 'double')

        self.assertEqual(
            factory.entities[manager.findClass(JBoolean)], "boolean")
        self.assertEqual(factory.entities[manager.findClass(JChar)], "char")
        self.assertEqual(factory.entities[manager.findClass(JByte)], "byte")
        self.assertEqual(factory.entities[manager.findClass(JShort)], "short")
        self.assertEqual(factory.entities[manager.findClass(JInt)], "int")
        self.assertEqual(factory.entities[manager.findClass(JLong)], "long")
        self.assertEqual(factory.entities[manager.findClass(JFloat)], "float")
        self.assertEqual(
            factory.entities[manager.findClass(JDouble)], "double")

        self.assertEqual(manager.populateMethod(0, None), None)

        sb = JClass("java.lang.StringBuilder")
        with self.assertRaises(JException):
            manager.populateMembers(sb)
        manager.shutdown()

        # See if we leaked any entities
        self.assertEqual(len(factory.entities), 0)
