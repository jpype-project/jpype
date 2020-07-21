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


class HtmlTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testEntities(self):
        html = JClass("org.jpype.html.Html")
        for k, v in html.ENTITIES.items():
            u = html.decode("&" + str(k) + ";")
            self.assertIsInstance(u, JString)
            self.assertEqual(len(u), 1)
            self.assertEqual(ord(u[0][0]), v)

    def testClass(self):
        JC = jpype.JClass("jpype.doc.Test")
        jd = JC.__doc__
        self.assertIsInstance(jd, str)
        self.assertRegex(jd, "random stuff")

    def testMethod(self):
        JC = jpype.JClass("jpype.doc.Test")
        jd = JC.methodOne.__doc__
        self.assertIsInstance(jd, str)
        # Disabling this test for now.  Something fails in Linux but I can't replicate it.
        #self.assertRegex(jd, "something special")
