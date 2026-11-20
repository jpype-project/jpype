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
import _jpype
import jpype
from jpype.types import *
from jpype import JPackage
import common


class JPackageTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.jl = JPackage('java.lang')

    def testCreate(self):
        self.assertEqual(self.jl.String, JClass('java.lang.String'))

    def testFail0(self):
        with self.assertRaises(TypeError):
            JPackage(1)

#    def testFail1(self):
#        #with self.assertRaises(RuntimeError):
#        jl = JPackage('java.nosuch')

    def testFail2(self):
        with self.assertRaises(AttributeError):
            self.jl.NoSuch

    def testFail3(self):
        with self.assertRaises(AttributeError):
            self.jl.bar

    def testStr(self):
        self.assertIsInstance(str(self.jl), str)
        self.assertEqual(str(self.jl), "java.lang")

    def testRepr(self):
        self.assertIsInstance(repr(self.jl), str)
        self.assertEqual(repr(self.jl), "<java package 'java.lang'>")

    def testCall(self):
        with self.assertRaises(TypeError):
            self.jl()

    def testDir(self):
        self.assertIsInstance(dir(self.jl), list)

    def testGetAttr(self):
        with self.assertRaises(TypeError):
            self.jl.__getattribute__(object())

    def testSetAttr(self):
        with self.assertRaises(TypeError):
            self.jl.__setattr__(object(), 1)

    def testInvalid(self):
        JL = JPackage("java.lng")
        with self.assertRaisesRegex(AttributeError, "Java package 'java.lng' is not valid"):
            getattr(JL, "foo")


class JPackageLargeClassTestCase(common.JPypeTestCase):
    """ Public classes with a very large constant pool or a very long string
    constant must still be listed (their sizes overflow a signed short). """

    def testLargeClassesAreListed(self):
        import pathlib
        import shutil
        import tempfile
        compiler = jpype.JClass("javax.tools.ToolProvider").getSystemJavaCompiler()
        if compiler is None:
            self.skipTest("no system Java compiler")
        d = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, d, True)
        pkg = pathlib.Path(d) / "src" / "bigpkg"
        pkg.mkdir(parents=True)
        (pkg / "Small.java").write_text("package bigpkg;\npublic class Small {}\n")
        # A 40000 byte UTF8 constant: its length does not fit a signed short
        (pkg / "BigString.java").write_text(
            "package bigpkg;\npublic class BigString {\n"
            "  public static final String S = \"%s\";\n}\n" % ("x" * 40000))
        # Each distinct int constant adds a field name and a constant: more
        # than 32767 constant pool entries in total
        fields = "".join("  public static final int f%d = %d;\n" % (i, 100000 + i)
                         for i in range(17000))
        (pkg / "ManyConst.java").write_text(
            "package bigpkg;\npublic class ManyConst {\n%s}\n" % fields)
        out = pathlib.Path(d) / "classes"
        out.mkdir()
        rc = compiler.run(None, None, None, jpype.JArray(jpype.JString)(
            [str(p) for p in pkg.glob("*.java")] + ["-d", str(out)]))
        self.assertEqual(rc, 0)
        jpype.addClassPath(out.absolute())
        self.assertEqual(sorted(dir(jpype.JPackage("bigpkg"))),
                         ["BigString", "ManyConst", "Small"])
