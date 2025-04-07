# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
# part of JPype1; author Martin K. Scherer; 2014
# from unittest import mock
import os
import pathlib
import sys
import unittest
from unittest import mock

from jpype._jvmfinder import (LinuxJVMFinder, JVMNotSupportedException, DarwinJVMFinder,
                              WindowsJVMFinder)


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
                      # cacoa and jamvm are not supported.
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
            # cacoa and javmvm should not be found
            self.assertEqual(
                p, os.path.join('jre/lib/amd64/server', 'libjvm.so'), 'wrong jvm returned')

    def test_find_libjvm_unsupported_jvm(self):
        """Checks for the case only unsupported JVM impls are being found."""
        walk_fake = [('jre', ('lib',), ()),
                     ('jre/lib', ('amd64',), ()),
                     ('jre/lib/amd64',
                      # cacoa and jamvm are not supported.
                      ('cacao', 'jamvm', ), ()),
                     ('jre/lib/amd64/cacao',
                      ('',), ('libjvm.so',)),
                     ('jre/lib/amd64/jamvm',
                      ('',), ('libjvm.so',)),
                     ]
        with mock.patch('os.walk') as mockwalk:
            # contains broken and working jvms
            mockwalk.return_value = walk_fake

            finder = LinuxJVMFinder()
            with self.assertRaises(JVMNotSupportedException):
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
            p, os.path.join('/usr/lib/jvm/java-6-openjdk-amd64/jre/lib/amd64/server', 'libjvm.so'))

    @mock.patch('platform.mac_ver')
    def testDarwinBinary(self, mock_mac_ver):
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

    def testLinuxGetFromBin(self):
        finder = LinuxJVMFinder()

        def f(s):
            return s
        with mock.patch('os.path') as pathmock, \
                mock.patch('jpype._jvmfinder.JVMFinder.find_libjvm') as mockfind:
            pathmock.exists.return_value = True
            pathmock.realpath = f
            pathmock.abspath = f
            pathmock.dirname = os.path.dirname
            pathmock.join = os.path.join
            finder._get_from_bin()
            self.assertEqual(
                pathmock.dirname.mock_calls[0][1], (finder._java,))

    @unittest.skipIf(sys.platform != "win", "only on windows")
    def testWindowsRegistry(self):
        finder = WindowsJVMFinder()
        jvm_home = pathlib.Path(finder.get_jvm_path())
        assert jvm_home.exists()
        assert jvm_home.is_dir()

if __name__ == '__main__':
    unittest.main()
