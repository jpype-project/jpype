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
import sys
import subprocess
import distutils.cmd
import distutils.log
from distutils.errors import DistutilsPlatformError
from distutils.dir_util import copy_tree
import glob
import re
import shlex
import shutil


def compileJava(self, coverage):
    javac = "javac"
    try:
        if os.path.exists(os.path.join(os.environ['JAVA_HOME'], 'bin', 'javac')):
            javac = '"%s"' % os.path.join(os.environ['JAVA_HOME'], 'bin', 'javac')
    except KeyError:
        pass
    jar = "jar"
    try:
        if os.path.exists(os.path.join(os.environ['JAVA_HOME'], 'bin', 'jar')):
            jar = '"%s"' % os.path.join(os.environ['JAVA_HOME'], 'bin', 'jar')
    except KeyError:
        pass
    target_version = "1.8"
    srcs = glob.glob('native/java/**/*.java', recursive=True)
    src1 = [i for i in srcs if "JPypeClassLoader" in i]
    src2 = [i for i in srcs if not "JPypeClassLoader" in i]
    cmd1 = shlex.split('%s -d build/lib -g:lines -source %s -target %s' %
                       (javac, target_version, target_version))
    cmd1.extend(src1)
    debug = "-g:lines"
    if coverage:
        debug = "-g:lines,vars,source"
    cmd2 = shlex.split('%s -d build/classes %s -source %s -target %s -cp build/lib' %
                       (javac, debug, target_version, target_version))
    cmd2.extend(src2)
    os.makedirs("build/lib", exist_ok=True)
    try:
        shutil.copytree('native/java/', 'build/classes', ignore=shutil.ignore_patterns('*.java'))
    except Exception:
        pass
    self.announce("  %s" % " ".join(cmd1), level=distutils.log.INFO)
    subprocess.check_call(cmd1)
    self.announce("  %s" % " ".join(cmd2), level=distutils.log.INFO)
    subprocess.check_call(cmd2)
    jar = "jar"
    try:
        if os.path.exists(os.path.join(os.environ['JAVA_HOME'], 'bin', 'jar')):
            jar = '"%s"' % os.path.join(os.environ['JAVA_HOME'], 'bin', 'jar')
    except KeyError:
        pass
    cmd3 = shlex.split(
        '"%s" cvf build/lib/org.jpype.jar -C build/classes/ .' % jar)
    self.announce("  %s" % " ".join(cmd3), level=distutils.log.INFO)
    subprocess.check_call(cmd3)


class BuildJavaCommand(distutils.cmd.Command):
    """A custom command to create jar file during build."""

    description = 'build jpype jar'
    user_options = []

    def initialize_options(self):
        """Set default values for options."""
        pass

    def finalize_options(self):
        """Post-process options."""
        pass

    def run(self):
        """Run command."""
        java = self.distribution.enable_build_jar

        # Try to use the cach if we are not requested build
        if not java:
            src = os.path.join('native', 'jars')
            dest = os.path.join('build', 'lib')
            if os.path.exists(src):
                distutils.log.info("Using Jar cache")
                copy_tree(src, dest)
                return

        distutils.log.info(
            "Jar cache is missing, using --enable-build-jar to recreate it.")

        coverage = self.distribution.enable_coverage

        # build the jar
        try:
            compileJava(self, coverage)
        except subprocess.CalledProcessError as exc:
            distutils.log.error(exc.output)
            raise DistutilsPlatformError("Error executing {}".format(exc.cmd))
