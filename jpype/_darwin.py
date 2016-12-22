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

# Reuse the Linux code
from ._linux import LinuxJVMFinder

# ------------------------------------------------------------------------------

class DarwinJVMFinder(LinuxJVMFinder):
    """
    Mac OS X JVM library finder class
    """
    def __init__(self):
        """
        Sets up members
        """
        # Call the parent constructor
        LinuxJVMFinder.__init__(self)

        # Library file name
        self._libfile = "libjli.dylib"

        self._methods = list(self._methods)
        self._methods.append(self._pre_vm7_path)
        self._methods.append(self._javahome_binary)

        # Predefined locations
        self._locations = ('/Library/Java/JavaVirtualMachines',)


    def _pre_vm7_path(self):
        """
        Returns the previous constant JVM library path:
        '/System/Library/Frameworks/JavaVM.framework/JavaVM'
        """
        return '/System/Library/Frameworks/JavaVM.framework/JavaVM'

    def _javahome_binary(self):
        """
        for osx > 10.5 we have the nice util /usr/libexec/java_home available. Invoke it and
        return its output. It seems this tool has been removed in osx 10.9.
        """
        import platform
        import subprocess
        from distutils.version import StrictVersion

        current = StrictVersion(platform.mac_ver()[0][:4])
        if current >= StrictVersion('10.6') and current < StrictVersion('10.9'):
            if hasattr(subprocess, 'check_output'):
                java_home = subprocess.check_output(['/usr/libexec/java_home']).strip()
            else:
                java_home = subprocess.Popen(['/usr/libexec/java_home'], stdout=subprocess.PIPE).communicate()[0]
            return java_home

# ------------------------------------------------------------------------------

# Alias
JVMFinder = DarwinJVMFinder
