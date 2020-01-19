# Tests for module functionality including failures that cannot
# be triggered in normal operations
import _jpype
import unittest
import subrun

##############################################################################
# Test methods


def runStartup(path):
    _jpype.startup(path, tuple(), False, False)


def runStartupBadArgs(path):
    _jpype.startup(path)


def runNoMethodDoc(path):
    _jpype.startup(path, tuple(), False, False)
    cls = _jpype.PyJPClass("java.lang.String")
    methods = cls.getClassMethods()
    methods[0].__doc__  # RuntimeError


def runNoMethodAnnotation(path):
    _jpype.startup(path, tuple(), False, False)
    cls = _jpype.PyJPClass("java.lang.String")
    methods = cls.getClassMethods()
    methods[0].__annotations__  # RuntimeError


def runNoMethodCode(path):
    _jpype.startup(path, tuple(), False, False)
    cls = _jpype.PyJPClass("java.lang.String")
    methods = cls.getClassMethods()
    methods[0].__code__  # RuntimeError


def runValueEntry():
    # fails as no JVM is running yet
    _jpype.PyJPValue()


def runShutdown():
    import jpype
    jpype.startJVM(convertStrings=False)
    jpype.shutdownJVM()


##############################################################################

class TestModule(unittest.TestCase):
    def setUp(self):
        import jpype
        self.path = jpype.getDefaultJVMPath()

    def testStartup(self):
        with subrun.Client() as client:
            client.execute(runStartup, self.path)

    def testStartupBadArgs(self):
        with self.assertRaises(TypeError):
            with subrun.Client() as client:
                client.execute(runStartupBadArgs, self.path)

    def testNoMethodDoc(self):
        with self.assertRaises(RuntimeError):
            with subrun.Client() as client:
                client.execute(runNoMethodDoc, self.path)

    def testNoMethodAnnotation(self):
        with self.assertRaises(RuntimeError):
            with subrun.Client() as client:
                client.execute(runNoMethodAnnotation, self.path)

    def testNoMethodCode(self):
        with self.assertRaises(RuntimeError):
            with subrun.Client() as client:
                client.execute(runNoMethodCode, self.path)

    def testValueEntry(self):
        with self.assertRaises(RuntimeError):
            with subrun.Client() as client:
                client.execute(runValueEntry)

    def testShutdown(self):
        with subrun.Client() as client:
            client.execute(runShutdown)
