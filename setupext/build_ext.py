# -*- coding: utf-8 -*-
from setuptools.command.build_ext import build_ext


class FeatureNotice(Warning):
    """ indicate notices about features """


# Customization of the build_ext
class BuildExtCommand(build_ext):
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
        tracing = self.distribution.enable_tracing
        if tracing:
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

        # has to be last call
        build_ext.build_extensions(self)
