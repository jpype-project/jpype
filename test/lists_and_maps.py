from jpype import *
import time


startJVM(getDefaultJVMPath())
#startJVM("c:/tools/jdk1.4.2/jre/bin/server/jvm.dll")

arr = java.util.ArrayList()

# no matching overloads found for this line:
arr.addAll([str(x) for x in xrange(50)])

print arr

hmap = java.util.HashMap()

# no matching overloads found for this line:
hmap.putAll({5:6, 7:8, 'hello':'there'})

print hmap

#for x in xrange(5):
#    # this works:
#    hmap.put(str(x), str(x))
#    # but this doesn't:
#    hmap.put(str(x), x)
#    
#    
## this throws: AttributeError: 'java.util.HashMap' object has no attribute 'iterator'
#for x in hmap:
#    print x, hmap[x]
