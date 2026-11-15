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


class JReflectMethodTestCase(common.JPypeTestCase):
    """ Exercises the ``toPython()`` customizer added to
    ``java.lang.reflect.Method`` (plan/ReflectMethod.md): binds a Python
    callable to exactly one already-resolved overload, skipping jpype's
    normal per-call overload search.
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Math = jpype.JClass("java.lang.Math")
        self.String = jpype.JClass("java.lang.String")

    def testStaticMethod(self):
        m = self.Math.class_.getDeclaredMethod(
            "max", jpype.JInt, jpype.JInt)
        f = m.toPython()
        self.assertEqual(f(3, 5), 5)

    def testInstanceMethodTakesInstanceFirst(self):
        m = self.String.class_.getDeclaredMethod("length")
        f = m.toPython()
        s = self.String("hello")
        self.assertEqual(f(s), 5)

    def testInstanceMethodWithArgs(self):
        m = self.String.class_.getDeclaredMethod("charAt", jpype.JInt)
        f = m.toPython()
        s = self.String("hello")
        self.assertEqual(str(f(s, 1)), "e")

    def testDoesNotFallBackToFullDispatch(self):
        # length() takes no arguments; a single-overload dispatch must not
        # silently accept args that would only match some other same-named
        # overload elsewhere on the class.
        m = self.String.class_.getDeclaredMethod("length")
        f = m.toPython()
        s = self.String("hello")
        with self.assertRaises(TypeError):
            f(s, 1)

    def testRepeatedLookupReusesDispatch(self):
        m1 = self.Math.class_.getDeclaredMethod("max", jpype.JInt, jpype.JInt)
        m2 = self.Math.class_.getDeclaredMethod("max", jpype.JInt, jpype.JInt)
        f1 = m1.toPython()
        f2 = m2.toPython()
        self.assertEqual(f1(1, 2), f2(1, 2))
