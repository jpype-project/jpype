# -*- coding: utf-8 -*-
import os
import subprocess
import distutils.cmd
import distutils.log
from distutils.errors import DistutilsPlatformError

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
    buildDir = os.path.join("..","build","lib")
    buildXmlFile = os.path.join("native","build.xml")
    command = [self.distribution.ant, '-Dbuild=%s'%buildDir, '-f', buildXmlFile]
    cmdStr= ' '.join(command)
    self.announce("  %s"%cmdStr, level=distutils.log.INFO)
    try:
        subprocess.check_call(command)
    except subprocess.CalledProcessError as exc:
        distutils.log.error(exc.output)
        raise DistutilsPlatformError("Error executing {}".format(exc.cmd))

