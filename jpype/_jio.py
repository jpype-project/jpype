# *****************************************************************************
#   Copyright 2017 Karl Einar Nelson
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
# *****************************************************************************
from . import _jcustomizer
import sys as _sys
from . import _jexception

# This contains a customizer for closeable so that we can use the python "with"
# statement.

if _sys.version_info > (3,):
    pass


@_jcustomizer.JImplementationFor("java.io.Closeable")
class _JCloseable(object):
    """ Customizer for ``java.io.Closable``

    This customizer adds support of the `with` operator to all Java 
    classes that implement Java Closable interface. 

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
            if (info[0] == None):
                raise jex
            pass
