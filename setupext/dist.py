# -*- coding: utf-8 -*-
# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
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
