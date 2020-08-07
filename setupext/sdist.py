# -*- coding: utf-8 -*-
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
import os
import distutils
from distutils.dir_util import copy_tree, remove_tree, mkpath
from distutils.file_util import copy_file
from setuptools.command.sdist import sdist

# Customization of the sdist


class BuildSourceDistribution(sdist):
    """
    Override some behavior on sdist

    Copy the build/lib to native/jars to remove javac/jdk dependency
    """

    def run(self):
        dest = os.path.join('native', 'jars')

        # We need to build a jar cache for the source distribution
        cmd = self.distribution.get_command_obj('build_ext')

        # Call with jar only option
        cmd.jar = True
        self.run_command('build_ext')

        # Find out the location of the jar file
        dirname = os.path.dirname(cmd.get_ext_fullpath("JAVA"))
        jarFile = os.path.join(dirname, "org.jpype.jar")

        # Also build the test harness files
        self.run_command("test_java")
        if not os.path.exists(jarFile):
            distutils.log.error("Jar source file is missing from build")
            raise distutils.errors.DistutilsPlatformError(
                "Error copying jar file")
        mkpath(dest)
        copy_file(jarFile, dest)

        # Collect the sources
        sdist.run(self)

        # Clean up the jar cache after sdist
        remove_tree(dest)
