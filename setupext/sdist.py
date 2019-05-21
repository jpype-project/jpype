# -*- coding: utf-8 -*-
import os
import sys
import warnings
import distutils
from distutils.dir_util import copy_tree, remove_tree
from setuptools.command.sdist import sdist
from setuptools import Extension

# Customization of the sdist
class BuildSourceDistribution(sdist):
    """
    Override some behavior on sdist

    Copy the build/lib to native/jars to remove ant/jdk dependency
    """
    def run(self):
        dest = os.path.join('native','jars')
        src = os.path.join('build','lib')
        if not os.path.exists(src):
            distutils.log.error("Jar source file is missing from build")
            raise distutils.errors.DistutilsPlatformError("Error copying jar file")
        copy_tree(src, dest)
        super(sdist, self).run()
        remove_tree(dest)

