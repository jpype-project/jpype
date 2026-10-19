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
import sys
import jpype
import common


class ForNameTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    # Class.forName(String) resolves the caller's classloader via ART's own
    # native caller-sensitivity machinery, which - reached here through
    # JPype's own JNI call, not a real interpreted Java frame - resolves to
    # a loader with no dex visibility on Android (same underlying platform
    # difference as testForName2's explicit getSystemClassLoader() below;
    # see doc/android.rst).
    @common.skipOnAndroid("Class.forName(String) caller-sensitivity differs on Android")
    def testForName(self):
        cls = jpype.JClass('java.lang.Class')
        test = cls.forName('jpype.overloads.Test1')
        # Should return a java.lang.Class, rather than the python wrapper for java.lang.Class
        self.assertTrue(type(test) == type(cls.class_))
        self.assertEqual(test.getName(), 'jpype.overloads.Test1')

    # ClassLoader.getSystemClassLoader() returns a boot-loader stub with
    # no dex visibility on Android - unlike desktop, where it happens to
    # be the same object as the app's own classloader (see doc/android.rst).
    @common.skipOnAndroid("ClassLoader.getSystemClassLoader() has no dex visibility on Android")
    def testForName2(self):
        cls = jpype.JClass('java.lang.Class')
        clsloader = jpype.JClass(
            'java.lang.ClassLoader').getSystemClassLoader()
        test = cls.forName('jpype.overloads.Test1', True, clsloader)
        # Should return a java.lang.Class, rather than the python wrapper for java.lang.Class
        self.assertTrue(type(test) == type(cls.class_))
        self.assertEqual(test.getName(), 'jpype.overloads.Test1')
