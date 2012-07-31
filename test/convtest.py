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
import array
import time
import os

def generateStringData(aSize):
    return ''.join(['a']*aSize)

DATA_SIZE = 5*1024*1024 # 5 MB



def runBaseline(data):    
    print 'Running baseline test : converting a python string->array.array->JArray(JByte). size = ', len(data)/1024.0, 'kb'
    print '    Start time (no optimize) on my machine is 3.56 seconds.'
    start = time.time()
     
    #darr = array.array('b', DATA)
    arr_cls = jpype.JArray(jpype.JByte)
    java_arr = arr_cls(DATA)
    
    end = time.time()
    
    print '    test run in', (end-start), 'seconds.'

def runStringToByteBuffer(data):
    print 'Running String conversion to byte buffer. size = ', len(data)/1024.0, 'kb'
    start = time.time()

    bb = jpype.nio.convertToDirectBuffer(data)

    end = time.time()
    
    print '    test run in', (end-start), 'seconds.'

    jpype.JPackage("jpype").nio.NioReceive.receiveBuffer(bb)

def runStringToByteArray(data):
    print 'Running String conversion to byte array. size = ', len(data)/1024.0, 'kb'
    start = time.time()

    arr_cls = jpype.JArray(jpype.JByte)
    java_arr = arr_cls(data)

    end = time.time()
    
    print '    test run in', (end-start), 'seconds.'

root = os.path.abspath(os.path.dirname(__file__))
jpype.startJVM(jpype.getDefaultJVMPath(), "-ea", "-Xmx5M", "-verbose:gc", "-Djava.class.path=./classes%s%s%sclasses" % (os.pathsep, root, os.sep))

DELETED = False
class MyStr(str):
    def __del__(self):
        global DELETED
        print 'string got deleted'
        DELETED = True

def testStringMemory():
    print 'with keeping the data'
    data = MyStr('5' * 1024)
    print data
    buf = jpype.nio.convertToDirectBuffer(data)
#    print buf.get()
#    print buf.get()
#    print buf.get()

    print 'now deleting the data'
    del data
#    print buf.get()
#    print buf.get()
#    print buf.get()
#    print buf.get()
    
    print 'now deleting the buffer itself'
    del buf
    print 'now waiting for the string to get deleted'
    while not DELETED:
        time.sleep(1)
        
        print '.',
        jpype.JPackage("jpype").nio.NioReceive.allocSomeMemory()

testStringMemory()

#for i in range(1,5) :
    
#    DATA = generateStringData(DATA_SIZE*i)
#    runBaseline(DATA)
#    runStringToByteBuffer(DATA)
#    runStringToByteArray(DATA)

    # expressly delete data to test the GC ...
    #del DATA
    #for i in range(3) :
    #    print 'GC', i
    #    jpype.JClass("java.lang.System").gc();
    #    time.sleep(15)

jpype.shutdownJVM()