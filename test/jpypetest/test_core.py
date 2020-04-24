import mock
import jpype
import common
from jpype.types import *


class JCharTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    @mock.patch('jpype._core.sys')
    def testVersion(self, mock_sys):
        mock_sys.version_info = (2, 7)
        with self.assertRaises(ImportError):
            jpype._core.versionTest()
        mock_sys.version_info = (3, 8)
        jpype._core.versionTest()

    def testShutdownHook(self):
        Thread = JClass("java.lang.Thread")
        Runnable = JClass("java.lang.Runnable")
        Runtime = JClass("java.lang.Runtime")
        @jpype.JImplements(Runnable)
        class Run:
            @jpype.JOverride
            def run(self):
                pass
        th = Thread(Run())
        Runtime.getRuntime().addShutdownHook(th)
        self.assertTrue(Runtime.getRuntime().removeShutdownHook(th))

    def testShutdownWrongThread(self):
        Thread = JClass("java.lang.Thread")
        Runnable = JClass("java.lang.Runnable")
        @jpype.JImplements(Runnable)
        class Run:
            def __init__(self):
                self.rc = False
            @jpype.JOverride
            def run(self):
                try:
                    jpype.shutdownJVM()
                except:
                    self.rc = True
        run = Run()
        th = Thread(run)
        th.start()
        th.join()
        self.assertTrue(run.rc)


