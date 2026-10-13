import common
import jpype
import subrun


@subrun.TestCase(individual=True)
class JExceptionIsInstanceTestCaseBeforeJVM(common.JPypeTestCase):
    """Test isinstance with JException when JVM is not running"""

    def testIsInstanceBeforeJVMStart(self):
        """Test that isinstance(obj, JException) doesn't crash before JVM starts"""
        # This test runs before JVM is started
        # Get a regular Python exception
        try:
            raise ValueError("test")
        except ValueError as e:
            # This should not crash and should return False
            result = isinstance(e, jpype.JException)
            self.assertFalse(result)

class JExceptionIsInstanceTestCaseAfterJVM(common.JPypeTestCase):
    def testIsInstanceWithNonJavaObject(self):
        """Test isinstance with non-Java objects after JVM starts"""
        # After JVM starts, test with Python objects
        common.JPypeTestCase.setUp(self)
        
        # Test with Python exception
        try:
            raise ValueError("test")
        except ValueError as e:
            result = isinstance(e, jpype.JException)
            self.assertFalse(result)
        
        # Test with regular Python object
        obj = "not a java object"
        result = isinstance(obj, jpype.JException)
        self.assertFalse(result)

    def testIsInstanceWithJavaException(self):
        """Test isinstance with actual Java exception"""
        # Create a Java exception
        Exception = jpype.JClass("java.lang.Exception")
        java_ex = Exception("test")
        
        # This should return True
        assert isinstance(java_ex, jpype.JException)
