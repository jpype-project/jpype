#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# *****************************************************************************
import _jpype
from . import _jcustomizer

__all__ = ['JException']


@_jcustomizer.JImplementationFor("java.lang.Throwable", base=True)
class JException(_jpype._JException):
    """ Base class for all ``java.lang.Throwable`` objects.

    When called as an object ``JException`` will produce a new exception class.
    The arguments may either be a string or an existing Java throwable.
    This functionality is deprecated as exception classes can be created with
    ``JClass``.

    Use ``issubclass(cls, JException)`` to test if a class is derived
    from ``java.lang.Throwable.``

    Use ``isinstance(obj, JException)`` to test if an object is a
    ``java.lang.Throwable``.

    """
    def __new__(cls, *args, **kwargs):
        if cls == JException:
            import warnings
            if not hasattr(JException, '_warned'):
                warnings.warn("Using JException to construct an exception type is deprecated.",
                              category=DeprecationWarning, stacklevel=2)
                JException._warned = True
            return _JExceptionClassFactory(*args, **kwargs)
        self = _jpype._JException.__new__(cls, *args)
        _jpype._JException.__init__(self, *args)
        return self

    def __str__(self):
        return str(self.toString())

    # Included for compatibility with JPype 0.6.3
    def message(self):
        return str(self.getMessage())

    # Included for compatibility with JPype 0.6.3
    def stacktrace(self):
        """ Get a string listing the stack frame.

        Returns:
          A string with the classic Java ``printStackTrace`` result.
        """
        StringWriter = _jpype.JClass("java.io.StringWriter")
        PrintWriter = _jpype.JClass("java.io.PrintWriter")
        sw = StringWriter()
        pw = PrintWriter(sw)
        self.printStackTrace(pw)
        pw.flush()
        r = sw.toString()
        sw.close()
        return r

    # For compatiblity with python exceptions
    @property
    def args(self):
        return self._jargs()
#    args = property(lambda self: self._jargs(), None)
#    """ Test doc string on property"""

    def _jargs(self):
        cause = self.getCause()
        if cause is None:
            return (str(self.getMessage()),)
        return (str(self.getMessage()), cause,)


def _JExceptionClassFactory(tp):
    if isinstance(tp, str):
        return _jpype.JClass(tp)
    if isinstance(tp, _jpype.JClass):
        return tp
    raise TypeError(
        "JException requires a string or java throwable type, got %s." % tp)


# Hook up module resources
_jpype.JException = JException
