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
from jpype import JException, java, JavaException, JProxy, JClass
import common
import os
import tempfile
import traceback

class SerializationTestCase(common.JPypeTestCase) :

    def setUp(self):
        self.tempname = tempfile.mktemp()

    def tearDown(self):
        os.remove(self.tempname)

    def testSerialize(self) :
        o = JClass("jpype.serial.SerializationTest")()
        fos = java.io.FileOutputStream(self.tempname)
        oos = java.io.ObjectOutputStream(fos)
        oos.writeObject(o)
        oos.flush()
        oos.close()       
        fos.close()
        
        
       
# The following cannto work because JPype has no way to simulate the "caller's ClassLoader" 
#    def testDeSerialize(self) :
#        fis = java.io.FileInputStream(self.tempname)
#        ois = java.io.ObjectInputStream(fis)
#        
#        o = ois.readObject()
#        ois.close()
#        fis.close()
