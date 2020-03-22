# -*- coding: utf-8 -*-
import os
import distutils
from distutils.dir_util import copy_tree, remove_tree
from setuptools.command.sdist import sdist

# Customization of the sdist


class BuildSourceDistribution(sdist):
    """
    Override some behavior on sdist

    Copy the build/lib to native/jars to remove javac/jdk dependency
    """

    def run(self):
        # We need to build a jar cache for the source distribution
        self.run_command("build_java")
        self.run_command("test_java")
        dest = os.path.join('native', 'jars')
        src = os.path.join('build', 'lib')
        if not os.path.exists(src):
            distutils.log.error("Jar source file is missing from build")
            raise distutils.errors.DistutilsPlatformError(
                "Error copying jar file")
        copy_tree(src, dest)

        # Collect the sources
        sdist.run(self)

        # Clean up the jar cache after sdist
        remove_tree(dest)
