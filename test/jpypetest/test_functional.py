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
import common


def callFunctional(cls, i):
    fun = cls @(lambda x: x)
    return fun.call(i)


class FunctionalTestCase(common.JPypeTestCase):
    
    def _checkValidFunctional(self, name, value):
        cls = jpype.JClass("jpype.functional." + name)
        self.assertEqual(callFunctional(cls, value), value)
    
    def testAnnotated(self):
        self._checkValidFunctional("Annotated", 0)

    def testNonAnnotated(self):
        self._checkValidFunctional("NonAnnotated", 1)

    def testExtendsFunctional(self):
        self._checkValidFunctional("ExtendsFunctional", 2)

    def testExtendsNonFunctional(self):
        with self.assertRaises(TypeError):
            self._checkValidFunctional("ExtendsNonFunctional", 3)
    
    def testAnnotatedWithObjectMethods(self):
        self._checkValidFunctional("AnnotatedWithObjectMethods", 4)

    def testNonAnnotatedWithObjectMethods(self):
        self._checkValidFunctional("NonAnnotatedWithObjectMethods", 5)

    def testRedeclaresAnnotated(self):
        self._checkValidFunctional("RedeclaresAnnotated", 6)

    def testRedeclaresNonAnnotated(self):
        self._checkValidFunctional("RedeclaresNonAnnotated", 7)

    def testStaticDefaultMethods(self):
        # Predicate has both static and default methods
        Predicate = jpype.JClass("java.util.function.Predicate")
        fun = Predicate @ (lambda x: True)
        self.assertTrue(fun.test(fun))
