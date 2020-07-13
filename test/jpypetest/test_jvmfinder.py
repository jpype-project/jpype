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


import unittest
from unittest import mock
import common
import os

import jpype._jvmfinder
from jpype._jvmfinder import *


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

            finder = jpype._jvmfinder.LinuxJVMFinder()
            p = finder.find_libjvm('arbitrary java home')
            self.assertEqual(
                p, os.path.join('jre/lib/amd64/server', 'libjvm.so'), 'wrong jvm returned')

        with mock.patch('os.walk') as mockwalk:
            # contains only broken jvms, since server impl is removed
            walk_fake[-1] = ((), (), (),)
            mockwalk.return_value = walk_fake

            finder = jpype._jvmfinder.LinuxJVMFinder()
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

        finder = jpype._jvmfinder.LinuxJVMFinder()
        p = finder._get_from_bin()

        self.assertEqual(
            p, os.path.join('/usr/lib/jvm/java-6-openjdk-amd64/jre/lib/amd64/server', 'libjvm.so'))

    @mock.patch('platform.mac_ver')
    def testDarwinBinary(self, mock_mac_ver):
        # this version has java_home binary
        mock_mac_ver.return_value = ('10.6.8', '', '')

        expected = '/System/Library/Java/JavaVirtualMachines/1.6.0.jdk/Contents/Home\n'

        finder = jpype._jvmfinder.DarwinJVMFinder()

        # fake check_output
        with mock.patch('subprocess.check_output') as mock_checkoutput:
            mock_checkoutput.return_value = expected
            p = finder._javahome_binary()
            self.assertEqual(p, expected.strip())

        # this version has no java_home binary
        mock_mac_ver.return_value = ('10.5', '', '')
        p = finder._javahome_binary()
        self.assertEqual(p, None)

    # FIXME this is testing the details of the implementation rather than the results.
    # it is included only for coverage purposes.  Revise this to be a more meaningful test
    # next time it breaks.
    # FIXME this test does passes locally but not in the CI.  Disabling for now.
    @common.unittest.skip
    def testPlatform(self):
        with mock.patch('jpype._jvmfinder.sys') as mocksys, mock.patch('jpype._jvmfinder.WindowsJVMFinder') as finder:
            mocksys.platform = 'win32'
            jpype._jvmfinder.getDefaultJVMPath()
            self.assertIn(finder().get_jvm_path, finder.mock_calls)

        with mock.patch('jpype._jvmfinder.sys') as mocksys, mock.patch('jpype._jvmfinder.LinuxJVMFinder') as finder:
            mocksys.platform = 'linux'
            getDefaultJVMPath()
            self.assertIn(finder().get_jvm_path, finder.mock_calls)

        with mock.patch('jpype._jvmfinder.sys') as mocksys, mock.patch('jpype._jvmfinder.DarwinJVMFinder') as finder:
            mocksys.platform = 'darwin'
            getDefaultJVMPath()
            self.assertIn(finder().get_jvm_path, finder.mock_calls)

    def testLinuxGetFromBin(self):
        finder = jpype._jvmfinder.LinuxJVMFinder()

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

    # FIXME this test is faking files using the mock system.  Replace it with stub
    # files so that we have a more accurate test rather than just testing the implementation.
    # FIXME this fails in the CI but works locally.   Disabling this for now.
    @common.unittest.skip
    def testCheckArch(self):
        import struct
        with mock.patch("builtins.open", mock.mock_open(read_data="data")) as mock_file, \
                self.assertRaises(JVMNotSupportedException):
            jpype._jvmfinder._checkJVMArch('path', 2**32)

        data = struct.pack('<ccIH', b'M', b'Z', 0, 332)
        with mock.patch("builtins.open", mock.mock_open(read_data=data)) as mock_file:
            jpype._jvmfinder._checkJVMArch('path', 2**32)
        with mock.patch("builtins.open", mock.mock_open(read_data=data)) as mock_file, \
                self.assertRaises(JVMNotSupportedException):
            jpype._jvmfinder._checkJVMArch('path', 2**64)

        data = struct.pack('<ccIH', b'M', b'Z', 0, 512)
        with mock.patch("builtins.open", mock.mock_open(read_data=data)) as mock_file:
            jpype._jvmfinder._checkJVMArch('path', 2**64)
        with mock.patch("builtins.open", mock.mock_open(read_data=data)) as mock_file, \
                self.assertRaises(JVMNotSupportedException):
            jpype._jvmfinder._checkJVMArch('path', 2**32)

        data = struct.pack('<ccIH', b'M', b'Z', 0, 34404)
        with mock.patch("builtins.open", mock.mock_open(read_data=data)) as mock_file:
            jpype._jvmfinder._checkJVMArch('path', 2**64)
        with mock.patch("builtins.open", mock.mock_open(read_data=data)) as mock_file, \
                self.assertRaises(JVMNotSupportedException):
            jpype._jvmfinder._checkJVMArch('path', 2**32)

    def testWindowsRegistry(self):
        finder = jpype._jvmfinder.WindowsJVMFinder()
        with mock.patch("jpype._jvmfinder.winreg") as winregmock:
            winregmock.QueryValueEx.return_value = ('success', '')
            self.assertEqual(finder._get_from_registry(), 'success')
            winregmock.OpenKey.side_effect = OSError()
            self.assertEqual(finder._get_from_registry(), None)


if __name__ == '__main__':
    unittest.main()
