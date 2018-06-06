# -*- coding: utf-8 -*-
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
    buildXmlFile = os.path.join("test","build.xml")
    command = [self.distribution.ant, '-f', buildXmlFile]
    cmdStr= ' '.join(command)
    self.announce("  %s"%cmdStr, level=distutils.log.INFO)
    subprocess.check_call(command)

