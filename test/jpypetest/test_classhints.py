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
import jpype
import common
import sys


class MyImpl(object):
    def blah(self):
        pass


class ClassProxy:
    def __init__(self, proxy):
        self.proxy = proxy


class ArrayProxy:
    def __init__(self, proxy):
        self.proxy = proxy


class StringProxy:
    def __init__(self, proxy):
        self.proxy = proxy


class ClassHintsTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Custom = jpype.JClass("jpype.classhints.Custom")
        self.ClassHintsTest = jpype.JClass("jpype.classhints.ClassHintsTest")

        @jpype.JImplements("jpype.classhints.Custom")
        class MyCustom(object):
            def __init__(self, arg):
                self.arg = arg
        self.MyCustom = MyCustom

    def testCharSequence(self):
        Instant = jpype.JClass("java.time.Instant")
        s = "2019-12-21T05:26:13.223189Z"
        self.assertTrue(str(Instant.parse(s)), s)

    def testInstant(self):
        import datetime
        if sys.version_info.major == 3 and sys.version_info.minor < 12:
            now = datetime.datetime.utcnow()
        else:
            now = datetime.datetime.now(datetime.UTC)
        Instant = jpype.JClass("java.time.Instant")
        self.assertIsInstance(jpype.JObject(now, Instant), Instant)

    def testPath(self):
        import pathlib
        JPath = jpype.JClass("java.nio.file.Path")
        self.assertIsInstance(jpype.JObject(
            pathlib.Path(__file__).absolute(), JPath), JPath)

    def testFile(self):
        import pathlib
        JFile = jpype.JClass("java.io.File")
        self.assertIsInstance(jpype.JObject(
            pathlib.Path(__file__).absolute(), JFile), JFile)

    def testConvertExact(self):
        cht = self.ClassHintsTest
        with self.assertRaises(TypeError):
            cht.call("hello")

        @jpype.JConversion(self.Custom, exact=str)
        def StrToCustom(jcls, args):
            return self.MyCustom(args)

        cht.call("hello")
        self.assertIsInstance(cht.input, self.MyCustom)
        self.assertEqual(cht.input.arg, "hello")

    def testConvertAttribute(self):
        cht = self.ClassHintsTest
        with self.assertRaises(TypeError):
            cht.call(MyImpl())

        @jpype.JConversion(self.Custom, attribute="blah")
        def StrToCustom(jcls, args):
            return self.MyCustom(args)

        cht.call(MyImpl())
        self.assertIsInstance(cht.input, self.MyCustom)
        self.assertIsInstance(cht.input.arg, MyImpl)

    def testClassCustomizer(self):

        @jpype.JConversion("java.lang.Class", instanceof=ClassProxy)
        def ClassCustomizer(jcls, obj):
            return obj.proxy

        hints = jpype.JClass('java.lang.Class')._hints
        self.assertTrue(ClassProxy in hints.implicit)

    def testArrayCustomizer(self):

        @jpype.JConversion(jpype.JInt[:], instanceof=ArrayProxy)
        def ArrayCustomizer(jcls, obj):
            return obj.proxy

        hints = jpype.JClass(jpype.JInt[:])._hints
        self.assertTrue(ArrayProxy in hints.implicit)

    def testStringCustomizer(self):

        @jpype.JConversion("java.lang.String", instanceof=StringProxy)
        def STringCustomizer(jcls, obj):
            return obj.proxy

        hints = jpype.JClass("java.lang.String")._hints
        self.assertTrue(StringProxy in hints.implicit)
