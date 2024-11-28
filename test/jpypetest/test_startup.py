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
import subrun
import os
from pathlib import Path
import unittest
import common

root = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
cp = os.path.join(root, 'classes').replace('\\', '/')


@subrun.TestCase(individual=True)
class StartJVMCase(unittest.TestCase):
    def setUp(self):
        self.jvmpath = jpype.getDefaultJVMPath()

    def testStartup(self):
        with self.assertRaises(OSError):
            jpype.startJVM(convertStrings=False)
            jpype.startJVM(convertStrings=False)

    def testRestart(self):
        with self.assertRaises(OSError):
            jpype.startJVM(convertStrings=False)
            jpype.shutdownJVM()
            jpype.startJVM(convertStrings=False)

    def testInvalidArgsFalse(self):
        with self.assertRaises(RuntimeError):
            jpype.startJVM(
                "-for_sure_InVaLiD",
                ignoreUnrecognized=False, convertStrings=False,
            )

    def testInvalidArgsTrue(self):
        jpype.startJVM(
            "-for_sure_InVaLiD",
            ignoreUnrecognized=True,
            convertStrings=False,
        )

    def testClasspathArgKeyword(self):
        jpype.startJVM(classpath=cp, convertStrings=False)
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testClasspathArgList(self):
        jpype.startJVM(
            classpath=[cp],
            convertStrings=False,
        )
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testClasspathArgListEmpty(self):
        jpype.startJVM(
            classpath=[cp, ''],
            convertStrings=False,
        )
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testClasspathArgDef(self):
        jpype.startJVM('-Djava.class.path=%s' % cp, convertStrings=False)
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testClasspathArgPath(self):
        jpype.startJVM(classpath=Path(cp), convertStrings=False)
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testClasspathArgPathList(self):
        jpype.startJVM(classpath=[Path(cp)], convertStrings=False)
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testClasspathArgGlob(self):
        jpype.startJVM(classpath=os.path.join(cp, '..', 'jar', 'mrjar*'))
        assert jpype.JClass('org.jpype.mrjar.A') is not None

    def testClasspathTwice(self):
        with self.assertRaises(TypeError):
            jpype.startJVM('-Djava.class.path=%s' %
                            cp, classpath=cp, convertStrings=False)

    def testClasspathBadType(self):
        with self.assertRaises(TypeError):
            jpype.startJVM(classpath=1, convertStrings=False)

    def testJVMPathArg_Str(self):
        jpype.startJVM(self.jvmpath, classpath=cp, convertStrings=False)
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testJVMPathArg_None(self):
        # It is allowed to pass None as a JVM path
        jpype.startJVM(
            None,  # type: ignore
            classpath=cp,
        )
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testJVMPathArg_NoArgs(self):
        jpype.startJVM(
            classpath=cp,
        )
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testJVMPathArg_Path(self):
        with self.assertRaises(TypeError):
            jpype.startJVM(
                # Pass a path as the first argument. This isn't supported (this is
                # reflected in the type definition), but the fact that it "works"
                # gives rise to this test.
                Path(self.jvmpath),  # type: ignore
                convertStrings=False,
            )

    def testJVMPathKeyword_str(self):
        jpype.startJVM(
            classpath=cp,
            jvmpath=self.jvmpath,
            convertStrings=False,
        )
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testJVMPathKeyword_Path(self):
        jpype.startJVM(jvmpath=Path(self.jvmpath), classpath=cp, convertStrings=False)
        assert jpype.JClass('jpype.array.TestArray') is not None

    def testPathTwice(self):
        with self.assertRaises(TypeError):
            jpype.startJVM(self.jvmpath, jvmpath=self.jvmpath)

    def testBadKeyword(self):
        with self.assertRaises(TypeError):
            jpype.startJVM(invalid=True)  # type: ignore

    def testNonASCIIPath(self):
        """Test that paths with non-ASCII characters are handled correctly.
        Regression test for https://github.com/jpype-project/jpype/issues/1194
        """
        jpype.startJVM(jvmpath=Path(self.jvmpath), classpath="test/jar/unicode_à😎/sample_package.jar")
        cl = jpype.JClass("java.lang.ClassLoader").getSystemClassLoader()
        self.assertEqual(type(cl), jpype.JClass("org.jpype.JPypeClassLoader"))
        assert dir(jpype.JPackage('org.jpype.sample_package')) == ['A', 'B']


    def testOldStyleNonASCIIPath(self):
        """Test that paths with non-ASCII characters are handled correctly.
        Regression test for https://github.com/jpype-project/jpype/issues/1194
        """
        jpype.startJVM("-Djava.class.path=test/jar/unicode_à😎/sample_package.jar", jvmpath=Path(self.jvmpath))
        cl = jpype.JClass("java.lang.ClassLoader").getSystemClassLoader()
        self.assertEqual(type(cl), jpype.JClass("org.jpype.JPypeClassLoader"))
        assert dir(jpype.JPackage('org.jpype.sample_package')) == ['A', 'B']

    def testNonASCIIPathWithSystemClassLoader(self):
        with self.assertRaises(ValueError):
            jpype.startJVM(
                "-Djava.system.class.loader=jpype.startup.TestSystemClassLoader",
                jvmpath=Path(self.jvmpath),
                classpath="test/jar/unicode_à😎/sample_package.jar"
            )

    def testOldStyleNonASCIIPathWithSystemClassLoader(self):
        with self.assertRaises(ValueError):
            jpype.startJVM(
                self.jvmpath,
                "-Djava.system.class.loader=jpype.startup.TestSystemClassLoader",
                "-Djava.class.path=test/jar/unicode_à😎/sample_package.jar"
            )

    @common.requireAscii
    def testASCIIPathWithSystemClassLoader(self):
        jpype.startJVM(
            "-Djava.system.class.loader=jpype.startup.TestSystemClassLoader",
            jvmpath=Path(self.jvmpath),
            classpath=f"test/classes"
        )
        classloader = jpype.JClass("java.lang.ClassLoader").getSystemClassLoader()
        test_classLoader = jpype.JClass("jpype.startup.TestSystemClassLoader")
        self.assertEqual(type(classloader), test_classLoader)
        assert dir(jpype.JPackage('jpype.startup')) == ['TestSystemClassLoader']

    @common.requireAscii
    def testOldStyleASCIIPathWithSystemClassLoader(self):
        jpype.startJVM(
            self.jvmpath,
            "-Djava.system.class.loader=jpype.startup.TestSystemClassLoader",
            "-Djava.class.path=test/classes"
        )
        classloader = jpype.JClass("java.lang.ClassLoader").getSystemClassLoader()
        test_classLoader = jpype.JClass("jpype.startup.TestSystemClassLoader")
        self.assertEqual(type(classloader), test_classLoader)
        assert dir(jpype.JPackage('jpype.startup')) == ['TestSystemClassLoader']

    @common.requireAscii
    def testDefaultSystemClassLoader(self):
        # we introduce no behavior change unless absolutely necessary
        jpype.startJVM(jvmpath=Path(self.jvmpath))
        cl = jpype.JClass("java.lang.ClassLoader").getSystemClassLoader()
        self.assertNotEqual(type(cl), jpype.JClass("org.jpype.JPypeClassLoader"))

    def testServiceWithNonASCIIPath(self):
        jpype.startJVM(
            self.jvmpath,
            "-Djava.locale.providers=SPI,CLDR",
            classpath="test/jar/unicode_à😎/service.jar",
        )
        ZoneId = jpype.JClass("java.time.ZoneId")
        ZoneRulesException = jpype.JClass("java.time.zone.ZoneRulesException")
        try:
            ZoneId.of("JpypeTest/Timezone")
        except ZoneRulesException:
            self.fail("JpypeZoneRulesProvider not loaded")
