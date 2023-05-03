# *****************************************************************************
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
#   See NOTICE file for details.
#
# *****************************************************************************
#   Copyright 2013 Thomas Calmant

import os
import sys

__all__ = ['getDefaultJVMPath',
           'JVMNotFoundException', 'JVMNotSupportedException']

try:
    import winreg
except ImportError:
    winreg = None   # type: ignore[assignment]


class JVMNotFoundException(ValueError):
    """ Exception raised when no JVM was found in the search path.

    This exception is raised when the all of the places searched did not
    contain a JVM. The locations searched depend on the machine architecture.
    To avoid this exception specify the JAVA_HOME environment variable as a
    valid jre or jdk root directory.
    """
    pass


class JVMNotSupportedException(ValueError):
    """ Exception raised when the JVM is not supported.

    This exception is raised after a search found a valid Java home directory
    was found, but the JVM shared library found is not supported. Typically
    this occurs when the JVM does not match the architecture of Python
    32 vs 64 bit, or the JVM is older than the version used to compile
    JPype.
    """
    pass


def getDefaultJVMPath():
    """
    Retrieves the path to the default or first found JVM library

    Returns:
      The path to the JVM shared library file

    Raises:
      JVMNotFoundException: If there was no JVM found in the search path.
      JVMNotSupportedException: If the JVM was found was not compatible with
        Python due to cpu architecture.

    """
    if sys.platform == "win32":
        finder = WindowsJVMFinder()
    elif sys.platform == "darwin":
        finder = DarwinJVMFinder()
    else:
        finder = LinuxJVMFinder()
    return finder.get_jvm_path()


class JVMFinder(object):
    """
    JVM library finder base class
    """

    def __init__(self):
        """
        Sets up members
        """
        # Library file name
        self._libfile = "libjvm.so"

        # Predefined locations
        self._locations = ("/usr/lib/jvm", "/usr/java")

        # Search methods
        self._methods = (self._get_from_java_home,
                         self._get_from_known_locations)

    def find_libjvm(self, java_home):
        """
        Recursively looks for the given file

        Parameters:
            java_home(str): A Java home folder
            filename(str): filename: Name of the file to find

        Returns:
            The first found file path, or None
        """
        non_supported_jvm = ('cacao', 'jamvm')
        found_non_supported_jvm = False

        # Look for the file
        for root, _, names in os.walk(java_home):
            if self._libfile in names:
                # Found it, but check for non supported jvms
                candidate = os.path.split(root)[1]
                if candidate in non_supported_jvm:
                    found_non_supported_jvm = True
                    continue  # maybe we will find another one?
                return os.path.join(root, self._libfile)

        if found_non_supported_jvm:
            raise JVMNotSupportedException("Sorry '{0}' is known to be "
                                           "broken. Please ensure your "
                                           "JAVA_HOME contains at least "
                                           "another JVM implementation "
                                           "(eg. server)"
                                           .format(candidate))
        # File not found
        raise JVMNotFoundException("Sorry no JVM could be found. "
                                   "Please ensure your JAVA_HOME "
                                   "environment variable is pointing "
                                   "to correct installation.")

    def find_possible_homes(self, parents):
        """
        Generator that looks for the first-level children folders that could be
        Java installations, according to their name

        Parameters:
            parents (str[]): A list of parent directories

        Returns:
            A list of the possible JVM installation folders
        """
        homes = []
        java_names = ('jre', 'jdk', 'java')

        for parent in parents:
            # Fast exit if folder does not exist
            if not os.path.exists(parent):
                continue

            for childname in sorted(os.listdir(parent)):
                # Compute the real path
                path = os.path.realpath(os.path.join(parent, childname))
                if path in homes or not os.path.isdir(path):
                    # Already known path, or not a directory -> ignore
                    continue

                # Check if the path seems OK
                real_name = os.path.basename(path).lower()
                for java_name in java_names:
                    if java_name in real_name:
                        # Correct JVM folder name
                        homes.append(path)
                        yield path
                        break

    def check(self, jvm):
        """
        Check if the jvm is valid for this architecture.

        This method should be overriden for each architecture.

        Raises:
            JVMNotSupportedException: If the jvm is not supported.
        """
        pass

    def get_jvm_path(self):
        """
        Retrieves the path to the default or first found JVM library

        Returns:
            The path to the JVM shared library file

        Raises:
            ValueError: No JVM library found or No Support JVM found
        """
        jvm_notsupport_ext = None
        for method in self._methods:
            try:
                jvm = method()

                # If found check the architecture
                if jvm:
                    self.check(jvm)
            except NotImplementedError:
                # Ignore missing implementations
                pass
            except JVMNotFoundException:
                # Ignore not successful methods
                pass
            except JVMNotSupportedException as e:
                jvm_notsupport_ext = e

            else:
                if jvm is not None:
                    return jvm

        if jvm_notsupport_ext is not None:
            raise jvm_notsupport_ext
        raise JVMNotFoundException("No JVM shared library file ({0}) "
                                   "found. Try setting up the JAVA_HOME "
                                   "environment variable properly."
                                   .format(self._libfile))

    def _get_from_java_home(self):
        """
        Retrieves the Java library path according to the JAVA_HOME environment
        variable

        Returns:
            The path to the JVM library, or None
        """
        # Get the environment variable
        java_home = os.getenv("JAVA_HOME")
        if java_home and os.path.exists(java_home):
            # Get the real installation path
            java_home = os.path.realpath(java_home)

            if not os.path.exists(java_home):
                java_home = os.getenv("JAVA_HOME")

            # Look for the library file
            return self.find_libjvm(java_home)

    def _get_from_known_locations(self):
        """
        Retrieves the first existing Java library path in the predefined known
        locations

        Returns:
            The path to the JVM library, or None
        """
        for home in self.find_possible_homes(self._locations):
            jvm = self.find_libjvm(home)
            if jvm is not None:
                return jvm


