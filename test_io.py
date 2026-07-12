"""
End-to-end smoke test for the python.io SPI's class-level eager
registration, run from a plain launched Python script
(jpype.startJVM()) rather than the Java-hosted NGTest harness used by
`mvn test`. This exercises a genuinely different code path: the JVM is
started *from* Python, and the SPI's eager Installer/.pyspi
registration has to have already run by the time this script does
anything, since nothing here drives it manually.

python.io.IO.using(context) (the mini-backend, used to *construct*
python.io objects) is deliberately not exercised here: it's a Java
entry point meant to be called from Java, and is already covered by
the NGTest suite (PyBytesIONGTest, PyStringIONGTest,
PyIOStreamAdapterNGTest). Calling it from this side of the bridge hits
an unrelated quirk - jpype's forward marshalling detects that the
returned Java proxy is backed by this same interpreter and unwraps it
back to the raw reverse-bridge stub instead of a normal Java-object
view, so its declared methods aren't reachable as Python attributes.
That's orthogonal to the SPI itself.

What *is* meant to work from this direction - and what actually
matters for "does the eager SPI registration work from a launched
script" - is a plain stdlib io.BytesIO being recognized as a
python.io.PyBytesIO the moment it crosses into Java: that's the
_concrete[io.BytesIO] -> python.io.PyBytesIO mapping the .pyspi
resources set up, exercised below via the java.io.InputStream/
OutputStream JConversion (jpype/protocol.py), which is built on top of
PyBufferedIOBase.asInputStream()/asOutputStream().

See plan/SPI.md and plan/IO.md.
"""
import io

import jpype
import jpype.imports

jpype.startJVM()

from java.io import (  # noqa: E402
    BufferedReader,
    ByteArrayOutputStream,
    InputStream,
    InputStreamReader,
    OutputStreamWriter,
)


def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


# --- InputStream/OutputStream JConversion, driven through Reader/Writer ---

src = io.BytesIO(b"hello world\n")
reader = BufferedReader(InputStreamReader(src, "UTF-8"))
line = reader.readLine()
check(line == "hello world", f"unexpected line read through InputStream: {line!r}")

dst = io.BytesIO()
writer = OutputStreamWriter(dst, "UTF-8")
writer.write("round trip")
writer.flush()
check(dst.getvalue() == b"round trip", f"unexpected bytes written through OutputStream: {dst.getvalue()!r}")

print("OK: java.io.InputStream/OutputStream JConversion works from a launched script")

# --- Same conversion, driven raw (no Reader/Writer, byte-exact) ---

src2 = io.BytesIO(b"round trip through java.io")
out = ByteArrayOutputStream()
buf = jpype.JArray(jpype.JByte)(4096)
raw_in = jpype.JObject(src2, InputStream)
n = raw_in.read(buf)
out.write(buf, 0, n)
check(bytes(out.toByteArray()) == b"round trip through java.io",
      f"unexpected round trip result: {bytes(out.toByteArray())!r}")

print("OK: raw JObject(pyBytesIO, java.io.InputStream) conversion works")

jpype.shutdownJVM()
print("test_io.py: all checks passed")
