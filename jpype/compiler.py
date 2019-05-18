# *****************************************************************************
#   Copyright 2018 Karl Nelson
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

from . import _jclass
from . import _jexception

__all__ = ['JCompiler', 'JSyntaxError']

class JSyntaxError(Exception):
    def __init__(self, msg, code, diag):
        self._code = code
        self._diag = diag
        Exception.__init__(self, msg)

    diagnostics = property(lambda self: self._diag, None)
    code = property(lambda self: self._code, None)


class JCompiler(object):
    """ Front end for java compiler.

    Thic class is used to compiler short pieces of java code to create a
    dynamic class.  Compiling classes is slow and thus this should only 
    be used in cases in which the Java API requires extension of a class
    to implement behavior.

    Calling the compiler will produce a new class which is returned to 
    the user.  This class can be used like any native class subject to
    restrictions. As the java class does not belong to any jar it can
    not be deserialized.

    Restrictions:
        * The java compiler must be installed on the system.
        * Public classes must have a package name.  They may not be in 
        the default package.
        * Private classes must have a defined public constructor or they cannot
        be instantiated.  Only public classes have default constructors.
        * Classes are loaded using the JPype internal memory class loader
        and thus may not be able to import access classes defined in other 
        external classloaders.

    Todo:
        * Compiler options are not exposed.

    """

    def __init__(self, *args):
        # Use the custom class loader
        classLoader = _jclass.JClass("org.jpype.classloader.JPypeClassLoader")
        Class = _jclass.JClass('java.lang.Class')

        # Get a class from jpype class loader
        cls = Class.forName("org.jpype.compiler.MemoryCompiler",
                            False, classLoader.getInstance())

        # Convert it to a python class
        self._memoryCompiler = _jclass.JClass(cls)()

    def available(self):
        return self._memoryCompiler.isAvailable()

    def compile(self, className, code):
        """ Compile a class with current compiler options.

        Args:
            className(str): Class name to be compiled including the package name.  This should be 
               dot seperated.
            code(str): java code to be compiled typically represented with a multiline string using
               regular expression encoding.

        Returns:
            A java class wrapper for the newly created class or throws an exception on failure.

        """
        if not self._memoryCompiler.isAvailable():
            raise OSError("Compiler is not available")

        cls = self._memoryCompiler.compile(className, code)

        if cls == None:
            dc = self._memoryCompiler.getDiagnostics().getDiagnostics()
            raise JSyntaxError("Failed to compile.", code, dc)

        return _jclass.JClass(cls)
