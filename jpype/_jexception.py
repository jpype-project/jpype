#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#*****************************************************************************
import _jpype
from . import _jclass

_CLASSES = {}

def JException(t):
    return t.PYEXC

class JavaException(Exception):
    def __init__(self, *data):
        if isinstance(data[0], tuple) \
           and data[0][0] is _jclass._SPECIAL_CONSTRUCTOR_KEY:
            # this is wrapped:
            self.__javaobject__ = data[0][1]
            self.__javaclass__ = self.__javaobject__.__class__
        else:
            self.__javaclass__ = _jclass.JClass(self.__class__.JAVACLASSNAME)
            self.__javaobject__ = self.__javaclass__(*data)

        Exception.__init__(self, self.__javaobject__)

    def javaClass(self):
        return self.__javaclass__

    def message(self):
        return self.__javaobject__.getMessage()

    def stacktrace(self):
        StringWriter = _jclass.JClass("java.io.StringWriter")
        PrintWriter = _jclass.JClass("java.io.PrintWriter")
        sw = StringWriter()
        pw = PrintWriter(sw)

        self.__javaobject__.printStackTrace(pw)
        pw.flush()
        r = sw.toString()
        sw.close()
        return r

    def __str__(self):
        return self.__javaobject__.toString()

def _initialize():
    _jpype.setResource('JavaExceptionClass', JavaException)

def _makePythonException(name, bc):
    if name in _CLASSES:
        return _CLASSES[name]

    if name == 'java.lang.Throwable':
        bases = (JavaException,)
    else:
        bases = (_makePythonException(bc.getName(), bc.getBaseClass()) ,)

    ec = type(name+"PyRaisable", bases, {'JAVACLASSNAME': name})

    _CLASSES[name] = ec
    return ec
