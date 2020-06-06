# Tests for module functionality including failures that cannot
# be triggered in normal operations
import _jpype
import unittest
import subrun
import common


@subrun.TestCase(individual=True)
class ModuleStartTestCase(unittest.TestCase):

    def testNoJObject(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.JObject
            jpype.startJVM()

    def testNoJInterface(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.JInterface
            jpype.startJVM()

    def testNoJArray(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.JArray
            jpype.startJVM()

    def testNoJException(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.JException
            jpype.startJVM()

    def testNoJClassPre(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype._JClassPre
            jpype.startJVM()

    def testNoJClassPost(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype._JClassPost
            jpype.startJVM()

    def testNoMethodDoc(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.getMethodDoc
            jpype.startJVM()

    def testNoMethodAnnotations(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.getMethodAnnotations
            jpype.startJVM()

    def testNoMethodCode(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.getMethodCode
            jpype.startJVM()

    def testShutdown(self):
        import jpype
        jpype.startJVM(convertStrings=False)
        jpype.shutdownJVM()


class ModuleTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testIsPackage(self):
        self.assertTrue(_jpype.isPackage("java"))
        self.assertFalse(_jpype.isPackage("jva"))
        with self.assertRaises(TypeError):
            _jpype.isPackage(object())

    def testGetClass(self):
        self.assertIsInstance(_jpype._getClass("java.lang.String"), _jpype._JClass)
        with self.assertRaises(TypeError):
            _jpype._getClass(object())

    def testHasClass(self):
        self.assertTrue(_jpype._hasClass("java.lang.String"))
        with self.assertRaises(TypeError):
            _jpype._hasClass(object())
