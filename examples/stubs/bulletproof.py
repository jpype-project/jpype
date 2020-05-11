# This is an outdated test that hit each entry point in the
# _jpype module and tested if the module response properly to
# an attempt to call without the jvm running.  We can't
# run this in our testbench as it requires that the
# jvm is not running.
import unittest
import jpype
import _jpype


class TestJpypeModule(unittest.TestCase):
    def setUp(self):
        self.methods = {
            'attach': [tuple(["none"]), RuntimeError],
            'attachThreadAsDaemon': [tuple(), RuntimeError],
            'attachThreadToJVM': [tuple(), RuntimeError],
            'convertToDirectBuffer': [tuple(), RuntimeError],
            'convertToJValue': [tuple(["a", 1]), RuntimeError],
            'createProxy': [tuple(['a', (1, 2)]), AttributeError],
            'detachThreadFromJVM': [tuple(), RuntimeError],
            'dumpJVMStats': [tuple(), None],
            'findArrayClass': [tuple('double[]'), RuntimeError],
            'findClass': [tuple('java.lang.String'), RuntimeError],
            'getArrayItem': [tuple([None, 1]), TypeError],
            'getArrayLength': [tuple([None]), TypeError],
            'getArraySlice': [tuple([None, 1, 2]), TypeError],
            'isStarted': [tuple(), None],
            'isThreadAttachedToJVM': [tuple(), RuntimeError],
            'newArray': [tuple([None, 1]), TypeError],
            'setArrayItem': [tuple([None, 1, 2]), TypeError],
            'setArraySlice': [tuple([None, 1, 2, 3]), TypeError],
            'setConvertStringObjects': [tuple(), RuntimeError],
            'setGetClassMethod': [tuple([None]), None],
            'setGetJavaArrayClassMethod': [tuple([None]), None],
            'setJavaArrayClass': [tuple([None]), None],
            'setJavaExceptionClass': [tuple([None]), None],
            'setJavaLangObjectClass': [tuple([None]), None],
            'setProxyClass': [tuple([None]), None],
            'setSpecialConstructorKey': [tuple([None]), None],
            'setStringWrapperClass': [tuple([None]), None],
            'setWrapperClass': [tuple([None]), None],
            'shutdown': [tuple(), RuntimeError],
            'startReferenceQueue': [tuple([1]), RuntimeError],
            'startup': [tuple([None, tuple([None]), None]), TypeError],
            'stopReferenceQueue': [tuple(), RuntimeError],
            'synchronized': [tuple(), None],
        }

    def testEntryPoints(self):
        for n, c in self.methods.items():
            print('====', n)
            method = getattr(_jpype, n)
            args = c[0]
            expect = c[1]
            if expect == None:
                method(*args)
            else:
                self.assertRaises(expect, method, *args)


# This is a special test suite that checks to see that every entry point to the private
# module will safely fail rather than segfaulting.  It can't be run with other tests
# as the jvm must not be loaded.
#
# To test use
#   nosetests test.bulletproof
