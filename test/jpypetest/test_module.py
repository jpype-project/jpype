# Tests for module functionality including failures that cannot
# be triggered in normal operations
import _jpype
import unittest
import subrun


@subrun.TestCase(individual=True)
class TestModule(unittest.TestCase):

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
