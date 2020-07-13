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

__all__ = ['JPackage']


class _JPackageMeta(type):
    def __instancecheck__(self, other):
        return isinstance(other, _jpype._JPackage)

    def __subclasscheck__(self, other):
        return issubclass(other, _jpype._JPackage)


class JPackage(_jpype._JPackage, metaclass=_JPackageMeta):
    """ Gateway for automatic importation of Java classes.

    This class allows structured access to Java packages and classes.
    This functionality has been replaced by ``jpype.imports``, but is still
    useful in some cases.

    Only the root of the package tree needs to be declared with the ``JPackage``
    constructor. Sub-packages will be created on demand.

    For example, to import the w3c DOM package:

    .. code-block:: python

      Document = JPackage('org').w3c.dom.Document

    Under some situations such as a missing jar file, the resulting object
    will be a JPackage object rather than the expected java class. This
    results in rather challanging debugging messages. Due to this 
    restriction, the ``jpype.imports`` module is preferred. To prevent these
    types of errors, a package can be declares as ``strict`` which prevents
    expanding package names that do not comply with Java package name
    conventions.

    Args:
      path (str): Path into the Java class tree.

    Example:

      .. code-block:: python

        # Alias into a library
        google = JPackage("com").google

        # Access members in the library
        result = google.common.IntMath.pow(x,m)

    """
    pass
