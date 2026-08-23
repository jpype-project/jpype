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

# test dir jpype/test
root = Path(__file__).parent.parent
cp_p = (root / "classes").absolute()
test_jar_p = (root / "jar").absolute()
assert cp_p.exists()
assert test_jar_p.exists()
cp = str(cp_p)
test_jar = str(test_jar_p)


@subrun.TestCase(individual=True)
class StartJVMCase(unittest.TestCase):
    def setUp(self):
        self.jvmpath = jpype.getDefaultJVMPath()

    def testStartup(self):
        with self.assertRaises(OSError):
            jpype.startJVM(convertStrings=False)
            jpype.startJVM(convertStrings=False)

    def testRestart(self):
        """restarts are forbidden."""
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
        jpype.startJVM(jvmpath=Path(self.jvmpath), classpath=f"{test_jar}/unicode_à😎/sample_package.jar")
        cl = jpype.JClass("java.lang.ClassLoader").getSystemClassLoader()
        self.assertEqual(type(cl), jpype.JClass("org.jpype.JPypeClassLoader"))
        assert dir(jpype.JPackage('org.jpype.sample_package')) == ['A', 'B']


    def testPlusAndSpecialCharsPath(self):
        """Test that classpath directories containing "+", "&", "=", or "#"
        are handled correctly.
        Regression test for https://github.com/jpype-project/jpype/issues/1413
        """
        jpype.startJVM(jvmpath=Path(self.jvmpath),
                        classpath=f"{test_jar}/plus+path&has=special#chars/sample_package.jar")
        assert dir(jpype.JPackage('org.jpype.sample_package')) == ['A']

    def testOldStyleNonASCIIPath(self):
        """Test that paths with non-ASCII characters are handled correctly.
        Regression test for https://github.com/jpype-project/jpype/issues/1194
        """
        jpype.startJVM(f"-Djava.class.path={test_jar}/unicode_à😎/sample_package.jar", jvmpath=Path(self.jvmpath))
        cl = jpype.JClass("java.lang.ClassLoader").getSystemClassLoader()
        self.assertEqual(type(cl), jpype.JClass("org.jpype.JPypeClassLoader"))
        assert dir(jpype.JPackage('org.jpype.sample_package')) == ['A', 'B']

    def testNonASCIIPathWithSystemClassLoader(self):
        with self.assertRaises(ValueError):
            jpype.startJVM(
                "-Djava.system.class.loader=jpype.startup.TestSystemClassLoader",
                jvmpath=Path(self.jvmpath),
                classpath=f"{test_jar}/unicode_à😎/sample_package.jar"
            )

    def testOldStyleNonASCIIPathWithSystemClassLoader(self):
        with self.assertRaises(ValueError):
            jpype.startJVM(
                self.jvmpath,
                "-Djava.system.class.loader=jpype.startup.TestSystemClassLoader",
                f"-Djava.class.path={test_jar}/unicode_à😎/sample_package.jar"
            )

    @common.requireAscii
    def testASCIIPathWithSystemClassLoader(self):
        jpype.startJVM(
            "-Djava.system.class.loader=jpype.startup.TestSystemClassLoader",
            jvmpath=Path(self.jvmpath),
            classpath=cp
        )
        classloader = jpype.JClass("java.lang.ClassLoader").getSystemClassLoader()
        test_classLoader = jpype.JClass("jpype.startup.TestSystemClassLoader")
        self.assertEqual(type(classloader), test_classLoader)
        assert dir(jpype.JPackage('jpype.startup')) == ['TestSystemClassLoader']

    def testUnsupportedClassVersionMessage(self):
        """If org.jpype.jar can't be loaded because it was compiled for a
        newer Java than the running JVM supports, startJVM must surface
        the underlying UnsupportedClassVersionError rather than the
        generic, uninformative "Can't find org.jpype.jar support
        library" message. Regression test for
        https://github.com/jpype-project/jpype/issues/1312

        Does not touch the real, installed org.jpype.jar: that file is
        shared with every other test process (this test itself runs in
        its own subrun subprocess, but the real jar is still the one
        every *other* concurrently-running worker's JVM has open) -
        in-place mutation of it was fine on Linux but reliably failed on
        Windows with "Access is denied" replacing a file another
        process still has open. Instead, this builds a private one-class
        jar with the same corrupted org/jpype/JPypeClassLoader.class and
        passes it via classpath= - _core.py's startJVM() always appends
        the real support_lib to the *end* of the classpath
        (java_class_path.append(support_lib)), so a same-named class
        earlier in the classpath shadows it and the JVM resolves
        org.jpype.JPypeClassLoader from the corrupted private jar
        instead, without the real org.jpype.jar ever being touched.

        The shadow jar's own directory is cleaned up with
        ignore_errors=True, not tempfile.TemporaryDirectory()'s default
        strict cleanup: on Windows, the JVM keeps a file handle open on
        any classpath jar it mapped for classloading for the remaining
        lifetime of this process, even one whose load ultimately failed
        with UnsupportedClassVersionError - the file can't be deleted
        until this subrun subprocess exits, so a strict rmtree here
        always failed with "used by another process" on Windows CI.
        """
        import zipfile
        import tempfile
        import shutil

        support_lib = Path(jpype.__file__).resolve(
        ).parent.parent / "org.jpype.jar"
        entry = "org/jpype/JPypeClassLoader.class"
        with zipfile.ZipFile(support_lib, 'r') as zin:
            data = bytearray(zin.read(entry))
        # Class file bytes 4-8 (big endian) hold the major version.
        # Set it far beyond anything a real JVM will ever support so
        # loading it always fails with UnsupportedClassVersionError.
        data[6] = 0xFF
        data[7] = 0xFF
        tmp_dir = tempfile.mkdtemp()
        try:
            shadow_jar = os.path.join(tmp_dir, "shadow.jar")
            with zipfile.ZipFile(shadow_jar, 'w') as zout:
                zout.writestr(entry, bytes(data))

            with self.assertRaises(RuntimeError) as cm:
                jpype.startJVM(classpath=[shadow_jar], convertStrings=False)
            self.assertNotEqual(
                str(cm.exception), "Can't find org.jpype.jar support library")
        finally:
            shutil.rmtree(tmp_dir, ignore_errors=True)

    @common.requireAscii
    def testOldStyleASCIIPathWithSystemClassLoader(self):
        jpype.startJVM(
            self.jvmpath,
            "-Djava.system.class.loader=jpype.startup.TestSystemClassLoader",
            f"-Djava.class.path={cp}"
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
            classpath=f"{test_jar}/unicode_à😎/service.jar",
        )
        ZoneId = jpype.JClass("java.time.ZoneId")
        ZoneRulesException = jpype.JClass("java.time.zone.ZoneRulesException")
        try:
            ZoneId.of("JpypeTest/Timezone")
        except ZoneRulesException:
            self.fail("JpypeZoneRulesProvider not loaded")

    def testShutdown(self):
        jpype.startJVM(self.jvmpath, classpath=cp)
        # Install a coverage hook
        instance = jpype.JClass("org.jpype.JPypeContext").getInstance()
        jpype.JClass("jpype.common.OnShutdown").addCoverageHook(instance)

        # Shutdown
        jpype.shutdownJVM()

        # Check that shutdown does not raise
        jpype._core._JTerminate()
