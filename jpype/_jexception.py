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
import sys as _sys
from . import _jcustomizer
from . import _jclass
from . import _jobject

__all__ = ['JException']

if _sys.version > '3':
    _unicode = str
else:
    _unicode = unicode


from traceback import FrameSummary, StackSummary, TracebackException, _some_str

def stack_trace_to_StackSummary(stack_trace):
    res = []
    for el in stack_trace:
        ln = int(el.getLineNumber())
        fn = el.getFileName()
        if fn:
            fn = str(fn)
        else:
            fn = None
        res.append(FrameSummary(fn, ln, ".".join((str(el.getClassName()), str(el.getMethodName()))), lookup_line=False, locals=None, line=None))
    res = StackSummary.from_list(res)
    return res

class JPypeTracebackException(TracebackException):
    def __init__(self, exc_value):
        exc_type = type(exc_value)
        self.stack = stack_trace_to_StackSummary(exc_value.getStackTrace())
        self.exc_type = exc_type
        self._str = _some_str(exc_value)
        cause = exc_value.getCause()
        if cause:
            self.__cause__ = self.__class__(cause)
        else:
            self.__cause__ = None
        self.__context__ = None
        self.exc_traceback = True
    
    @classmethod
    def from_exception(cls, exc, *args, **kwargs):
        return cls(exc, *args, **kwargs)


class _JException(object):
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
        return super(JException, cls).__new__(cls)

    def __init__(self, *args, **kwargs):
        if hasattr(self, '__javavalue__'):
            pass
        elif len(args) == 1 and isinstance(args[0], _jpype.PyJPValue):
            self.__javavalue__ = args[0]
        else:
            self.__javavalue__ = self.__class__.__javaclass__.newInstance(
                *args)
        super(Exception, self.__class__).__init__(self)

    def __str__(self):
        return str(self.toString())

    # Included for compatibility with JPype 0.6.3
    def message(self):
        return str(self.getMessage())

    # Included for compatibility with JPype 0.6.3
    def stacktrace(self):
        """ Get a string listing the stack frame.

        Returns:
          A string from lines of ``traceback.TracebackException.format()`` result.
        """
        return "".join(l for l in JPypeTracebackException(self).format())

    def java_stacktrace(self):
        """ Get a string listing the stack frame.

        Returns:
          A string with the classic Java ``printStackTrace`` result.
        """
        StringWriter = _jclass.JClass("java.io.StringWriter")
        PrintWriter = _jclass.JClass("java.io.PrintWriter")
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


JException = _jobject.defineJObjectFactory("JException", "java.lang.Throwable",
                                           _JException, bases=(Exception, _jobject.JObject))
_jcustomizer.registerClassBase('java.lang.Throwable', JException)


def _JExceptionClassFactory(tp):
    if isinstance(tp, (str, _unicode)):
        return _jclass.JClass(tp)
    if isinstance(tp, _jclass.JClass):
        return _jclass.JClass(tp.__javaclass__)
    raise TypeError(
        "JException requires a string or java throwable type, got %s." % tp)
