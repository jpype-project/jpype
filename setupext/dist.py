# -*- coding: utf-8 -*-
from setuptools.dist import Distribution as _Distribution

# Add a new global option to the setup.py script.


class Distribution(_Distribution):
    global_options = [
        ('enable-build-jar', None, 'Build the java jar portion'),
        ('enable-tracing', None, 'Set for tracing for debugging'),
        ('enable-coverage', None, 'Instrument c++ code for code coverage measuring'),

    ] + _Distribution.global_options

    def parse_command_line(self):
        self.enable_tracing = False
        self.enable_build_jar = False
        self.enable_coverage = False
        return _Distribution.parse_command_line(self)