class LinuxJVMFinder(JVMFinder):
    """
    Linux JVM library finder class
    """

    def __init__(self):
        """
        Sets up members
        """
        # Call the parent constructor
        JVMFinder.__init__(self)

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
        self._methods.append(self._javahome_binary)

        # Predefined locations
        self._locations = ('/Library/Java/JavaVirtualMachines',)

    def _javahome_binary(self):
        """
        for osx > 10.5 we have the nice util /usr/libexec/java_home available. Invoke it and
        return its output. It seems this tool has been removed in osx 10.9.
        """
        import platform
        import subprocess
        from packaging.version import Version

        current = Version(platform.mac_ver()[0][:4])
        if current >= Version('10.6') and current < Version('10.9'):
            return subprocess.check_output(
                ['/usr/libexec/java_home']).strip()


def _checkJVMArch(jvmPath, maxsize=sys.maxsize):
    import struct
    IMAGE_FILE_MACHINE_I386 = 332
    IMAGE_FILE_MACHINE_IA64 = 512
    IMAGE_FILE_MACHINE_AMD64 = 34404

    is64 = maxsize > 2**32
    with open(jvmPath, "rb") as f:
        s = f.read(2)
        if s != b"MZ":
            raise JVMNotSupportedException("JVM not valid")
        f.seek(60)
        s = f.read(4)
        header_offset = struct.unpack("<L", s)[0]
        f.seek(header_offset + 4)
        s = f.read(2)
        machine = struct.unpack("<H", s)[0]

    if machine == IMAGE_FILE_MACHINE_I386:
        if is64:
            raise JVMNotSupportedException(
                "JVM mismatch, python is 64 bit and JVM is 32 bit.")
    elif machine == IMAGE_FILE_MACHINE_IA64 or machine == IMAGE_FILE_MACHINE_AMD64:
        if not is64:
            raise JVMNotSupportedException(
                "JVM mismatch, python is 32 bit and JVM is 64 bit.")
    else:
        raise JVMNotSupportedException("Unable to determine JVM Type")


reg_keys = [r"SOFTWARE\JavaSoft\Java Runtime Environment",
            r"SOFTWARE\JavaSoft\JRE",
            ]


class WindowsJVMFinder(JVMFinder):
    """
    Windows JVM library finder class
    """

    def __init__(self):
        """
        Sets up members
        """
        # Call the parent constructor
        JVMFinder.__init__(self)

        # Library file name
        self._libfile = "jvm.dll"

        # Search methods
        self._methods = (self._get_from_java_home, self._get_from_registry)

    def check(self, jvm):
        _checkJVMArch(jvm)

    def _get_from_registry(self):
        """
        Retrieves the path to the default Java installation stored in the
        Windows registry

        :return: The path found in the registry, or None
        """
        if not winreg:
            return None
        for location in reg_keys:
            try:
                jreKey = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, location)
                cv = winreg.QueryValueEx(jreKey, "CurrentVersion")
                versionKey = winreg.OpenKey(jreKey, cv[0])
                winreg.CloseKey(jreKey)

                cv = winreg.QueryValueEx(versionKey, "RuntimeLib")
                winreg.CloseKey(versionKey)

                return cv[0]
            except OSError:
                pass
        return None
