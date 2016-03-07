# part of JPype1; author Martin K. Scherer; 2014
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import mock

from jpype._jvmfinder import *
from jpype._linux import *
from jpype._darwin import *

import sys

class JVMFinderTest(unittest.TestCase):
    """
    test some methods to obtain a jvm.
    TODO: add windows (need to mock registry)
    """

    def test_find_libjvm(self):
        """
        test JVMFinder.find_libjvm does not return broken jvm implementation.
        """
        walk_fake = [('jre', ('lib',), ()),
                     ('jre/lib', ('amd64',), ()),
                     ('jre/lib/amd64',
                      ('cacao', 'jamvm', 'server'), ()),
                     ('jre/lib/amd64/cacao',
                      ('',), ('libjvm.so',)),
                     ('jre/lib/amd64/jamvm',
                      ('',), ('libjvm.so',)),
                     ('jre/lib/amd64/server',
                      ('',), ('libjvm.so',)),
                     ]
        with mock.patch('os.walk') as mockwalk:
            # contains broken and working jvms
            mockwalk.return_value = walk_fake

            finder = LinuxJVMFinder()
            p = finder.find_libjvm('arbitrary java home')
            self.assertEqual(
                p, os.path.join('jre/lib/amd64/server','libjvm.so'), 'wrong jvm returned')

        with mock.patch('os.walk') as mockwalk:
            # contains only broken jvms, since server impl is removed
            walk_fake[-1] = ((), (), (),)
            mockwalk.return_value = walk_fake

            finder = LinuxJVMFinder()
            with self.assertRaises(JVMNotSupportedException) as context:
                finder.find_libjvm('arbitrary java home')

    @mock.patch('os.walk')
    @mock.patch('os.path.exists')
    @mock.patch('os.path.realpath')
    def test_get_from_bin(self, mock_real_path, mock_path_exists, mock_os_walk):
        """
        test _get_from_bin method (used in linux and darwin)
        '/usr/bin/java' => some jre/jdk path
        """
        java_path = '/usr/lib/jvm/java-6-openjdk-amd64/bin/java'

        mock_os_walk.return_value = [
            ('/usr/lib/jvm/java-6-openjdk-amd64/jre/lib/amd64/server', ('',), ('libjvm.so',))]
        mock_path_exists.return_value = True
        mock_real_path.return_value = '/usr/lib/jvm/java-6-openjdk-amd64/bin/java'

        finder = LinuxJVMFinder()
        p = finder._get_from_bin()

        self.assertEqual(
            p, os.path.join('/usr/lib/jvm/java-6-openjdk-amd64/jre/lib/amd64/server','libjvm.so'))

    @unittest.skipIf(sys.version_info[:2] == (2, 6), "skip on py26")
    @mock.patch('platform.mac_ver')
    def test_javahome_binary_py27(self, mock_mac_ver):
        # this version has java_home binary
        mock_mac_ver.return_value = ('10.6.8', '', '')

        expected = '/System/Library/Java/JavaVirtualMachines/1.6.0.jdk/Contents/Home\n'

        finder = DarwinJVMFinder()

        # fake check_output
        with mock.patch('subprocess.check_output') as mock_checkoutput:
            mock_checkoutput.return_value = expected
            p = finder._javahome_binary()

            self.assertEqual(p, expected.strip())

        # this version has no java_home binary
        mock_mac_ver.return_value = ('10.5', '', '')
        p = finder._javahome_binary()

        self.assertEqual(p, None)

    @unittest.skipUnless(sys.version_info[:2] == (2, 6), "only py26")
    @mock.patch('platform.mac_ver')
    def test_javahome_binary_py26(self, mock_mac_ver):
        # this version has java_home binary
        mock_mac_ver.return_value = ('10.6.8', '', '')
        expected = '/System/Library/Java/JavaVirtualMachines/1.6.0.jdk/Contents/Home\n'

        finder = DarwinJVMFinder()

        # fake check_output
        with mock.patch('subprocess.Popen') as mock_popen:
            class proc:
                def communicate(self):
                    return (expected, )
            mock_popen.return_value = proc()

            p = finder._javahome_binary()

            self.assertEqual(p.strip(), expected.strip())

        # this version has no java_home binary
        mock_mac_ver.return_value = ('10.5', '', '')
        p = finder._javahome_binary()

        self.assertEqual(p, None)

if __name__ == '__main__':
    unittest.main()
