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
import common
import jpype
from jpype._jclass import *
from jpype.types import *
from jpype.imports import *


class JExtensionTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testExtendObject(self):
        from java.lang import Object
        class MyObject(Object):

            @JPublic
            def __init__(self):
                pass

        self.assertIsInstance(MyObject(), MyObject)


    def testInitOnce(self):
        from java.lang import Object
        count = 0
        class MyObject(Object):

            @JPublic
            def __init__(self):
                nonlocal count
                count += 1

        MyObject()
        self.assertEqual(count, 1)

    def testOverrideSimple(self):
        from java.lang import Object, String
        class MyObject(Object):

            @JPublic
            def __init__(self):
                pass

            @JPublic
            @JOverride
            def toString(self) -> String:
                return "test"

        self.assertEqual(str(MyObject()), "test")

    def testOverloadConstructor(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                pass

            @JPublic
            @JOverride
            def __init__(self, i: JInt):
                pass

            @JPublic
            @JOverride
            def __init__(self, o: JObject):
                pass

            @JPublic
            @JOverride
            def identity(self, i: JInt) -> JInt:
                return 0

            @JPublic
            @JOverride
            def identity(self, o: JObject) -> JObject:
                return None

        o = MyObject()
        self.assertEqual(o.identity(1), 0)
        self.assertEqual(o.identity(JObject()), None)

