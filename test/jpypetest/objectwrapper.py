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
from jpype import *
import unittest, common

def suite() :
    return unittest.makeSuite(ObjectWrapperTestCase)

class ObjectWrapperTestCase(common.JPypeTestCase) :
    def testCallOverloads(self) :
        # build the harness   
        h = JPackage("jpype.objectwrapper").Test1()
        
        o = java.lang.Integer(1)
        assert h.Method1(JObject(o, java.lang.Number)) == 1
        assert h.Method1(o) == 2  
        assert h.Method1(JObject(java.lang.Integer(1), java.lang.Object)) == 3
        assert h.Method1(JString("")) == 4

