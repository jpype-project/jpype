# -*- coding: utf-8 -*-
import os
import sys
import warnings
from setuptools.command.build_ext import build_ext
from setuptools import Extension


class FeatureNotice(Warning):
    """ indicate notices about features """
    pass


# Customization of the build_ext
class BuildExtCommand(build_ext):
    """
    Override some behavior in extension building:

    1. Numpy:
        If not opted out, try to use NumPy and define macro 'HAVE_NUMPY', so arrays
        returned from Java can be wrapped efficiently in a ndarray.
    2. handle compiler flags for different compilers via a dictionary.
    3. try to disable warning -Wstrict-prototypes is valid for C/ObjC but not for C++
    """

    # extra compile args
    copt = {'msvc': [],
            'unix': ['-ggdb'],
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
#        if 'CFLAGS' in cfg_vars:
#            cfg_vars['CFLAGS'] = cfg_vars['CFLAGS'].replace('-Wstrict-prototypes', '')
        replacement = {
            '-Wstrict-prototypes': '',
            '-Wimplicit-function-declaration': '',
        }
        tracing = self.distribution.enable_tracing
        if tracing:
            replacement['-O3'] = '-O0'

        for k, v in cfg_vars.items():
            for r, t in replacement.items():
                if isinstance(v, str) and k.find("FLAGS") != -1 and v.find(r) != -1:
                    v = v.replace(r, t)
                    cfg_vars[k] = v
        build_ext.initialize_options(self)

    def _set_cflags(self):
        # set compiler flags
        c = self.compiler.compiler_type
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
        disable_numpy = self.distribution.disable_numpy
        tracing = self.distribution.enable_tracing
        self._set_cflags()
        # handle numpy
        if not disable_numpy:
            try:
                import numpy
                jpypeLib.include_dirs.append(numpy.get_include())
                jpypeLib.define_macros.append(('HAVE_NUMPY', 1))
                warnings.warn("Turned ON Numpy support for fast Java array access",
                              FeatureNotice)
            except ImportError:
                pass
        else:
            warnings.warn("Turned OFF Numpy support for fast Java array access",
                          FeatureNotice)
        if tracing:
            jpypeLib.define_macros.append(('JP_TRACING_ENABLE', 1))

        # has to be last call
        build_ext.build_extensions(self)
