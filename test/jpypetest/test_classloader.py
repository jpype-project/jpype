"""
Tests for class loader handling.

This test suite verifies that JPype correctly handles classes loaded through
different mechanisms and with explicit class loaders. It includes regression
tests for bug #992 where enum types implementing interfaces could not be passed
to methods when loaded by different class loaders.

The tests verify:
- Normal class loading through JPype works correctly
- Enums implementing interfaces can be used in method calls
- Explicit class loader specification works (JClass with loader parameter)
- Class.forName() with explicit loader works
- Multiple access patterns (direct, valueOf, JPackage) are consistent
"""
import jpype
import common


class ClassLoaderTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.RoleType = jpype.JClass('jpype.enumtest.RoleType')
        self.TestInterface = jpype.JClass('jpype.enumtest.TestInterface')
        self.Enumerator = jpype.JClass('jpype.enumtest.Enumerator')

    def testEnumImplementsInterface(self):
        """Test that enum implementing an interface is recognized"""
        role = self.RoleType.MASTER
        self.assertIsInstance(role, self.Enumerator)
        self.assertEqual(role.getName(), "Master")
        self.assertEqual(role.getValue(), 0)

    def testEnumDirectFieldAccess(self):
        """Test passing enum accessed via direct field access"""
        test_obj = self.TestInterface()
        role = self.RoleType.MASTER
        test_obj.setRole(role)
        self.assertEqual(test_obj.getRoleAsString(), "Master")

    def testEnumValueOf(self):
        """Test passing enum accessed via valueOf"""
        test_obj = self.TestInterface()
        role = self.RoleType.valueOf("SLAVE")
        test_obj.setRole(role)
        self.assertEqual(test_obj.getRoleAsString(), "Slave")

    def testEnumJPackageAccess(self):
        """Test passing enum accessed via JPackage"""
        test_obj = self.TestInterface()
        role = jpype.JPackage("jpype.enumtest").RoleType.MASTER
        test_obj.setRole(role)
        self.assertEqual(test_obj.getRoleAsString(), "Master")

    def testEnumMultipleAccesses(self):
        """Test that multiple accesses of the same enum value work correctly"""
        test_obj = self.TestInterface()
        # Access the same enum value multiple times
        for i in range(3):
            role = self.RoleType.MASTER
            test_obj.setRole(role)
            self.assertEqual(test_obj.getRoleAsString(), "Master")

    def testEnumFreshClassLoading(self):
        """Test that enum works even with fresh JClass calls"""
        test_obj = self.TestInterface()
        # Force fresh class lookups
        for i in range(3):
            RoleType_fresh = jpype.JClass('jpype.enumtest.RoleType')
            role = RoleType_fresh.SLAVE
            test_obj.setRole(role)
            self.assertEqual(test_obj.getRoleAsString(), "Slave")

    def testEnumClassIdentity(self):
        """Test that enum classes from different access paths are identical"""
        role1 = self.RoleType.MASTER
        role2 = self.RoleType.valueOf("MASTER")
        role3 = jpype.JPackage("jpype.enumtest").RoleType.MASTER

        # All should be equal
        self.assertEqual(role1, role2)
        self.assertEqual(role1, role3)

        # Their Java classes should be the same
        self.assertEqual(role1.getClass(), role2.getClass())
        self.assertEqual(role1.getClass(), role3.getClass())

    def testNestedEnumDirectAccess(self):
        """Test nested enum (static inner class) direct access"""
        Example = jpype.JClass('jpype.enumtest.Example')
        value1 = Example.ExampleEnum.VALUE1
        # Should not be None (as reported in bug)
        self.assertIsNotNone(value1)
        Example.takeEnum(value1)

    def testNestedEnumAfterValueOf(self):
        """Test that nested enum works correctly after valueOf is called"""
        Example = jpype.JClass('jpype.enumtest.Example')
        # Call valueOf first
        value2 = Example.ExampleEnum.valueOf("VALUE2")
        self.assertIsNotNone(value2)
        Example.takeEnum(value2)
        # Now try direct access
        value1 = Example.ExampleEnum.VALUE1
        self.assertIsNotNone(value1)
        Example.takeEnum(value1)

    def testEnumWithExplicitClassLoader(self):
        """Test loading enum through explicit class loader (solution for bug #992)"""
        # Get the system class loader
        ClassLoader = jpype.JClass('java.lang.ClassLoader')
        system_loader = ClassLoader.getSystemClassLoader()

        # Load the enum class with explicit loader
        RoleType_loaded = jpype.JClass('jpype.enumtest.RoleType', loader=system_loader)
        test_obj = self.TestInterface()

        # Should work fine
        role = RoleType_loaded.valueOf("MASTER")
        test_obj.setRole(role)
        self.assertEqual(test_obj.getRoleAsString(), "Master")

    def testEnumWithForName(self):
        """Test loading enum using Class.forName with loader (alternative solution)"""
        Class = jpype.JClass('java.lang.Class')
        ClassLoader = jpype.JClass('java.lang.ClassLoader')

        # Get the system class loader
        system_loader = ClassLoader.getSystemClassLoader()

        # Load class using forName with specific loader
        RoleTypeClass = Class.forName('jpype.enumtest.RoleType', True, system_loader)

        # Access enum constant through reflection
        role = RoleTypeClass.getField("SLAVE").get(None)
        self.assertIsNotNone(role)

        # Should work with methods expecting this enum type
        test_obj = self.TestInterface()
        test_obj.setRole(role)
        self.assertEqual(test_obj.getRoleAsString(), "Slave")
