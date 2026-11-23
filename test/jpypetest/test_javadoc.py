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

    def testEntitiesAdjacent(self):
        html = JClass("org.jpype.html.Html")
        for text, expected in [("&lt;&gt;", "<>"),
                               ("{@code a &amp;&amp; b}", "{@code a && b}"),
                               ("Map&lt;String, List&lt;String&gt;&gt;",
                                "Map<String, List<String>>"),
                               ("x &lt;&lt; 2", "x << 2"),
                               ("&#65;&#66;&#x43;", "ABC")]:
            self.assertEqual(str(html.decode(text)), expected)

    def testEntitiesMalformed(self):
        # Documentation text must never lose its docstring to a decode error;
        # anything that is not a well formed entity is left as written.
        html = JClass("org.jpype.html.Html")
        for text in ["AT&T", "a & b", "trailing &", "&unknown;", "&;",
                     "&#;", "&#xZZ;", "R&D; &", "&amp"]:
            self.assertEqual(str(html.decode(text)), text)

    def testEntitiesNumeric(self):
        html = JClass("org.jpype.html.Html")
        # Above the BMP: one code point (a surrogate pair in Java)
        self.assertEqual(str(html.decode("&#128512;")), "\U0001F600")
        self.assertEqual(str(html.decode("&#x1F600;")), "\U0001F600")
        # NUL, lone surrogates and out-of-range become U+FFFD
        for ref in ["&#0;", "&#xD800;", "&#1114112;", "&#99999999999999;"]:
            self.assertEqual(str(html.decode("x" + ref + "y")), "x\ufffdy")
        # Missing semicolon on a numeric reference is tolerated
        self.assertEqual(str(html.decode("&#65 b")), "A b")

    def testRendererLongFirstWord(self):
        # A first word longer than the line width must not lose its first
        # character to a line break.
        R = JClass("org.jpype.javadoc.JavadocRenderer")
        m = R.class_.getDeclaredMethod(
            "formatWidth", java.lang.StringBuilder, java.lang.String,
            java.lang.Integer.TYPE, java.lang.Integer.TYPE, java.lang.Boolean.TYPE)
        m.setAccessible(True)
        word = "A" * 130
        sb = java.lang.StringBuilder()
        m.invoke(None, sb, word + " tail", java.lang.Integer(120),
                 java.lang.Integer(0), java.lang.Boolean(False))
        out = str(sb.toString())
        self.assertTrue(out.startswith(word), out[:20])
        self.assertIn("tail", out)

    def testClass(self):
        JC = jpype.JClass("jpype.doc.Test")
        jd = JC.__doc__
        self.assertIsInstance(jd, str)
        self.assertRegex(jd, "random stuff")
        # A cross-reference to a JDK platform class (java.lang.Object). This
        # is the case that silently produced mangled output on JDK 17+
        # (https://... treated as a local reference) - assert it resolved
        # to something readable rather than a scraped-URL path.
        self.assertIn("Object", jd)
        self.assertNotIn("docs.oracle.com.en.java", jd)

    def testMethod(self):
        JC = jpype.JClass("jpype.doc.Test")
        jd = JC.methodOne.__doc__
        self.assertIsInstance(jd, str)
        self.assertRegex(jd, "something special")
        self.assertIn("Parameters:", jd)
        self.assertIn("Returns:", jd)
        self.assertNotIn("docs.oracle.com.en.java", jd)
