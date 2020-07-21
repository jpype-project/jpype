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
import sys
import logging
import time
import common


class ConversionCharSequenceTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testAutoConvert(self):
        Instant = jpype.JClass("java.time.Instant")
        now = "2019-11-12T03:20:54.710948400Z"
        then = Instant.parse(now)
        self.assertEqual(str(then), now)
        then = Instant.parse(jpype.JString(now))
        self.assertEqual(str(then), now)
        then = Instant.parse(jpype.JObject(now, "java.lang.CharSequence"))
        self.assertEqual(str(then), now)
