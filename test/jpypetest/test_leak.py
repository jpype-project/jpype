# -*- coding: utf-8 -*-
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
import jpype
import os
import sys
from os import path
import subrun
import unittest

# subrun._import() (used for the individual=True subprocess isolation this
# module's LeakTestCase relies on) loads this file directly from its path in
# a freshly spawned child interpreter, so this directory is not guaranteed to
# already be on sys.path there the way a normal pytest collection run would
# have it. Make the leakharness import work either way.
sys.path.insert(0, path.dirname(path.abspath(__file__)))
from leakharness import LeakChecker, haveResource  # noqa: E402


def hasRefCount():
    try:
        sys.getrefcount(sys)
        return True
    except:
        return False


@subrun.TestCase(individual=True)
class LeakTestCase(unittest.TestCase):

    def setUp(self):
        root = path.dirname(path.abspath(path.dirname(__file__)))
        jpype.addClassPath(path.join(root, 'classes'))
        jvm_path = jpype.getDefaultJVMPath()
        classpath_arg = "-Djava.class.path=%s"
        classpath_arg %= jpype.getClassPath()
        jpype.startJVM(jvm_path, "-ea",
                       # "-Xcheck:jni",
                       "-Xmx256M", "-Xms16M", classpath_arg)

    def assertNotLeaky(self, function, counts=5000):
        lc = LeakChecker()
        # leaksweep.py's config-driven sweep runs these same tests through a
        # time budget instead of a fixed batch count -- it sets this env var
        # (inherited by this subrun-spawned child process) rather than
        # threading a parameter through unittest's fixed test-method
        # signature. Absent it (the normal pytest run of this file), nothing
        # changes from the original fixed-batch behavior.
        budget = os.environ.get('JPYPE_LEAK_BUDGET_SECONDS')
        if budget:
            leaky = lc.memTestBudget(function, counts, float(budget))
        else:
            leaky = lc.memTest(function, counts)
        assert not leaky, 'Potential leak found'

    @unittest.skipUnless(haveResource(), "resource not available")
    def testStringLeak(self):
        def stringFunc():
            jpype.java.lang.String('aaaaaaaaaaaaaaaaa')
        self.assertNotLeaky(stringFunc)

    @unittest.skipUnless(haveResource(), "resource not available")
    def testStringLeak__str__(self):
        def stringFunc():
            # Casting to a string includes a cache stage, we want to make sure
            # that the cache is tidying up properly.
            s = jpype.types.JString('aaaaaaaaaaaaaaaaa')
            str(s)
        self.assertNotLeaky(stringFunc)

    @unittest.skipUnless(haveResource(), "resource not available")
    def testClassLeak(self):
        def classFunc():
            cls = jpype.JClass('java.lang.String')
        self.assertNotLeaky(classFunc)

    @unittest.skipUnless(haveResource(), "resource not available")
    def testCtorLeak(self):
        cls = jpype.JClass("java.lang.String")

        def func():
            cls("test")

        self.assertNotLeaky(func)

    @unittest.skipUnless(haveResource(), "resource not available")
    def testInvokeLeak(self):
        jstr = jpype.JString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaa")

        def func():
            jstr.getBytes()

        self.assertNotLeaky(func)
