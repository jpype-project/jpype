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
from . import _jcustomizer

__all__ = ['JException']


@_jcustomizer.JImplementationFor("java.lang.Throwable", base=True)
class JException(_jpype._JException, internal=True):
    """ Base class for all ``java.lang.Throwable`` objects.

    Use ``issubclass(cls, JException)`` to test if a class is derived
    from ``java.lang.Throwable.``

    Use ``isinstance(obj, JException)`` to test if an object is a
    ``java.lang.Throwable``.

    """

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

    @property
    def args(self):
        return self._args


# Hook up module resources
_jpype.JException = JException
