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
import os
import setuptools._distutils.log as log
from setuptools._distutils import sysconfig
from setuptools._distutils import ccompiler
from setuptools.command.build_clib import build_clib

class BuildExeCommand(build_clib):
    user_options = build_clib.user_options + [
    ]

    def initialize_options(self, *args):
        super().initialize_options()

    def finalize_options(self):
        self.set_undefined_options(
            'build',
            ('build_temp', 'build_clib'),
            ('build_temp', 'build_temp'),
            ('compiler', 'compiler'),
            ('debug', 'debug'),
            ('force', 'force'),
        )

        self.executables = self.distribution.executables
        if self.executables:
            self.check_executables_list(self.executables)

        py_include = sysconfig.get_python_inc()
        plat_py_include = sysconfig.get_python_inc(plat_specific=1)
#        if self.include_dirs is None:
#            self.include_dirs = self.distribution.include_dirs or []
#        if isinstance(self.include_dirs, str):
#            self.include_dirs = self.include_dirs.split(os.pathsep)
#
#        # Put the Python "system" include dir at the end, so that
#        # any local include dirs take precedence.

#        if self.libraries is None:
#            self.libraries = self.distribution.libraries or []
#        self.libraries.append(sysconfig.get_config_vars()['LDLIBRARY'])

        self.compiler = ccompiler.new_compiler(
            compiler=self.compiler, dry_run=self.dry_run, force=self.force
        )

        self.compiler.add_library(sysconfig.get_config_vars()["BLDLIBRARY"][2:])
        self.compiler.include_dirs.extend(py_include.split(os.path.pathsep))
        if plat_py_include != py_include:
            self.compiler.include_dirs.extend(plat_py_include.split(os.path.pathsep))
        self.compiler.add_library_dir(sysconfig.get_config_vars()["LIBPL"])

    def run(self):
        # This is not exposed at all 
        from setuptools._distutils import ccompiler

       #customize_compiler(self.compiler)

        if self.include_dirs is not None:
            self.compiler.set_include_dirs(self.include_dirs)
        if self.define is not None:
            # 'define' option is a list of (name,value) tuples
            for name, value in self.define:
                self.compiler.define_macro(name, value)
        if self.undef is not None:
            for macro in self.undef:
                self.compiler.undefine_macro(macro)

        for executable in self.executables:
            self.build_executable(executable)

    def build_executable(self, executable):
            sources = executable.sources
            if sources is None or not isinstance(sources, (list, tuple)):
                raise DistutilsSetupError(
                    "in 'executables' option (executable '%s'), "
                    "'sources' must be present and must be "
                    "a list of source filenames" % executable.name
                )
            sources = list(sources)

            log.info("building '%s' executable", executable.name)

            macros = executable.define_macros[:]
            for undef in executable.undef_macros:
                macros.append((undef,))
            objects = self.compiler.compile(
                sources,
                output_dir=self.build_temp,
                macros=macros,
                include_dirs=executable.include_dirs,
                debug=self.debug,
                depends=executable.depends
            )
            name = executable.name
            if self.compiler.exe_extension is not None:
                name = name + self.compile.exe_extension

            self.compiler.link(ccompiler.CCompiler.EXECUTABLE, objects, name, library_dirs=executable.library_dirs, libraries=executable.libraries, debug=self.debug)

    def check_executables_list(self, executables):
        """Ensure that the list of libraries is valid.

        `library` is presumably provided as a command option 'libraries'.
        This method checks that it is a list of 2-tuples, where the tuples
        are (library_name, build_info_dict).

        Raise DistutilsSetupError if the structure is invalid anywhere;
        just returns otherwise.
        """
        if not isinstance(executables, list):
            raise DistutilsSetupError("'executables' option must be a list of Executables")


    
