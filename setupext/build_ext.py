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
from setuptools.command.build_ext import build_ext
import sys
import subprocess
import distutils.cmd
import distutils.log
from distutils.errors import DistutilsPlatformError
from distutils.dir_util import copy_tree
import glob
import re
import shlex
import shutil
import sysconfig


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


class Makefile(object):
    compiler_type = "unix"

    def __init__(self, actual):
        self.actual = actual
        self.compile_command = None
        self.compile_pre = None
        self.compile_post = None
        self.objects = []
        self.sources = []

    def captureCompile(self, x):
        command = x[0]
        x = x[1:]
        includes = [i for i in x if i.startswith("-I")]
        x = [i for i in x if not i.startswith("-I")]
        i0 = None
        i1 = None
        for i, v in enumerate(x):
            if v == '-c':
                i1 = i
            elif v == '-o':
                i0 = i
        pre = set(x[:i1])
        post = x[i0 + 2:]

        self.compile_command = command
        self.compile_pre = pre
        self.compile_post = post
        self.includes = includes
        self.sources.append(x[i1 + 1])

    def captureLink(self, x):
        self.link_command = x[0]
        x = x[1:]
        i = x.index("-o")
        self.library = x[i + 1]
        del x[i]
        del x[i]
        self.objects = [i for i in x if i.endswith(".o")]
        self.link_options = [i for i in x if not i.endswith(".o")]
        u = self.objects[0].split("/")
        self.build_dir = "/".join(u[:2])

    def compile(self, *args, **kwargs):
        self.actual.spawn = self.captureCompile
        rc = self.actual.compile(*args, **kwargs)
        return rc

    def _need_link(self, *args):
        return True

    def link_shared_object(self, *args, **kwargs):
        self.actual._need_link = self._need_link
        self.actual.spawn = self.captureLink
        rc = self.actual.link_shared_object(*args, **kwargs)
        self.write()
        return rc

    def detect_language(self, x):
        return self.actual.detect_language(x)

    def write(self):
        print("Write makefile")
        library = os.path.basename(self.library)
        link_command = self.link_command
        compile_command = self.compile_command
        compile_pre = " ".join(list(self.compile_pre))
        compile_post = " ".join(list(self.compile_post))
        build = self.build_dir
        link_flags = " ".join(self.link_options)
        includes = " ".join(self.includes)
        sources = " \\\n     ".join(self.sources)
        with open("Makefile", "w") as fd:
            print("LIB = %s" % library, file=fd)
            print("CC = %s" % compile_command, file=fd)
            print("LINK = %s" % link_command, file=fd)
            print("CFLAGS = %s %s" % (compile_pre, compile_post), file=fd)
            print("INCLUDES = %s" % includes, file=fd)
            print("BUILD = %s" % build, file=fd)
            print("LINKFLAGS = %s" % link_flags, file=fd)
            print("SRCS = %s" % sources, file=fd)
            print("""
all: $(LIB)

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
#build/src/jp_thunk.cpp: $(call rwildcard,native/java,*.java)
#	python setup.py build_thunk

DEPDIR = build/deps
$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.cpp=$(DEPDIR)/%.d)

deps: $(DEPFILES)

%/:
	echo $@

$(DEPDIR)/%.d: %.cpp 
	mkdir -p $(dir $@)
	$(CC) $(INCLUDES) -MT $(patsubst $(DEPDIR)%,'$$(BUILD)%',$(patsubst %.d,%.o,$@)) -MM $< -o $@

OBJS = $(addprefix $(BUILD)/, $(SRCS:.cpp=.o))


$(BUILD)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


$(LIB): $(OBJS)
	$(LINK) $(OBJS) $(LINKFLAGS) -o $@


-include $(DEPFILES)
""", file=fd)


# Customization of the build_ext
class BuildExtCommand(build_ext):
    """
    Override some behavior in extension building:

    1. handle compiler flags for different compilers via a dictionary.
    2. try to disable warning -Wstrict-prototypes is valid for C/ObjC but not for C++
    """

    user_options = build_ext.user_options + [
        ('android', None, 'configure for android'),
        ('makefile', None, 'Build a makefile for extensions'),
        ('jar', None, 'Build the jar only'),
    ]

    def initialize_options(self, *args):
        """omit -Wstrict-prototypes from CFLAGS since its only valid for C code."""
        self.android = False
        self.makefile = False
        self.jar = False
        import distutils.sysconfig
        cfg_vars = distutils.sysconfig.get_config_vars()
        replacement = {
            '-Wstrict-prototypes': '',
            '-Wimplicit-function-declaration': '',
        }
        tracing = self.distribution.enable_tracing

        # Arguments to remove so we set debugging and optimization level
        remove_args = ['-O0', '-O1', '-O2', '-O3', '-g']

        for k, v in cfg_vars.items():
            if not isinstance(v, str):
                continue
            if not k == "OPT" and not "FLAGS" in k:
                continue

            args = v.split()
            for r in remove_args:
                args = list(filter((r).__ne__, args))

            cfg_vars[k] = " ".join(args)
        super().initialize_options()

    def _set_cflags(self):
        # set compiler flags
        c = self.compiler.compiler_type
        jpypeLib = [i for i in self.extensions if i.name == '_jpype'][0]
        if c == 'unix' and self.distribution.enable_coverage:
            jpypeLib.extra_compile_args.extend(
                ['-ggdb', '--coverage', '-ftest-coverage'])
            jpypeLib.extra_compile_args = ['-O0' if x == '-O2' else x for x in jpypeLib.extra_compile_args]
            jpypeLib.extra_link_args.extend(['--coverage'])
        if c == 'unix' and self.distribution.enable_tracing:
            jpypeLib.extra_compile_args = ['-O0' if x == '-O2' else x for x in jpypeLib.extra_compile_args]

    def build_extensions(self):
        if self.makefile:
            self.compiler = Makefile(self.compiler)
            self.force = True

        jpypeLib = [i for i in self.extensions if i.name == '_jpype'][0]
        tracing = self.distribution.enable_tracing
        self._set_cflags()
        if tracing:
            jpypeLib.define_macros.append(('JP_TRACING_ENABLE', 1))
        coverage = self.distribution.enable_coverage
        if coverage:
            jpypeLib.define_macros.append(('JP_INSTRUMENTATION', 1))

        # has to be last call
        print("Call build extensions")
        super().build_extensions()

    def build_extension(self, ext):
        if ext.language == "java":
            return self.build_java_ext(ext)
        if self.jar:
            return
        print("Call build ext")
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
        java = self.distribution.enable_build_jar

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

        coverage = self.distribution.enable_coverage

        target_version = "1.8"
        # build the jar
        try:
            dirname = os.path.dirname(self.get_ext_fullpath("JAVA"))
            jarFile = os.path.join(dirname, ext.name + ".jar")
            build_dir = os.path.join(self.build_temp, ext.name, "classes")
            os.makedirs(build_dir, exist_ok=True)
            os.makedirs(dirname, exist_ok=True)
            cmd1 = shlex.split('%s -cp "%s" -d "%s" -g:none -source %s -target %s' %
                               (javac, classpath, build_dir, target_version, target_version))
            cmd1.extend(ext.sources)
            debug = "-g:none"
            if coverage:
                debug = "-g:lines,vars,source"
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
