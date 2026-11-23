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
from . import _jcustomizer
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
        try:
            self.close()
        except _jexception.JException as jex:
            # Eat the second exception only if the with body itself failed;
            # being inside an unrelated except block does not count.
            if exception_type is None:
                raise jex
