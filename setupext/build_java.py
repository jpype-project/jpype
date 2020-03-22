# -*- coding: utf-8 -*-
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


def compileJava(self, coverage):
    target_version = "1.7"
    srcs = glob.glob('native/java/**/*.java', recursive=True)
    src1 = [i for i in srcs if "JPypeClassLoader" in i]
    src2 = [i for i in srcs if not "JPypeClassLoader" in i]
    cmd1 = shlex.split('javac -d build/lib -g:none -source %s -target %s' %
                       (target_version, target_version))
    cmd1.extend(src1)
    debug = "-g:none"
    if coverage:
        debug = "-g:lines,vars,source"
    cmd2 = shlex.split('javac -d build/classes %s -source %s -target %s -cp build/lib' %
                       (debug, target_version, target_version))
    cmd2.extend(src2)
    os.makedirs("build/lib", exist_ok=True)
    os.makedirs("build/classes", exist_ok=True)
    self.announce("  %s" % " ".join(cmd1), level=distutils.log.INFO)
    subprocess.check_call(cmd1)
    self.announce("  %s" % " ".join(cmd2), level=distutils.log.INFO)
    subprocess.check_call(cmd2)
    cmd3 = shlex.split(
        'jar cvf build/lib/org.jpype.jar -C build/classes/ .')
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
