# -*- coding: utf-8 -*-
from __future__ import print_function
import os
from setuptools.command.build_ext import build_ext

# This setup target constructs a prototype Makefile suitable for compiling
# the _jpype extension module.  It is intended to help with development
# of the extension library on unix systems.
#
# To create a Makefile use
#    python setup.py build_makefile
#
# Then edit with the desired options


class FeatureNotice(Warning):
    """ indicate notices about features """


class Makefile(object):
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
        post = x[i0+2:]

        self.compile_command = command
        self.compile_pre = pre
        self.compile_post = post
        self.includes = includes
        self.sources.append(x[i1+1])

    def captureLink(self, x):
        self.link_command = x[0]
        x = x[1:]
        i = x.index("-o")
        self.library = x[i+1]
        del x[i]
        del x[i]
        self.objects = [i for i in x if i.endswith(".o")]
        self.link_options = [i for i in x if not i.endswith(".o")]
        u = self.objects[0].split("/")
        self.build_dir = "/".join(u[:2])

    def compile(self, *args, **kwargs):
        self.actual.spawn = self.captureCompile
        return self.actual.compile(*args, **kwargs)

    def link_shared_object(self, *args, **kwargs):
        self.actual.spawn = self.captureLink
        return self.actual.link_shared_object(*args, **kwargs)

    def detect_language(self, x):
        return self.actual.detect_language(x)

    def write(self):
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
build/src/jp_thunk.cpp: $(call rwildcard,native/java,*.java)
	python setup.py build_thunk

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
	$(LINK) $(LINKFLAGS) $(OBJS) -ldl -o $@


-include $(DEPFILES)
""", file=fd)


# Customization of the build_ext
class BuildMakefileCommand(build_ext):
    """
    Override some behavior in extension building:

    1. handle compiler flags for different compilers via a dictionary.
    2. try to disable warning -Wstrict-prototypes is valid for C/ObjC but not for C++
    """

    # extra compile args
    copt = {'msvc': [],
            'unix': ['-ggdb', ],
            'mingw32': [],
            }
    # extra link args
    lopt = {
        'msvc': [],
        'unix': [],
        'mingw32': [],
    }

    def initialize_options(self, *args):
        """omit -Wstrict-prototypes from CFLAGS since its only valid for C code."""
        import distutils.sysconfig
        cfg_vars = distutils.sysconfig.get_config_vars()
        replacement = {
            '-Wstrict-prototypes': '',
            '-Wimplicit-function-declaration': '',
        }
        replacement['-O3'] = '-O0'

        for k, v in cfg_vars.items():
            if not isinstance(v, str):
                continue
            if not k == "OPT" and not "FLAGS" in k:
                continue
            for r, t in replacement.items():
                if v.find(r) != -1:
                    v = v.replace(r, t)
                    cfg_vars[k] = v
        build_ext.initialize_options(self)

    def _set_cflags(self):
        # set compiler flags
        c = self.compiler.compiler_type
        if c == 'unix' and self.distribution.enable_coverage:
            self.extensions[0].extra_compile_args.extend(
                ['-O0', '--coverage', '-ftest-coverage'])
            self.extensions[0].extra_link_args.extend(['--coverage'])
        if c in self.copt:
            for e in self.extensions:
                e.extra_compile_args.extend(self.copt[c])
        if c in self.lopt:
            for e in self.extensions:
                e.extra_link_args.extend(self.lopt[c])

    def build_extensions(self):
        # We need to create the thunk code
        self.run_command("build_java")
        self.run_command("build_thunk")

        jpypeLib = self.extensions[0]
        tracing = self.distribution.enable_tracing
        self._set_cflags()
        if tracing:
            jpypeLib.define_macros.append(('JP_TRACING_ENABLE', 1))
        coverage = self.distribution.enable_coverage
        if coverage:
            jpypeLib.define_macros.append(('JP_INSTRUMENTATION', 1))

        self.compiler = Makefile(self.compiler)

        # has to be last call
        build_ext.build_extensions(self)

        self.compiler.write()

    def __init__(self, *args):
        build_ext.__init__(self, *args)
