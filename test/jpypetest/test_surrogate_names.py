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
"""Unpaired UTF-16 surrogates in Java names (legal in class files and in
StackTraceElement) must not make a class unloadable or drop traceback frames.
"""
import pathlib
import shutil
import subprocess
import tempfile
import traceback
import unittest

import jpype
from jpype.types import *
import common


def _jstr(s):
    # Built from a char[] so the lone surrogate survives the trip into Java.
    return JClass("java.lang.String")(JArray(JChar)(list(s)))


class SurrogateNamesTestCase(common.JPypeTestCase):

    def _throwWithFrames(self, names):
        ex = JClass("java.lang.RuntimeException")("boom")
        STE = JClass("java.lang.StackTraceElement")
        ex.setStackTrace(JArray(STE)(
            [STE("Outer", n, "Outer.java", i + 1) for i, n in enumerate(names)]))
        return ex

    def _frames(self, names):
        Supplier = JClass("java.util.function.Supplier")
        Optional = JClass("java.util.Optional")
        with self.assertRaises(JClass("java.lang.RuntimeException")) as cm:
            Optional.empty().orElseThrow(
                Supplier @ (lambda: self._throwWithFrames(names)))
        tb = cm.exception.__cause__.__traceback__
        return [f.name for f in traceback.extract_tb(tb)]

    def testFramesBaseline(self):
        self.assertEqual(self._frames(["a", "b", "c", "d"]),
                         ["Outer.d", "Outer.c", "Outer.b", "Outer.a"])

    def testBadFrameKeepsOthers(self):
        bad = r"\ud83dX"
        self.assertEqual(self._frames([_jstr("a\ud83dX"), "b", "c", "d"]),
                         ["Outer.d", "Outer.c", "Outer.b", "Outer.a" + bad])
        self.assertEqual(self._frames(["a", "b", _jstr("c\ud83dX"), "d"]),
                         ["Outer.d", "Outer.c" + bad, "Outer.b", "Outer.a"])
        self.assertEqual(self._frames(["a", "b", "c", _jstr("d\ud83dX")]),
                         ["Outer.d" + bad, "Outer.c", "Outer.b", "Outer.a"])

    def testExceptionMessageWithSurrogate(self):
        # Converting this exception used to abort the process with
        # "Fatal error in exception handling".
        NFE = JClass("java.lang.NumberFormatException")
        with self.assertRaises(NFE) as cm:
            JClass("java.lang.Integer").parseInt(_jstr("\ud83dX"))
        self.assertIn("\ud83dX", str(cm.exception))
        self.assertIn("\ud83dX", str(cm.exception.getMessage()))
        # Formatting the traceback must not fail on the message either.
        text = "".join(traceback.format_exception(
            type(cm.exception), cm.exception, cm.exception.__traceback__))
        self.assertIn("NumberFormatException", text)

    @unittest.skipUnless(shutil.which("javac"), "javac not available")
    def testClassWithSurrogateMethodName(self):
        tmp = pathlib.Path(tempfile.mkdtemp())
        try:
            src = tmp / "SurrogateName.java"
            src.write_text(
                "public class SurrogateName {\n"
                "  public static int m中() { return 7; }\n"
                "  public static int other() { return 3; }\n"
                "}\n", encoding="utf-8")
            subprocess.run(["javac", "-encoding", "UTF-8", "-d", str(tmp), str(src)],
                           check=True)
            # Same length in modified UTF-8, so the class file stays valid.
            cls = tmp / "SurrogateName.class"
            data = cls.read_bytes()
            self.assertIn("中".encode(), data)
            cls.write_bytes(data.replace("中".encode(), b"\xed\xa0\xbd"))

            url = JClass("java.io.File")(str(tmp)).toURI().toURL()
            loader = JClass("java.net.URLClassLoader")(JArray(JClass("java.net.URL"))([url]))
            C = JClass("SurrogateName", loader=loader)
            # One bad member does not hide the rest of the class.
            self.assertEqual(C.other(), 3)
            self.assertEqual(getattr(C, "m\ud83d")(), 7)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
