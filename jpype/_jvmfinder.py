# *****************************************************************************
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
# *****************************************************************************

import os
import sys

# ------------------------------------------------------------------------------


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
    this occures when the JVM does not match the architecture of Python
    32 vs 64 bit, or the JVM is older than the version used to compile
    JPype.
    """
    pass


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
        found_jamvm = False
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

        else:
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

        else:
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

            # Cygwin has a bug in realpath
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
