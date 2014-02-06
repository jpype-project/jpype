#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#	   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#   
#*****************************************************************************
import unittest
from jpypetest import *

import jpype
import os.path
import sys

def suite() :
	return unittest.TestSuite( (
		numeric.suite(),
		attr.suite(),
		array.suite(),
		objectwrapper.suite(),
		proxy.suite(), 
		exc.suite(), 
		serial.suite(),	  
		mro.suite(),
	))
	
def runTest() :	
	root = os.path.abspath(os.path.dirname(__file__))

	print "Running testsuite using JVM", jpype.getDefaultJVMPath()
	jpype.startJVM(jpype.getDefaultJVMPath(),
				"-ea",
				#"-Xcheck:jni", 
				"-Xmx256M", "-Xms64M",
				"-Djava.class.path=./classes%s%s%sclasses" % (os.pathsep, root, os.sep))

	runner = unittest.TextTestRunner()
	result = runner.run(suite())
		
	jpype.shutdownJVM()
	if not result.wasSuccessful():
		sys.exit(-1)

if __name__ == '__main__' :
	runTest()
	
	
