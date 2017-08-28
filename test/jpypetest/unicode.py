#*****************************************************************************
#   Copyright 2017 Robbert Segeren
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
from jpype import java, JArray, JByte
from . import common


class UnicodeTestCase(common.JPypeTestCase):

    def testToString(self):
        UTF_8 = java.nio.charset.StandardCharsets.UTF_8
        smiley = java.lang.String(b'\xF0\x9F\x98\x8A', UTF_8)
        self.assertEqual(smiley.toString(), u'\U0001f60a')

    def testFromUnicode(self):
        smiley = java.lang.String(u'\U0001f60a')
        self.assertEqual(smiley.toString(), u'\U0001f60a')

    def testUTF8Encodable(self):
        smiley = java.lang.String(b'\xF0\x9F\x98\x8A'.decode('utf8'))
        self.assertEqual(smiley.toString().encode('utf8'), b'\xf0\x9f\x98\x8a')

    def testBytes(self):
        UTF_8 = java.nio.charset.StandardCharsets.UTF_8
        smiley = java.lang.String(JArray(JByte)([0xF0, 0x9F, 0x98, 0x8A]), UTF_8)
        self.assertEqual(list(smiley.getBytes(UTF_8)), [0xF0, 0x9F, 0x98, 0x8A])
