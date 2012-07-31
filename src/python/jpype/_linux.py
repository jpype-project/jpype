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

import os, re



_KNOWN_LOCATIONS = [
    ("/opt/sun/", re.compile(r"j2sdk(.+)/jre/lib/i386/client/libjvm.so") ),
    ("/usr/java/", re.compile(r"j2sdk(.+)/jre/lib/i386/client/libjvm.so") ),
    ("/usr/java/", re.compile(r"jdk(.+)/jre/lib/i386/client/libjvm.so") ),
]

JRE_ARCHS = [
			 "amd64/server/libjvm.so",
			 "i386/client/libjvm.so",
			 "i386/server/libjvm.so",
			 ]


def getDefaultJVMPath() :
    jvm = _getJVMFromJavaHome()
    if jvm is not None :
        return jvm
       
    #on linux, the JVM has to be in the LD_LIBRARY_PATH anyway, so might as well inspect it first
    jvm = _getJVMFromLibPath()
    if jvm is not None :
        return jvm
    
    # failing that, lets look in the "known" locations
    for i in _KNOWN_LOCATIONS :
        # TODO
        pass

    return "/usr/java/jre1.5.0_05/lib/i386/client/libjvm.so"
        
def _getJVMFromJavaHome():
	java_home = os.getenv("JAVA_HOME")
	rootJre = None
	if os.path.exists(java_home+"/bin/javac") :
		# this is a JDK home
		rootJre = java_home + '/jre/lib'
	elif os.path.exists(java_home+"/bin/java") :
		# this is a JRE home
		rootJre = java_home + '/lib'
	else:
		return None
	
	for i in JRE_ARCHS :
		if os.path.exists(rootJre+"/"+i) :
			return rootJre+"/"+i
	return None
		
        
def _getJVMFromLibPath() :
    if 'LD_LIBRARY_PATH' in os.environ :
        libpath = os.environ['LD_LIBRARY_PATH']
        if libpath is None :
            return None
        
        paths = libpath.split(os.pathsep)
        for i in paths :
            if i.find('jre') != -1 :
                # this could be it!
                # TODO
                pass
                
    return None
    
