# -*- coding: utf-8 -*-
from setuptools.dist import Distribution as _Distribution

# Add a new global option to the setup.py script.


class Distribution(_Distribution):
    global_options = [
        ('enable-build-jar', None, 'Build the java jar portion'),
        ('enable-tracing', None, 'Set for tracing for debugging'),
        ('ant=', None, 'Set the ant executable (default ant)', 1),
        ('disable-numpy', None, 'Do not compile with numpy extenstions')
    ] + _Distribution.global_options

    def parse_command_line(self):
        self.disable_numpy = False
        self.ant = "ant"
        self.enable_tracing = False
        self.enable_build_jar = False
        return _Distribution.parse_command_line(self)
