#*****************************************************************************
#   Copyright 2013 Thomas Calmant
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

import os

from . import _jvmfinder

# ------------------------------------------------------------------------------

class LinuxJVMFinder(_jvmfinder.JVMFinder):
    """
    Linux JVM library finder class
    """
    def __init__(self):
        """
        Sets up members
        """
        # Call the parent constructor
        _jvmfinder.JVMFinder.__init__(self)

        # Java bin file
        self._java = "/usr/bin/java"

        # Library file name
        self._libfile = "libjvm.so"

        # Predefined locations
        self._locations = ("/usr/lib/jvm", "/usr/java", "/opt/sun")

        # Search methods
        self._methods = (self._get_from_java_home,
                         self._get_from_bin,
                         self._get_from_known_locations)

    def _get_from_bin(self):
        """
        Retrieves the Java library path according to the real installation of
        the java executable

        :return: The path to the JVM library, or None
        """
        # Find the real interpreter installation path
        java_bin = os.path.realpath(self._java)
        if os.path.exists(java_bin):
            # Get to the home directory
            java_home = os.path.abspath(os.path.join(os.path.dirname(java_bin),
                                                     '..'))

            # Look for the JVM library
            return self.find_libjvm(java_home)
