# -*- coding: utf-8 -*-
import os
import subprocess
import distutils.cmd
import distutils.log
from distutils.errors import DistutilsPlatformError
from distutils.dir_util import copy_tree


class BuildJavaCommand(distutils.cmd.Command):
    """A custom command to create jar file during build."""

    description = 'run ant to make a jar'
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
            src = os.path.join('native','jars')
            dest = os.path.join('build','lib')
            if os.path.exists(src):
                distutils.log.info("Using Jar cache")
                copy_tree(src, dest)
                return

        distutils.log.info("Jar cache is missing, using --enable-build-jar to recreate it.")

        # build the jar
        buildDir = os.path.join("..", "build")
        buildXmlFile = os.path.join("native", "build.xml")
        command = [self.distribution.ant, '-Dbuild=%s' %
                   buildDir, '-f', buildXmlFile]
        cmdStr = ' '.join(command)
        self.announce("  %s" % cmdStr, level=distutils.log.INFO)
        try:
            subprocess.check_call(command)
        except subprocess.CalledProcessError as exc:
            distutils.log.error(exc.output)
            raise DistutilsPlatformError("Error executing {}".format(exc.cmd))

        # Coverage tool requires special placement of the source
        if self.distribution.enable_coverage:
            import shutil
            shutil.copyfile(os.path.join("build","lib","org.jpype.jar"), os.path.join("native", "org.jpype.jar"))


