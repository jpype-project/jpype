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
    Linux JVM library finder class
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
        winreg = self._get_winreg()
        if winreg!=None:
            self._methods = (self._get_from_java_home, self._get_from_registry)
        else:
            self._methods = (self._get_from_java_home, )

    def _get_winreg(self):
        # Py2
        try:
            import _winreg as winreg
            return winreg
        except ImportError:
            pass

        # in Py3, winreg has been moved
        try:
            import winreg
            return winreg
        except ImportError:
            pass
        return None


    def _get_from_registry(self):
        """
        Retrieves the path to the default Java installation stored in the
        Windows registry

        :return: The path found in the registry, or None
        """
        try:
            # Winreg is an optional package in cygwin
            winreg = self._get_winreg()
            if not winreg:
                return None

            jreKey = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                                r"SOFTWARE\JavaSoft\Java Runtime Environment")
            cv = winreg.QueryValueEx(jreKey, "CurrentVersion")
            versionKey = winreg.OpenKey(jreKey, cv[0])
            winreg.CloseKey(jreKey)

            cv = winreg.QueryValueEx(versionKey, "RuntimeLib")
            winreg.CloseKey(versionKey)

            return cv[0]

        except WindowsError:
            return None
