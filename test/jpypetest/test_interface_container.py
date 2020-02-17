from jpype import JPackage, JArray, JByte, java, JClass, JProxy
import common


class DestructionTracker:
    del_calls = 0

    def __del__(self):
        DestructionTracker.del_calls += 1

    def callback(self, message):
        pass


class InterfaceContainerTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testNoInterfaceLeak(self):
        DestructionTracker.del_calls = 0

        interface = JProxy(
            "jpype.interface_container.TestInterface",
            dict={'callback': DestructionTracker().callback})

        del interface
        self.assertEqual(DestructionTracker.del_calls, 1)

    def testContainerLeak(self):
        DestructionTracker.del_calls = 0

        interface = JProxy(
            "jpype.interface_container.TestInterface",
            dict={'callback': DestructionTracker().callback})
        interface_container = JClass(
            "jpype.interface_container.TestInterfaceContainer")(interface)

        del interface, interface_container
        self.assertEqual(DestructionTracker.del_calls, 1)
