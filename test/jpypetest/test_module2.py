import jpype
import _jpype
import common


class ModuleTestCase2(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testMonitorOnNull(self):
        value = jpype.JObject(None)
        with self.assertRaises(TypeError):
            _jpype.PyJPMonitor(value.__javavalue__)
    def testMonitorOnString(self):
        value = jpype.JString("foo")
        with self.assertRaises(TypeError):
            _jpype.PyJPMonitor(value.__javavalue__)
    def testMonitorOnPrim(self):
        value = jpype.JInt(1)
        with self.assertRaises(TypeError):
            _jpype.PyJPMonitor(value.__javavalue__)
    def testMonitorInitBad(self):
        with self.assertRaises(TypeError):
            _jpype.PyJPMonitor()
    def testMonitorInitBad2(self):
        with self.assertRaises(TypeError):
            _jpype.PyJPMonitor(None)
    def testMonitorStr(self):
        obj = jpype.java.lang.Object()
        monitor = _jpype.PyJPMonitor(obj.__javavalue__)
        self.assertIsInstance(str(monitor), str)
    def testProxyInitBad(self):
        with self.assertRaises(TypeError):
            _jpype.PyJPProxy(None)
    def testProxyInitBad2(self):
        with self.assertRaises(TypeError):
            _jpype.PyJPProxy(None, None, None)
    def testProxyInitBad3(self):
        with self.assertRaises(TypeError):
            _jpype.PyJPProxy(None, None, tuple([None, None]))
    def testProxyStr(self):
        proxy = _jpype.PyJPProxy(None, None, tuple())
        self.assertIsInstance(str(proxy), str)
    def testValueInitBad(self):
        with self.assertRaises(TypeError):
            _jpype.PyJPValue("no")
    def testValueInitBad2(self):
        with self.assertRaises(TypeError):
            _jpype.PyJPValue("no","no")
    def testValueStr(self):
        obj = jpype.JClass("java.lang.Object")()
        self.assertIsInstance(str(obj.__javavalue__), str)
    def testModuleDump(self):
        _jpype.dumpJVMStats()

