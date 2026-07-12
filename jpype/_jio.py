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
from . import _jcustomizer
import sys as _sys
from . import _jexception

# This contains a customizer for closeable so that we can use the python "with"
# statement.


@_jcustomizer.JImplementationFor("java.lang.AutoCloseable")
class _JCloseable(object):
    """ Customizer for ``java.lang.AutoCloseable`` and ``java.io.Closeable``

    This customizer adds support of the ``with`` operator to all Java
    classes that implement the Java ``AutoCloseable`` interface.

    Example:

    .. code-block:: python

        from java.nio.files import Files, Paths
        with Files.newInputStream(Paths.get("foo")) as fd:
          # operate on the input stream

        # Input stream closes at the end of the block.

    """

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        info = _sys.exc_info()
        try:
            self.close()
        except _jexception.JException as jex:
            # Eat the second exception if we are already handling one.
            if (info[0] is None):
                raise jex


# This contains a customizer for the java.io stream hierarchy so that a
# Java Writer/Reader/OutputStream/InputStream can be handed directly to
# Python's own stdio hooks (``sys.stdout = some_writer.toPython()``).

_JCharArray = None
_JString = None


def _char_array(size):
    global _JCharArray
    if _JCharArray is None:
        _JCharArray = jpype.JArray(jpype.JChar)
    return _JCharArray(size)


def _chars_to_str(buf, n):
    global _JString
    if _JString is None:
        _JString = jpype.JClass("java.lang.String")
    return str(_JString(buf, 0, n))


class _JavaTextIO(io.TextIOBase):
    """ Wraps a ``java.io.Writer`` or ``java.io.Reader`` as a Python text
    stream, suitable for assignment to ``sys.stdout``/``sys.stderr``/
    ``sys.stdin``.
    """

    _CHUNK = 4096

    def __init__(self, stream, encoding=None, errors="strict"):
        self._stream = stream
        self._encoding = encoding
        self._errors = errors

    @property
    def encoding(self):
        return self._encoding

    @property
    def errors(self):
        return self._errors

    def writable(self):
        return hasattr(self._stream, "write")

    def readable(self):
        return hasattr(self._stream, "read")

    def write(self, s):
        self._stream.write(s)
        return len(s)

    def _read_chunk(self, size):
        buf = _char_array(size)
        n = self._stream.read(buf, 0, size)
        if n < 0:
            return None
        return _chars_to_str(buf, n)

    def read(self, size=-1):
        if size is None or size < 0:
            parts = []
            while True:
                chunk = self._read_chunk(self._CHUNK)
                if chunk is None:
                    break
                parts.append(chunk)
            return "".join(parts)
        if size == 0:
            return ""
        chunk = self._read_chunk(size)
        return chunk if chunk is not None else ""

    def readline(self, size=-1):
        parts = []
        total = 0
        while True:
            ch = self._read_chunk(1)
            if ch is None:
                break
            parts.append(ch)
            total += 1
            if ch == "\n":
                break
            if size is not None and size >= 0 and total >= size:
                break
        return "".join(parts)

    def flush(self):
        if hasattr(self._stream, "flush"):
            self._stream.flush()

    def isatty(self):
        return False

    def close(self):
        self._stream.close()
        super().close()


@_jcustomizer.JImplementationFor("java.io.Writer")
class _JWriter(object):
    def toPython(self, encoding=None, errors="strict"):
        return _JavaTextIO(self, encoding, errors)


@_jcustomizer.JImplementationFor("java.io.Reader")
class _JReader(object):
    def toPython(self, encoding=None, errors="strict"):
        return _JavaTextIO(self, encoding, errors)


_JOutputStreamWriter = None
_JInputStreamReader = None


@_jcustomizer.JImplementationFor("java.io.OutputStream")
class _JOutputStream(object):
    def toPython(self, encoding="utf-8", errors="strict"):
        global _JOutputStreamWriter
        if _JOutputStreamWriter is None:
            _JOutputStreamWriter = jpype.JClass("java.io.OutputStreamWriter")
        return _JOutputStreamWriter(self, encoding).toPython(encoding, errors)


@_jcustomizer.JImplementationFor("java.io.InputStream")
class _JInputStream(object):
    def toPython(self, encoding="utf-8", errors="strict"):
        global _JInputStreamReader
        if _JInputStreamReader is None:
            _JInputStreamReader = jpype.JClass("java.io.InputStreamReader")
        return _JInputStreamReader(self, encoding).toPython(encoding, errors)
