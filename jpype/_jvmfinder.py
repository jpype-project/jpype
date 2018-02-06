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
import sys

# ------------------------------------------------------------------------------

class JVMNotFoundException(RuntimeError):
    pass

class JVMNotSupportedException(RuntimeError):
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

        :param java_home: A Java home folder
        :param filename: Name of the file to find
        :return: The first found file path, or None
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
                    continue # maybe we will find another one?
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

        :param parents: A list of parent directories
        :return: The possible JVM installation folders
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
        :raise JVMNotSupportedException: If the jvm is not supported.
        """
        pass

    def get_jvm_path(self):
        """
        Retrieves the path to the default or first found JVM library

        :return: The path to the JVM shared library file
        :raise ValueError: No JVM library found
        """
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
            except JVMNotSupportedException:
                pass

            else:
                if jvm is not None:
                    return jvm

        else:
            raise JVMNotFoundException("No JVM shared library file ({0}) "
                                       "found. Try setting up the JAVA_HOME "
                                       "environment variable properly."
                                       .format(self._libfile))


    def _get_from_java_home(self):
        """
        Retrieves the Java library path according to the JAVA_HOME environment
        variable

        :return: The path to the JVM library, or None
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

        :return: The path to the JVM library, or None
        """
        for home in self.find_possible_homes(self._locations):
            jvm = self.find_libjvm(home)
            if jvm is not None:
                return jvm
