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
import subrun


def haveJImports():
    try:
        import jpype.imports
        return True
    except ImportError:
        return False


def isJavaClass(tp):
    return isinstance(tp, jpype.JClass)


def isJavaEnum(tp):
    return issubclass(tp, jpype.JClass('java.lang.Enum'))


class ImportsTestCase(common.JPypeTestCase):

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportPackage(self):
        import java.lang
        assert isJavaClass(java.lang.String)

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportClass(self):
        from java.lang import String
        assert isJavaClass(String)

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportClassAs(self):
        from java.lang import String as Str
        assert isJavaClass(Str)

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportClassMultiple(self):
        from java.lang import Number, Integer, Double
        assert isJavaClass(Number)
        assert isJavaClass(Integer)
        assert isJavaClass(Double)

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportStatic(self):
        from java.lang.ProcessBuilder import Redirect
        assert isJavaClass(Redirect)

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportInner(self):
        from java.lang import Character
        assert isJavaClass(Character.UnicodeScript)

    @common.unittest.skipUnless(haveJImports(), "jpype.imports not available")
    def testImportInnerEnum(self):
        from java.lang import Character
        assert isJavaEnum(Character.UnicodeScript)

    def testImportFail(self):
        with self.assertRaises(ImportError):
            from java.lang import NotThere

    def testAlias1(self):
        jpype.imports.registerDomain("jpypex", alias="jpype")
        from jpypex.test.common import Fixture  # type: ignore
        self.assertEqual(Fixture, jpype.JClass("org.jpype.test.common.Fixture"))

    def testAlias2(self):
        jpype.imports.registerDomain("commonx", alias="org.jpype.test.common")
        from commonx import Fixture as Fixture2  # type: ignore
        self.assertEqual(Fixture2, jpype.JClass("org.jpype.test.common.Fixture"))

    def testAliasBad(self):
        jpype.imports.registerDomain("brokenx", alias="jpype.broken")
        with self.assertRaises(ImportError):
            from brokenx import Fixture as Fixture2  # type: ignore

    def testIsPackage(self):
        import java.lang
        self.assertIsInstance(java, jpype.JPackage)
        self.assertIsInstance(java.lang, jpype.JPackage)
        assert not isinstance(java.lang.Class, jpype.JPackage)
        assert issubclass(type(java.lang), jpype.JPackage)

    def testMRJar(self):
        import org.jpype.mrjar as mrjar  # type: ignore
        u = dir(mrjar)
        assert "A" in u
        assert "B" in u
        assert "sub" in u

    def testAddClassPath(self):
        import pathlib
        import org.jpype as ojp
        assert not "late" in dir(ojp)
        with self.assertRaises(ImportError):
            import org.jpype.late as late  # type: ignore
        jar = pathlib.Path(__file__).parent.parent / "jar/late/late.jar"
        assert jar.exists()
        jpype.addClassPath(jar.absolute())
        import org.jpype.test.late as late
        assert "Test" in dir(late)
        #t = late.Test()
        assert t.field == 5
        assert t.method() == "Yes"

    def testStar(self):
        pass
        # fixme: this has the side-effect of loading late and late2 jars, should it go to a subrun test?
        #import importstar

    def testMissing(self):
        import org
        assert "missing" in dir(org.jpype)


@subrun.TestCase
class ImportsBeforeCase(common.unittest.TestCase):
    """JVM not yet started, e.g. we do not derive from JPypeTestCase."""

    def testPre(self):
        assert not jpype.isJVMStarted()
        with self.assertRaises(ImportError):
            import java
        with self.assertRaises(ImportError):
            import java.lang
