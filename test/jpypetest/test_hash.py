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
import sys


class HashTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testHashString(self):
        self.assertIsNotNone(hash(jpype.java.lang.String("upside down")))
        self.assertIsNotNone(hash(jpype.JString("upside down")))
        self.assertEqual(hash(jpype.JString("upside down")),
                         hash("upside down"))

    def testHashArray(self):
        self.assertIsNotNone(hash(jpype.JArray(jpype.JInt)([1, 2, 3])))

    def testHashObject(self):
        self.assertIsNotNone(hash(jpype.java.lang.Object()))

    def testHashBoolean(self):
        self.assertIsNotNone(hash(jpype.java.lang.Boolean(True)))
        self.assertEqual(hash(jpype.java.lang.Boolean(True)), hash(True))

    def testHashByte(self):
        self.assertIsNotNone(hash(jpype.java.lang.Byte(5)))
        self.assertEqual(hash(jpype.java.lang.Byte(5)), hash(5))

    def testHashChar(self):
        self.assertIsNotNone(hash(jpype.java.lang.Character("a")))
        # Differences in implementation yield different hashes currently.
        #self.assertEqual(hash(jpype.java.lang.Character("a")), hash("a"))

    def testHashShort(self):
        self.assertIsNotNone(hash(jpype.java.lang.Short(1)))
        self.assertEqual(hash(jpype.java.lang.Short(1)), hash(1))

    def testHashLong(self):
        self.assertIsNotNone(hash(jpype.java.lang.Long(55)))
        self.assertEqual(hash(jpype.java.lang.Long(55)), hash(55))

    def testHashInteger(self):
        self.assertIsNotNone(hash(jpype.java.lang.Integer(123)))
        self.assertEqual(hash(jpype.java.lang.Integer(123)), hash(123))

    def testHashFloat(self):
        self.assertIsNotNone(hash(jpype.java.lang.Float(3.141592)))

    def testHashDouble(self):
        self.assertIsNotNone(hash(jpype.java.lang.Double(6.62607004e-34)))

    def testHashNone(self):
        self.assertEqual(hash(None), hash(jpype.JObject(None)))
        q = jpype.JObject(None, jpype.java.lang.Double)
        self.assertEqual(hash(None), hash(jpype.JObject(None)))
