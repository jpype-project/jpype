# *****************************************************************************
#   Copyright 2004-2008 Steve Menard
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
# *****************************************************************************

import pytest
import jpype
import logging
import os
from os import path
import sys
try:
    import unittest2 as unittest
except ImportError:
    import unittest


CLASSPATH = None

def version(v):
    return tuple([int(i) for i in v.split('.')])


@pytest.mark.usefixtures("common_opts")
class JPypeTestCase(unittest.TestCase):
    def setUp(self):
        if not jpype.isJVMStarted():
            try:
                import faulthandler
                faulthandler.enable()
                faulthandler.disable()
            except:
                pass
            root = path.dirname(path.abspath(path.dirname(__file__)))
            jpype.addClassPath(path.join(root, 'classes'))
            jvm_path = jpype.getDefaultJVMPath()
            logger = logging.getLogger(__name__)
            logger.info("Running testsuite using JVM %s" % jvm_path)
            classpath_arg = "-Djava.class.path=%s"
            args = ["-ea", "-Xmx256M", "-Xms16M", "-Xcheck:jni" ]
            # TODO: enabling this check crashes the JVM with: FATAL ERROR in native method: Bad global or local ref passed to JNI
            #"-Xcheck:jni",
            if self._jar:
                import warnings
                jpype.addClassPath(self._jar)
                warnings.warn("using jar instead of thunks")
            if self._convertStrings:
                import warnings
                warnings.warn("using deprecated convertStrings")
            if self._jacoco:
                import warnings
                args.append("-javaagent:project/coverage/org.jacoco.agent-0.8.5-runtime.jar=destfile=jacoco.exec,includes=org.jpype.*")
                warnings.warn("using JaCoCo")

            classpath_arg %= jpype.getClassPath()
            args.append(classpath_arg)
            #JPypeTestCase.str_conversion = eval(os.getenv('JPYPE_STR_CONVERSION', 'True'))
            jpype.startJVM(jvm_path, *args, convertStrings=self._convertStrings)
        self.jpype = jpype.JPackage('jpype')
        if sys.version < '3':
            self.assertCountEqual = self.assertItemsEqual

    def tearDown(self):
        pass


if __name__ == '__main__':
    unittest.main()
