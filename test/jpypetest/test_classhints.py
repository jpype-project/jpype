import jpype
import common
import sys

class MyImpl(object):
    def blah(self):
        pass

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
        now = datetime.datetime.utcnow()
        Instant = jpype.JClass("java.time.Instant")
        self.assertIsInstance(jpype.JObject(now, Instant), Instant)

    def testPath(self):
        import pathlib
        if not hasattr(pathlib.Path, '__fspath__'):
            raise common.unittest.SkipTest("not tested on cygwin")
        JPath = jpype.JClass("java.nio.file.Path")
        self.assertIsInstance(jpype.JObject(pathlib.Path(__file__).absolute(), JPath), JPath)

    def testFile(self):
        import pathlib
        if not hasattr(pathlib.Path, '__fspath__'):
            raise common.unittest.SkipTest("not tested on cygwin")
        JFile = jpype.JClass("java.io.File")
        self.assertIsInstance(jpype.JObject(pathlib.Path(__file__).absolute(), JFile), JFile)

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

