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


class VirtualsTestCase(common.JPypeTestCase):
    """ Test that we get the right method regardless of how we call it.

    Okay JNI determines the call type based on the return type, thus
    to make sure we are hitting every possible path we have to hit
    every possible return, in every way it can be called.
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.vt = jpype.JClass('jpype.types.VirtualTest')

    def testCallBooleanImplements(self):
        v1 = self.vt.getBooleanImplements()
        self.assertEqual(v1.get(), True)
        self.assertEqual(self.vt.BooleanSupplier.get(v1), True)
        self.assertEqual(self.vt.ClassBooleanSupplier.get(v1), True)

    def testCallBooleanExtends(self):
        v1 = self.vt.getBooleanExtends()
        self.assertEqual(v1.get(), False)
        self.assertEqual(self.vt.BooleanSupplier.get(v1), False)
        self.assertEqual(self.vt.ClassBooleanSupplier.get(v1), True)

    def testCallBooleanAnon(self):
        v1 = self.vt.getBooleanAnon()
        self.assertEqual(v1.get(), False)
        self.assertEqual(self.vt.BooleanSupplier.get(v1), False)
        with self.assertRaises(TypeError):
            self.assertEqual(self.vt.ClassBooleanSupplier.get(v1), True)

    def testCallBooleanAnonExtends(self):
        v1 = self.vt.getBooleanAnonExtends()
        self.assertEqual(v1.get(), False)
        self.assertEqual(self.vt.BooleanSupplier.get(v1), False)
        self.assertEqual(self.vt.ClassBooleanSupplier.get(v1), True)

    def testCallCharImplements(self):
        v1 = self.vt.getCharImplements()
        self.assertEqual(v1.get(), '1')
        self.assertEqual(self.vt.CharSupplier.get(v1), '1')
        self.assertEqual(self.vt.ClassCharSupplier.get(v1), '1')

    def testCallCharExtends(self):
        v1 = self.vt.getCharExtends()
        self.assertEqual(v1.get(), '2')
        self.assertEqual(self.vt.CharSupplier.get(v1), '2')
        self.assertEqual(self.vt.ClassCharSupplier.get(v1), '1')

    def testCallCharAnon(self):
        v1 = self.vt.getCharAnon()
        self.assertEqual(v1.get(), '3')
        self.assertEqual(self.vt.CharSupplier.get(v1), '3')
        with self.assertRaises(TypeError):
            self.assertEqual(self.vt.ClassCharSupplier.get(v1), '1')

    def testCallByteImplements(self):
        v1 = self.vt.getByteImplements()
        self.assertEqual(v1.get(), 1)
        self.assertEqual(self.vt.ByteSupplier.get(v1), 1)
        self.assertEqual(self.vt.ClassByteSupplier.get(v1), 1)

    def testCallByteExtends(self):
        v1 = self.vt.getByteExtends()
        self.assertEqual(v1.get(), 2)
        self.assertEqual(self.vt.ByteSupplier.get(v1), 2)
        self.assertEqual(self.vt.ClassByteSupplier.get(v1), 1)

    def testCallByteAnon(self):
        v1 = self.vt.getByteAnon()
        self.assertEqual(v1.get(), 3)
        self.assertEqual(self.vt.ByteSupplier.get(v1), 3)
        with self.assertRaises(TypeError):
            self.assertEqual(self.vt.ClassByteSupplier.get(v1), 1)

    def testCallByteAnonExtends(self):
        v1 = self.vt.getByteAnonExtends()
        self.assertEqual(v1.get(), 4)
        self.assertEqual(self.vt.ByteSupplier.get(v1), 4)
        self.assertEqual(self.vt.ClassByteSupplier.get(v1), 1)

    def testCallCharAnonExtends(self):
        v1 = self.vt.getCharAnonExtends()
        self.assertEqual(v1.get(), '4')
        self.assertEqual(self.vt.CharSupplier.get(v1), '4')
        self.assertEqual(self.vt.ClassCharSupplier.get(v1), '1')

    def testCallShortImplements(self):
        v1 = self.vt.getShortImplements()
        self.assertEqual(v1.get(), 1)
        self.assertEqual(self.vt.ShortSupplier.get(v1), 1)
        self.assertEqual(self.vt.ClassShortSupplier.get(v1), 1)

    def testCallShortExtends(self):
        v1 = self.vt.getShortExtends()
        self.assertEqual(v1.get(), 2)
        self.assertEqual(self.vt.ShortSupplier.get(v1), 2)
        self.assertEqual(self.vt.ClassShortSupplier.get(v1), 1)

    def testCallShortAnon(self):
        v1 = self.vt.getShortAnon()
        self.assertEqual(v1.get(), 3)
        self.assertEqual(self.vt.ShortSupplier.get(v1), 3)
        with self.assertRaises(TypeError):
            self.assertEqual(self.vt.ClassShortSupplier.get(v1), 1)

    def testCallShortAnonExtends(self):
        v1 = self.vt.getShortAnonExtends()
        self.assertEqual(v1.get(), 4)
        self.assertEqual(self.vt.ShortSupplier.get(v1), 4)
        self.assertEqual(self.vt.ClassShortSupplier.get(v1), 1)

    def testCallIntegerImplements(self):
        v1 = self.vt.getIntegerImplements()
        self.assertEqual(v1.get(), 1)
        self.assertEqual(self.vt.IntegerSupplier.get(v1), 1)
        self.assertEqual(self.vt.ClassIntegerSupplier.get(v1), 1)

    def testCallIntegerExtends(self):
        v1 = self.vt.getIntegerExtends()
        self.assertEqual(v1.get(), 2)
        self.assertEqual(self.vt.IntegerSupplier.get(v1), 2)
        self.assertEqual(self.vt.ClassIntegerSupplier.get(v1), 1)

    def testCallIntegerAnon(self):
        v1 = self.vt.getIntegerAnon()
        self.assertEqual(v1.get(), 3)
        self.assertEqual(self.vt.IntegerSupplier.get(v1), 3)
        with self.assertRaises(TypeError):
            self.assertEqual(self.vt.ClassIntegerSupplier.get(v1), 1)

    def testCallIntegerAnonExtends(self):
        v1 = self.vt.getIntegerAnonExtends()
        self.assertEqual(v1.get(), 4)
        self.assertEqual(self.vt.IntegerSupplier.get(v1), 4)
        self.assertEqual(self.vt.ClassIntegerSupplier.get(v1), 1)

    def testCallLongImplements(self):
        v1 = self.vt.getLongImplements()
        self.assertEqual(v1.get(), 1)
        self.assertEqual(self.vt.LongSupplier.get(v1), 1)
        self.assertEqual(self.vt.ClassLongSupplier.get(v1), 1)

    def testCallLongExtends(self):
        v1 = self.vt.getLongExtends()
        self.assertEqual(v1.get(), 2)
        self.assertEqual(self.vt.LongSupplier.get(v1), 2)
        self.assertEqual(self.vt.ClassLongSupplier.get(v1), 1)

    def testCallLongAnon(self):
        v1 = self.vt.getLongAnon()
        self.assertEqual(v1.get(), 3)
        self.assertEqual(self.vt.LongSupplier.get(v1), 3)
        with self.assertRaises(TypeError):
            self.assertEqual(self.vt.ClassLongSupplier.get(v1), 1)

    def testCallLongAnonExtends(self):
        v1 = self.vt.getLongAnonExtends()
        self.assertEqual(v1.get(), 4)
        self.assertEqual(self.vt.LongSupplier.get(v1), 4)
        self.assertEqual(self.vt.ClassLongSupplier.get(v1), 1)

    def testCallFloatImplements(self):
        v1 = self.vt.getFloatImplements()
        self.assertEqual(v1.get(), 1)
        self.assertEqual(self.vt.FloatSupplier.get(v1), 1)
        self.assertEqual(self.vt.ClassFloatSupplier.get(v1), 1)

    def testCallFloatExtends(self):
        v1 = self.vt.getFloatExtends()
        self.assertEqual(v1.get(), 2)
        self.assertEqual(self.vt.FloatSupplier.get(v1), 2)
        self.assertEqual(self.vt.ClassFloatSupplier.get(v1), 1)

    def testCallFloatAnon(self):
        v1 = self.vt.getFloatAnon()
        self.assertEqual(v1.get(), 3)
        self.assertEqual(self.vt.FloatSupplier.get(v1), 3)
        with self.assertRaises(TypeError):
            self.assertEqual(self.vt.ClassFloatSupplier.get(v1), 1)

    def testCallFloatAnonExtends(self):
        v1 = self.vt.getFloatAnonExtends()
        self.assertEqual(v1.get(), 4)
        self.assertEqual(self.vt.FloatSupplier.get(v1), 4)
        self.assertEqual(self.vt.ClassFloatSupplier.get(v1), 1)

    def testCallDoubleImplements(self):
        v1 = self.vt.getDoubleImplements()
        self.assertEqual(v1.get(), 1)
        self.assertEqual(self.vt.DoubleSupplier.get(v1), 1)
        self.assertEqual(self.vt.ClassDoubleSupplier.get(v1), 1)

    def testCallDoubleExtends(self):
        v1 = self.vt.getDoubleExtends()
        self.assertEqual(v1.get(), 2)
        self.assertEqual(self.vt.DoubleSupplier.get(v1), 2)
        self.assertEqual(self.vt.ClassDoubleSupplier.get(v1), 1)

    def testCallDoubleAnon(self):
        v1 = self.vt.getDoubleAnon()
        self.assertEqual(v1.get(), 3)
        self.assertEqual(self.vt.DoubleSupplier.get(v1), 3)
        with self.assertRaises(TypeError):
            self.assertEqual(self.vt.ClassDoubleSupplier.get(v1), 1)

    def testCallDoubleAnonExtends(self):
        v1 = self.vt.getDoubleAnonExtends()
        self.assertEqual(v1.get(), 4)
        self.assertEqual(self.vt.DoubleSupplier.get(v1), 4)
        self.assertEqual(self.vt.ClassDoubleSupplier.get(v1), 1)

    def testCallObjectImplements(self):
        v1 = self.vt.getObjectImplements()
        self.assertEqual(v1.get(), 'implements')
        self.assertEqual(self.vt.ObjectSupplier.get(v1), "implements")
        self.assertEqual(self.vt.ClassObjectSupplier.get(v1), "implements")

    def testCallObjectExtends(self):
        v1 = self.vt.getObjectExtends()
        self.assertEqual(v1.get(), 'extends')
        self.assertEqual(self.vt.ObjectSupplier.get(v1), "extends")
        self.assertEqual(self.vt.ClassObjectSupplier.get(v1), "implements")

    def testCallObjectAnon(self):
        v1 = self.vt.getObjectAnon()
        self.assertEqual(v1.get(), 'anon')
        self.assertEqual(self.vt.ObjectSupplier.get(v1), "anon")
        with self.assertRaises(TypeError):
            self.assertEqual(self.vt.ClassObjectSupplier.get(v1), "implements")

    def testCallObjectAnonExtends(self):
        v1 = self.vt.getObjectAnonExtends()
        self.assertEqual(v1.get(), 'anon-override')
        self.assertEqual(self.vt.ObjectSupplier.get(v1), "anon-override")
        self.assertEqual(self.vt.ClassObjectSupplier.get(v1), "implements")

    # Success is not segfaulting
    def testCallVoidImplements(self):
        v1 = self.vt.getVoidImplements()
        v1.get()
        self.vt.VoidSupplier.get(v1)
        self.vt.ClassVoidSupplier.get(v1)

    def testCallVoidExtends(self):
        v1 = self.vt.getVoidExtends()
        v1.get()
        self.vt.VoidSupplier.get(v1)
        self.vt.ClassVoidSupplier.get(v1)

    def testCallVoidAnon(self):
        v1 = self.vt.getVoidAnon()
        v1.get()
        self.vt.VoidSupplier.get(v1)
        with self.assertRaises(TypeError):
            self.vt.ClassVoidSupplier.get(v1)

    def testCallVoidAnonExtends(self):
        v1 = self.vt.getVoidAnonExtends()
        v1.get()
        self.vt.VoidSupplier.get(v1)
        self.vt.ClassVoidSupplier.get(v1)
