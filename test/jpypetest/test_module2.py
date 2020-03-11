import jpype
import _jpype
import common


class ModuleTestCase2(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testMonitorOnNull(self):
        value = jpype.JObject(None)
        with self.assertRaises(TypeError):
            _jpype._JMonitor(value)

    def testMonitorOnString(self):
        value = jpype.JString("foo")
        with self.assertRaises(TypeError):
            _jpype._JMonitor(value)

    def testMonitorOnPrim(self):
        value = jpype.JInt(1)
        with self.assertRaises(TypeError):
            _jpype._JMonitor(value)

    def testMonitorInitBad(self):
        with self.assertRaises(TypeError):
            _jpype._JMonitor()

    def testMonitorInitBad2(self):
        with self.assertRaises(TypeError):
            _jpype._JMonitor(None)

    def testMonitorStr(self):
        obj = jpype.java.lang.Object()
        monitor = _jpype._JMonitor(obj)
        self.assertIsInstance(str(monitor), str)

    def testProxyInitBad(self):
        with self.assertRaises(TypeError):
            _jpype._JProxy(None)

    def testProxyInitBad2(self):
        with self.assertRaises(TypeError):
            _jpype._JProxy(None, None, None)

    def testProxyInitBad3(self):
        with self.assertRaises(TypeError):
            _jpype._JProxy(None, None, tuple([None, None]))

    def testProxyNoInterfaces(self):
        with self.assertRaises(TypeError):
            proxy = _jpype._JProxy(None, None, tuple())

    def testValueStr(self):
        obj = jpype.JClass("java.lang.Object")()
        self.assertIsInstance(str(obj), str)
