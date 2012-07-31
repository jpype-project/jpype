from jpype import *
import time

remote_pack="c:/tools/netbeean-remote-pack"

profiler_options = [
	"-agentpath:%s/lib/deployed/jdk15/windows/profilerinterface.dll=%s/lib,5140" % (remote_pack, remote_pack)
]

options = [
	'-verbose:gc', 
	'-Xmx16m', 
] #+ profiler_options

#startJVM(getDefaultJVMPath(), *options)
startJVM("c:/tools/jdk1.4.2/jre/bin/server/jvm.dll", *options)

class MyStr(str):
    def __del__(self):
        print 'string got deleted'
        
while True:
	buf = java.lang.String('5' * 1024 * 1024 * 5)
	buf = nio.convertToDirectBuffer(MyStr('5' * 1024 * 1024))
#	time.sleep(1)

