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

from setuptools.command.develop import develop as develop_cmd


class Develop(develop_cmd):
    user_options = develop_cmd.user_options + [
        ('enable-build-jar', None, 'Build the java jar portion'),
        ('enable-tracing', None, 'Set for tracing for debugging'),
        ('enable-coverage', None, 'Instrument c++ code for code coverage measuring'),
    ]

    def initialize_options(self, *args):
        self.enable_tracing = False
        self.enable_build_jar = False
        self.enable_coverage = False
        super().initialize_options()

    def reinitialize_command(self, command, reinit_subcommands=0, **kw):
        cmd = super().reinitialize_command(
            command, reinit_subcommands=reinit_subcommands, **kw)
        build_ext_command = self.distribution.get_command_obj("build_ext")
        build_ext_command.enable_tracing = self.enable_tracing
        build_ext_command.enable_build_jar = self.enable_build_jar
        build_ext_command.enable_coverage = self.enable_coverage
        return cmd
