#*****************************************************************************
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
#*****************************************************************************

import jpype
from os import path
import unittest2 as unittest

CLASSPATH = None

class JPypeTestCase(unittest.TestCase) :
    def setUp(self) :
        if not jpype.isJVMStarted():
            root = path.dirname(path.abspath(path.dirname(__file__)))
            jvm_path = jpype.getDefaultJVMPath()
            print "Running testsuite using JVM", jvm_path
            classpath_arg = "-Djava.class.path=%s"
            classpath_arg %= path.join(root, 'classes')
            jpype.startJVM(jvm_path, "-ea",
                           # "-Xcheck:jni", 
                           "-Xmx256M", "-Xms64M", classpath_arg)
        self.jpype = jpype.JPackage('jpype')

    def tearDown(self) :
        pass
