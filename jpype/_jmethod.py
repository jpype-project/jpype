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
__all__ = []
import _jpype
from . import _jclass


def _jmethodGetDoc(method, cls, overloads):
    """Generator for _JMethod.__doc__ property

    Parameters:
      method (_JMethod): method to generate doc string for.
      cls (java.lang.Class): Class holding this method dispatch.
      overloads (java.lang.reflect.Method[]): tuple holding all the methods
        that are served by this method dispatch.

    Returns:
      The doc string for the method dispatch.
    """
    jcls = _jpype.JClass(cls)
    if not hasattr(jcls, "__javadoc__"):
        jcls.__doc__
    jd = getattr(jcls, "__javadoc__")
    if jd is not None:
        md = jd.methods.get(method.__name__)
        if md is not None:
            return str(md)
    from textwrap import TextWrapper
    out = []
    out.append("Java method dispatch '%s' for '%s'" %
               (method.__name__, cls.getName()))
    out.append("")
    exceptions = []
    returns = []
    methods = []
    classmethods = []
    for ov in overloads:
        modifiers = ov.getModifiers()
        exceptions.extend(ov.getExceptionTypes())
        returnName = ov.getReturnType().getCanonicalName()
        params = ", ".join([str(i.getCanonicalName())
                            for i in ov.getParameterTypes()])
        if returnName != "void":
            returns.append(returnName)
        if modifiers & 8:
            classmethods.append("    * %s %s(%s)" %
                                (returnName, ov.getName(), params))
        else:
            methods.append("    * %s %s(%s)" %
                           (returnName, ov.getName(), params))
    if classmethods:
        out.append("  Static Methods:")
        out.extend(classmethods)
        out.append("")

    if methods:
        out.append("  Virtual Methods:")
        out.extend(methods)
        out.append("")

    if exceptions:
        out.append("  Raises:")
        for exc in set(exceptions):
            out.append("    %s: from java" % exc.getCanonicalName())
        out.append("")

    if returns:
        out.append("  Returns:")
        words = ", ".join([str(i) for i in set(returns)])
        wrapper = TextWrapper(initial_indent='    ',
                              subsequent_indent='    ')
        out.extend(wrapper.wrap(words))
        out.append("")

    return "\n".join(out)


def _jmethodGetAnnotation(method, cls, overloads):
    """Generator for ``_JMethod.__annotation__`` property

    Parameters:
      method (_JMethod): method to generate annotations for.
      cls (java.lang.Class): Class holding this method dispatch.
      overloads (java.lang.reflect.Method[]): tuple holding all the methods
        that are served by this method dispatch.

    Returns:
      The dict to use for type annotations.
    """
    returns = []

    # Special handling if we have 1 overload
    if len(overloads) == 1:
        ov = overloads[0]
        out = {}
        for i, p in enumerate(ov.getParameterTypes()):
            out['arg%d' % i] = _jclass.JClass(p)
        out['return'] = _jclass.JClass(ov.getReturnType())
        return out

    # Otherwise, we only get the return
    for ov in overloads:
        returns.append(ov.getReturnType())

    returns = set(returns)
    if len(returns) == 1:
        return {"return": _jclass.JClass([i for i in returns][0])}
    return {}


def _jmethodGetCode(method):
    def call(*args):
        return method.__call__(*args)
    return call


_jpype.getMethodDoc = _jmethodGetDoc
_jpype.getMethodAnnotations = _jmethodGetAnnotation
_jpype.getMethodCode = _jmethodGetCode
