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

from . import _jvmfinder

# ------------------------------------------------------------------------------


class WindowsJVMFinder(_jvmfinder.JVMFinder):
    """
    Windows JVM library finder class
    """
    def __init__(self):
        """
        Sets up members
        """
        # Call the parent constructor
        _jvmfinder.JVMFinder.__init__(self)

        # Library file name
        self._libfile = "jvm.dll"

        # Search methods
        self._methods = (self._get_from_java_home, self._get_from_registry)

    def check(self, jvm):
        from ._windows import _checkJVMArch
        _checkJVMArch(jvm)

    def _get_from_registry(self):
        """
        Retrieves the path to the default Java installation stored in the
        Windows registry

        :return: The path found in the registry, or None
        """
        from ._windows import reg_keys
        for location in reg_keys:
            location = location.replace('\\', '/')
            jreKey = "/proc/registry/HKEY_LOCAL_MACHINE/{}".format(location)
            try:
                with open(jreKey + "/CurrentVersion") as f:
                    cv = f.read().split('\x00')
                versionKey = jreKey + "/" + cv[0]

                with open(versionKey + "/RunTimeLib") as f:
                    cv = f.read().split('\x00')

                return cv[0]

            except OSError:
                pass
        return None
