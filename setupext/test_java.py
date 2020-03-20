# -*- coding: utf-8 -*-
import sys
import os
import subprocess
import distutils.cmd
import distutils.log


class TestJavaCommand(distutils.cmd.Command):
    """A custom command to create jar file during test."""

    description = 'run ant to make test harness'
    user_options = []

    def initialize_options(self):
        """Set default values for options."""
        pass

    def finalize_options(self):
        """Post-process options."""
        pass

    def run(self):
        """Run command."""
        if os.path.exists(os.path.join("test", "classes")):
            distutils.log.info("Skip building Java testbench")
            return
        distutils.log.info("Building Java testbench")
        buildXmlFile = os.path.join("test", "build.xml")
        command = [self.distribution.ant, '-f', buildXmlFile]
        if self.distribution.ant.endswith(".py"):
            command.insert(0, sys.executable)
        cmdStr = ' '.join(command)
        self.announce("  %s" % cmdStr, level=distutils.log.INFO)
        subprocess.check_call(command)
