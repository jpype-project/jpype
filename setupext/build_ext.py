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

import distutils.cmd
import distutils.log
import glob
import os
import shlex
import shutil
import subprocess
from distutils.dir_util import copy_tree
from distutils.errors import DistutilsPlatformError
from setuptools._distutils import sysconfig
from setuptools._distutils import ccompiler
from setuptools.command.build_ext import build_ext

from setuptools import Extension
from .makefile import Makefile
class Executable(Extension):
    """Just like a regular Extension, but built as a executable instead"""

class Jar(Extension):
    """Just like a regular Extension, but built as a jar instead"""


# This setup option constructs a prototype Makefile suitable for compiling
# the _jpype extension module.  It is intended to help with development
# of the extension library on unix systems.  This works only on unix systems.
#
# To create a Makefile use
#    python setup.py build_ext --makefile
#
# Then edit with the desired options


class FeatureNotice(Warning):
    """ indicate notices about features """

# Customization of the build_ext
class BuildExtCommand(build_ext):
    """
    Override some behavior in extension building:

    1. handle compiler flags for different compilers via a dictionary.
    2. try to disable warning -Wstrict-prototypes is valid for C/ObjC but not for C++
    """

    user_options = build_ext.user_options + [
        ('enable-build-jar', None, 'Build the java jar portion'),
        ('enable-tracing', None, 'Set for tracing for debugging'),
        ('enable-coverage', None, 'Instrument c++ code for code coverage measuring'),

        ('android', None, 'configure for android'),
        ('makefile', None, 'Build a makefile for extensions'),
        ('jar', None, 'Build the jar only'),
        ('exe', None, 'Build the exe only'),
    ]

    def initialize_options(self, *args):
        """omit -Wstrict-prototypes from CFLAGS since its only valid for C code."""
        self.enable_tracing = False
        self.enable_build_jar = False
        self.enable_coverage = False

        self.android = False
        self.makefile = False
        self.jar = False
        self.exe = False
        import distutils.sysconfig
        cfg_vars = distutils.sysconfig.get_config_vars()

        # Arguments to remove so we set debugging and optimization level
        remove_args = ['-O0', '-O1', '-O2', '-O3', '-g']

        for k, v in cfg_vars.items():
            if not isinstance(v, str):
                continue
            if not k == "OPT" and "FLAGS" not in k:
                continue

            args = v.split()
            args = [arg for arg in args if arg not in remove_args]

            cfg_vars[k] = " ".join(args)
        super().initialize_options()

    def _set_cflags(self):
        # set compiler flags
        c = self.compiler.compiler_type
        jpypeLib = [i for i in self.extensions if i.name == '_jpype'][0]
        if c == 'unix' and self.enable_coverage:
            jpypeLib.extra_compile_args.extend(
                ['-ggdb', '--coverage', '-ftest-coverage'])
            jpypeLib.extra_compile_args = ['-O0' if x == '-O2' else x for x in jpypeLib.extra_compile_args]
            jpypeLib.extra_link_args.extend(['--coverage'])
        if c == 'unix' and self.enable_tracing:
            jpypeLib.extra_compile_args = ['-O0' if x == '-O2' else x for x in jpypeLib.extra_compile_args]

    def build_extensions(self):
        if self.makefile:
            self.compiler = Makefile(self.compiler)
            self.force = True

        jpypeLib = [i for i in self.extensions if i.name == '_jpype'][0]
        self._set_cflags()
        if self.enable_tracing:
            jpypeLib.define_macros.append(('JP_TRACING_ENABLE', 1))
        if self.enable_coverage:
            jpypeLib.define_macros.append(('JP_INSTRUMENTATION', 1))

        super().build_extensions()

    def build_extension(self, ext):
        if isinstance(ext, Jar) and not self.exe:
            return self.build_java_ext(ext)
        if isinstance(ext, Executable) and not self.jar:
            return self.build_executable(ext)
        if isinstance(ext, Extension) and not self.jar and not self.exe:
            return super().build_extension(ext)

    def copy_extensions_to_source(self):
        build_py = self.get_finalized_command('build_py')
        for ext in self.extensions:
            if ext.language == "java":
                fullname = self.get_ext_fullname("JAVA")
                filename = ext.name + ".jar"
            else:
                fullname = self.get_ext_fullname(ext.name)
                filename = self.get_ext_filename(fullname)
            modpath = fullname.split('.')
            package = '.'.join(modpath[:-1])
            package_dir = build_py.get_package_dir(package)
            dest_filename = os.path.join(package_dir,
                                         os.path.basename(filename))
            src_filename = os.path.join(self.build_lib, filename)
            # Always copy, even if source is older than destination, to ensure
            # that the right extensions for the current Python/platform are
            # used.
            distutils.file_util.copy_file(
                src_filename, dest_filename, verbose=self.verbose,
                dry_run=self.dry_run
            )
            if ext._needs_stub:
                self.write_stub(package_dir or os.curdir, ext, True)

    def build_java_ext(self, ext):
        """Run command."""
        java = self.enable_build_jar

        javac = "javac"
        try:
            if os.path.exists(os.path.join(os.environ['JAVA_HOME'], 'bin', 'javac')):
                javac = '"%s"' % os.path.join(os.environ['JAVA_HOME'], 'bin', 'javac')
        except KeyError:
            pass
        jar = "jar"
        try:
            if os.path.exists(os.path.join(os.environ['JAVA_HOME'], 'bin', 'jar')):
                jar = '"%s"' % os.path.join(os.environ['JAVA_HOME'], 'bin', 'jar')
        except KeyError:
            pass
        # Try to use the cache if we are not requested build
        if not java:
            src = os.path.join('native', 'jars')
            dest = os.path.dirname(self.get_ext_fullpath("JAVA"))
            if os.path.exists(src):
                distutils.log.info("Using Jar cache")
                copy_tree(src, dest)
                return

        classpath = "."
        if ext.libraries:
            classpath = os.path.pathsep.join(ext.libraries)

        distutils.log.info(
            "Jar cache is missing, using --enable-build-jar to recreate it.")

        target_version = "1.8"
        # build the jar
        try:
            dirname = os.path.dirname(self.get_ext_fullpath("JAVA"))
            jarFile = os.path.join(dirname, ext.name + ".jar")
            build_dir = os.path.join(self.build_temp, ext.name, "classes")
            os.makedirs(build_dir, exist_ok=True)
            os.makedirs(dirname, exist_ok=True)
            cmd1 = shlex.split('%s -cp "%s" -d "%s" -g:none -source %s -target %s -encoding UTF-8' %
                               (javac, classpath, build_dir, target_version, target_version))
            cmd1.extend(ext.sources)

            os.makedirs("build/classes", exist_ok=True)
            self.announce("  %s" % " ".join(cmd1), level=distutils.log.INFO)
            subprocess.check_call(cmd1)
            try:
                for file in glob.iglob("native/java/**/*.*", recursive=True):
                    if file.endswith(".java") or os.path.isdir(file):
                        continue
                    p = os.path.join(build_dir, os.path.relpath(file, "native/java"))
                    print("Copy file", file, p)
                    shutil.copyfile(file, p)
            except Exception as ex:
                print("FAIL", ex)
                pass
            cmd3 = shlex.split(
                '%s cvf "%s" -C "%s" .' % (jar, jarFile, build_dir))
            self.announce("  %s" % " ".join(cmd3), level=distutils.log.INFO)
            subprocess.check_call(cmd3)

        except subprocess.CalledProcessError as exc:
            distutils.log.error(exc.output)
            raise DistutilsPlatformError("Error executing {}".format(exc.cmd))

    def build_executable(self, executable):
        sources = executable.sources
        if sources is None or not isinstance(sources, (list, tuple)):
            raise DistutilsSetupError(
                "in 'executables' option (executable '%s'), "
                "'sources' must be present and must be "
                "a list of source filenames" % executable.name
            )
        sources = list(sources)

        distutils.log.info("building '%s' executable", executable.name)

        py_include = sysconfig.get_python_inc()
        plat_py_include = sysconfig.get_python_inc(plat_specific=1)

        compiler = self.compiler
#ccompiler.new_compiler(
#            compiler=self.compiler, dry_run=self.dry_run, force=self.force
#        )

        compiler.add_library(sysconfig.get_config_vars()["BLDLIBRARY"][2:])
        compiler.include_dirs.extend(py_include.split(os.path.pathsep))
        if plat_py_include != py_include:
            compiler.include_dirs.extend(plat_py_include.split(os.path.pathsep))
        compiler.add_library_dir(sysconfig.get_config_vars()["LIBPL"])


        macros = executable.define_macros[:]
        for undef in executable.undef_macros:
            macros.append((undef,))
        objects = compiler.compile(
            sources,
            output_dir=self.build_temp,
            macros=macros,
            include_dirs=executable.include_dirs,
            debug=self.debug,
            depends=executable.depends
        )
        name = executable.name
        if compiler.exe_extension is not None:
            name = name + self.compile.exe_extension

        compiler.link(ccompiler.CCompiler.EXECUTABLE, objects, name, library_dirs=executable.library_dirs, libraries=executable.libraries, debug=self.debug, target_lang="c++")


