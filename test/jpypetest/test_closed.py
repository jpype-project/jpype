import jpype
import common


class ClosedTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testObjects(self):
        from jpype import java

        s = java.lang.String('foo')
        s._allowed = 1
        try:
            s.forbidden = 1
        except AttributeError:
            pass
        else:
            raise AssertionError("AttributeError not raised")

    def testArrays(self):
        s = jpype.JArray(jpype.JInt)(5)
        # Setting private members is allowed
        s._allowed = 1
        try:
            # Setting public members is prohibited
            s.forbidden = 1
        except AttributeError:
            pass
        else:
            raise AssertionError("AttributeError not raised")

    def testStatic(self):
        static = jpype.JClass('jpype.objectwrapper.StaticTest')
        self.assertEqual(static.i, 1)
        self.assertEqual(static.d, 1.2345)
        self.assertEqual(static.s, "hello")
