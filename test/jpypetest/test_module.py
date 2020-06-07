# Tests for module functionality including failures that cannot
# be triggered in normal operations
import _jpype
import jpype
import unittest
import subrun
import common
import unittest.mock as mock


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
            del _jpype._jclassPre
            jpype.startJVM()

    def testNoJClassPost(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype._jclassPost
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


class JInitTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testJInit(self):
        with mock.patch("_jpype.isStarted") as started:
            started.return_value = False
            self.assertEqual(len(jpype._jinit.JInitializers), 0)
            A = []

            def func():
                A.append(1)
            jpype.registerJVMInitializer(func)
            self.assertEqual(len(A), 0)
            self.assertEqual(jpype._jinit.JInitializers[0], func)
            started.return_value = True
            jpype.registerJVMInitializer(func)
            self.assertEqual(len(A), 1)
            jpype._jinit.runJVMInitializers()
            self.assertEqual(len(A), 2)
