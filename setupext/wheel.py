# -*- coding: utf-8 -*-
import os
from setuptools import Command
from distutils import log
try:
    from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

    class WheelCommand(_bdist_wheel):

        def run(self):
            version = None
            try:
                import numpy
                version = numpy.__version__
            except:
                pass

            # Override the requirements to match the version of numpy used
            if not version:
                self.distribution.extras_require={}
            else:
                self.distribution.extras_require['numpy']="numpy>=%s"%version
            print("Setting numpy requirement to %s"%version)
            _bdist_wheel.run(self)

except ImportException:
    class WheelCommand(Command):
        pass



