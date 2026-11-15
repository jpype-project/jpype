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
import io
import jpype
import common


class StreamRedirectTestCase(common.JPypeTestCase):
    """ Exercises the ``toPython()`` customizers added to the ``java.io``
    stream hierarchy (plan/StreamRedirect.md): a Java Writer/Reader/
    OutputStream/InputStream duck-typed as a Python ``io.TextIOBase``
    stream, suitable for assignment to ``sys.stdout``/``sys.stderr``/
    ``sys.stdin``.
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.StringWriter = jpype.JClass("java.io.StringWriter")
        self.StringReader = jpype.JClass("java.io.StringReader")
        self.ByteArrayOutputStream = jpype.JClass("java.io.ByteArrayOutputStream")
        self.ByteArrayInputStream = jpype.JClass("java.io.ByteArrayInputStream")
        self.PrintStream = jpype.JClass("java.io.PrintStream")

    def testWriterToPythonIsTextIOBase(self):
        sw = self.StringWriter()
        py_out = sw.toPython()
        self.assertIsInstance(py_out, io.TextIOBase)
        self.assertTrue(py_out.writable())
        self.assertFalse(py_out.readable())

    def testWriterWrite(self):
        sw = self.StringWriter()
        py_out = sw.toPython()
        py_out.write("hello ")
        py_out.write("world\n")
        py_out.flush()
        self.assertEqual(str(sw.toString()), "hello world\n")

    def testReaderReadWithSize(self):
        sr = self.StringReader("abcdef")
        py_in = sr.toPython()
        self.assertTrue(py_in.readable())
        self.assertFalse(py_in.writable())
        self.assertEqual(py_in.read(3), "abc")
        self.assertEqual(py_in.read(), "def")
        self.assertEqual(py_in.read(), "")

    def testReaderReadline(self):
        sr = self.StringReader("line one\nline two\nline three")
        py_in = sr.toPython()
        self.assertEqual(py_in.readline(), "line one\n")
        self.assertEqual(py_in.readline(), "line two\n")
        self.assertEqual(py_in.readline(), "line three")
        self.assertEqual(py_in.readline(), "")

    def testReaderClose(self):
        sr = self.StringReader("abc")
        py_in = sr.toPython()
        py_in.close()
        self.assertTrue(py_in.closed)

    def testOutputStreamToPythonDefaultsToUtf8(self):
        baos = self.ByteArrayOutputStream()
        py_out = baos.toPython()
        py_out.write("café\n")
        py_out.flush()
        self.assertEqual(bytes(baos.toByteArray()), "café\n".encode("utf-8"))

    def testInputStreamToPython(self):
        data = "stdin line\nsecond\n".encode("utf-8")
        bais = self.ByteArrayInputStream(data)
        py_in = bais.toPython()
        self.assertEqual(py_in.readline(), "stdin line\n")
        self.assertEqual(py_in.readline(), "second\n")

    def testPrintStreamInheritsToPython(self):
        # PrintStream extends OutputStream - toPython() should be reachable
        # on it purely through the customizer's ordinary Python inheritance,
        # not a separate registration.
        baos = self.ByteArrayOutputStream()
        ps = self.PrintStream(baos)
        py_out = ps.toPython()
        py_out.write("via printstream\n")
        py_out.flush()
        self.assertEqual(bytes(baos.toByteArray()), b"via printstream\n")

    def testSysStdoutAssignmentRoundTrip(self):
        # The "Python-side trigger" from the plan: no jpype internals
        # required, just assign the wrapper to sys.stdout directly.
        import sys
        sw = self.StringWriter()
        original = sys.stdout
        try:
            sys.stdout = sw.toPython()
            print("captured", end="")
            sys.stdout.flush()
        finally:
            sys.stdout = original
        self.assertEqual(str(sw.toString()), "captured")
