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


class JCallerSensitiveCase(common.JPypeTestCase):
    """ Test for caller sensitive methods.

    Java uses a special pattern to deal with the security manager.  It
    uses the stack to find the callers class and then decides if the
    call is permitted.  But when we call from JNI there is no class.
    Thus, we need a special caller pattern which proxies to Java and
    then calls the method.

    This alternative method has to be tested against all of the
    different patterns (static, member), returning (void, primitive, object),
    called with (nothing, object, primitive, many, varargs)

    Unfortunately, the actual problematic method in Java is private,
    so we can't get to it directly.  Thus will will perform indirect tests.

    For now we do not support caller sensitive constructors.
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not jpype.getJVMVersion() > (1, 8, 0):
            raise common.unittest.SkipTest

        self.Class = jpype.JClass("jpype.method.Caller")
        self.obj = self.Class()

    def testCallStatic(self):
        self.assertIsInstance(self.Class.callObjectStatic(), self.Class)

    def testCallStaticAsMember(self):
        self.assertIsInstance(self.obj.callObjectStatic(), self.Class)

    def testCallMember(self):
        self.assertIsInstance(self.obj.callObjectMember(), self.Class)

    def testCallMemberFromClass(self):
        self.assertIsInstance(
            self.Class.callObjectMember(self.obj), self.Class)

    def testCallVoidStatic(self):
        self.assertEqual(self.Class.callVoidStatic(), None)

    def testCallVoidStaticAsMember(self):
        self.assertEqual(self.obj.callVoidStatic(), None)

    def testCallVoidMemberFromClass(self):
        self.assertEqual(self.Class.callVoidMember(self.obj), None)

    def testCallIntegerStatic(self):
        self.assertEqual(self.Class.callIntegerStatic(), 123)

    def testCallIntegerMember(self):
        self.assertEqual(self.obj.callIntegerMember(), 123)

    def testCallIntegerStaticAsMember(self):
        self.assertEqual(self.obj.callIntegerStatic(), 123)

    def testCallIntegerMemberFromClass(self):
        self.assertEqual(self.Class.callIntegerMember(self.obj), 123)

    def testArgs(self):
        self.assertEqual(self.obj.callArgs(1, 2), 2)

    def testArgsFromClass(self):
        self.assertEqual(self.Class.callArgs(self.obj, 1, 2), 2)

    def testPrimitive(self):
        self.assertEqual(self.obj.callArg1(123), 123)

    def testPrimitiveFromClass(self):
        self.assertEqual(self.Class.callArg1(self.obj, 125), 125)

    def testVarArgs(self):
        self.assertEqual(tuple(self.obj.callVarArgs(1, 2, 3)), (2, 3))

    def testVarArgsFromClass(self):
        self.assertEqual(
            tuple(self.Class.callVarArgs(self.obj, 1, 2, 3)), (2, 3))

    def testDeclaredMethod(self):
        self.assertIsInstance(jpype.java.lang.Object.class_.getDeclaredMethod(
            'wait'), jpype.java.lang.reflect.Method)

    def testStackWalker1(self):
        with self.assertRaises(jpype.java.lang.IllegalCallerException):
            self.obj.callStackWalker1()

    def testStackWalker2(self):
        self.assertEqual(self.obj.callStackWalker2(), jpype.JClass(
            jpype.java.lang.Class.forName("org.jpype.JPypeContext")).class_)
