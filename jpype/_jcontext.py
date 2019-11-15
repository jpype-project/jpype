# *****************************************************************************
#   Copyright 2004-2008 Steve Menard
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
import _jpype


class JContext(_jpype.PyJPContext):
    def __init__(self):
        self._started = False

    def _start(self):
        self._started = True

        self._java_lang_Object = JClass("java.lang.Object", jvm=self)
        self._java_lang_Class = JClass("java.lang.Class", jvm=self)

        self._java_ClassLoader = JClass(
            'java.lang.ClassLoader', jvm=self).getSystemClassLoader()
        if not self._java_ClassLoader:
            raise RuntimeError("Unable to load Java SystemClassLoader")

        global self._java_lang_RuntimeException
        self._java_lang_RuntimeException = JClass(
            "java.lang.RuntimeException", jvm=self)

        # Preload needed classes
        java_lang_Boolean = JClass("java.lang.Boolean", jvm=self)
        java_lang_Long = JClass("java.lang.Long", jvm=self)
        java_lang_Double = JClass("java.lang.Double", jvm=self)
        java_lang_String = JClass("java.lang.String", jvm=self)
        object_classes = {}
        object_classes[bool] = java_lang_Boolean
        object_classes[int] = java_lang_Long
        object_classes[_long] = java_lang_Long
        object_classes[float] = java_lang_Double
        object_classes[str] = java_lang_String
        object_classes[_unicode] = java_lang_String
        object_classes[type] = self._java_lang_Class
        object_classes[_jpype.PyJPClass] = self._java_lang_Class
        object_classes[object] = self._java_lang_Object
        self._object_classes = object_classes


_jpype._jvm = JContext()
