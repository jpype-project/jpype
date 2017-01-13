#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys
import codecs
import platform
from glob import glob
import warnings

from setuptools import setup
from setuptools import Extension
from setuptools.command.build_ext import build_ext


"""
this parameter is used to opt out numpy support in _jpype library
"""
if "--disable-numpy" in sys.argv:
    disabled_numpy = True
    sys.argv.remove("--disable-numpy")
else:
    disabled_numpy = False

class FeatureNotice(Warning):
    """ indicate notices about features """
    pass


def read_utf8(*parts):
    filename = os.path.join(os.path.dirname(__file__), *parts)
    return codecs.open(filename, encoding='utf-8').read()

def find_sources():
    cpp_files = []
    for dirpath, dirnames, filenames in os.walk('native'):
        for filename in filenames:
            if filename.endswith('.cpp') or filename.endswith('.c'):
                cpp_files.append(os.path.join(dirpath, filename))
    return cpp_files


platform_specific = {
    'include_dirs': [
        os.path.join('native', 'common', 'include'),
        os.path.join('native', 'python', 'include'),
    ],
    'sources': find_sources(),
}

fallback_jni = os.path.join('native', 'jni_include')
# try to include JNI first from eventually given JAVA_HOME, then from distributed
java_home = os.getenv('JAVA_HOME', '')
found_jni = False
if os.path.exists(java_home):
    platform_specific['include_dirs'] += [os.path.join(java_home, 'include')]

    # check if jni.h can be found
    for d in platform_specific['include_dirs']:
        if os.path.exists(os.path.join(d, 'jni.h')):
            found_jni = True
            break

    if not found_jni:
        import warnings
        warnings.warn('Falling back to provided JNI headers, since your provided'
                      ' JAVA_HOME "%s" does not provide jni.h' % java_home)
        platform_specific['include_dirs'] += [fallback_jni]

else:
    platform_specific['include_dirs'] += [fallback_jni]

if sys.platform == 'win32':
    platform_specific['libraries'] = ['Advapi32']
    platform_specific['define_macros'] = [('WIN32', 1)]
    jni_md_platform = 'win32'

elif sys.platform == 'darwin':
    platform_specific['libraries'] = ['dl']
    platform_specific['define_macros'] = [('MACOSX', 1)]
    jni_md_platform = 'darwin'

elif sys.platform.startswith('linux'):
    platform_specific['libraries'] = ['dl']
    jni_md_platform = 'linux'

elif sys.platform.startswith('freebsd'):
    jni_md_platform = 'freebsd'

else:
    warnings.warn("Your platform is not being handled explicitly."
                  " It may work or not!", UserWarning)

if found_jni:
    platform_specific['include_dirs'] += \
        [os.path.join(java_home, 'include', jni_md_platform)]

# include this stolen from FindJNI.cmake
"""
FIND_PATH(JAVA_INCLUDE_PATH2 jni_md.h
${JAVA_INCLUDE_PATH}
${JAVA_INCLUDE_PATH}/win32
${JAVA_INCLUDE_PATH}/linux
${JAVA_INCLUDE_PATH}/freebsd
${JAVA_INCLUDE_PATH}/solaris
${JAVA_INCLUDE_PATH}/hp-ux
${JAVA_INCLUDE_PATH}/alpha
)"""
 


jpypeLib = Extension(name='_jpype', **platform_specific)

class my_build_ext(build_ext):
    """
    Override some behavior in extension building:

    1. Numpy:
        If not opted out, try to use NumPy and define macro 'HAVE_NUMPY', so arrays
        returned from Java can be wrapped efficiently in a ndarray.
    2. handle compiler flags for different compilers via a dictionary.
    3. try to disable warning ‘-Wstrict-prototypes’ is valid for C/ObjC but not for C++
    """

    # extra compile args
    copt = {'msvc': ['/EHsc'],
            'unix' : ['-ggdb'],
            'mingw32' : [],
           }
    # extra link args
    lopt = {
            'msvc': [],
            'unix': [],
            'mingw32' : [],
           }

    def initialize_options(self, *args):
        """omit -Wstrict-prototypes from CFLAGS since its only valid for C code."""
        import distutils.sysconfig
        cfg_vars = distutils.sysconfig.get_config_vars()
        if 'CFLAGS' in cfg_vars:
            cfg_vars['CFLAGS'] = cfg_vars['CFLAGS'].replace('-Wstrict-prototypes', '')

        build_ext.initialize_options(self)

    def _set_cflags(self):
        # set compiler flags
        c = self.compiler.compiler_type
        if c in self.copt:
            for e in self.extensions:
                e.extra_compile_args = self.copt[ c ]
        if c in self.lopt:
            for e in self.extensions:
                e.extra_link_args = self.lopt[ c ]

    def build_extensions(self):
        self._set_cflags()
        # handle numpy
        if not disabled_numpy:
            try:
                import numpy
                jpypeLib.define_macros.append(('HAVE_NUMPY', 1))
                jpypeLib.include_dirs.append(numpy.get_include())
                warnings.warn("Turned ON Numpy support for fast Java array access",
                               FeatureNotice)
            except ImportError:
                pass
        else:
            warnings.warn("Turned OFF Numpy support for fast Java array access",
                          FeatureNotice)

        # has to be last call
        build_ext.build_extensions(self)

setup(
    name='JPype1',
    version='0.6.2',
    description='A Python to Java bridge.',
    long_description=(read_utf8('README.rst') + '\n\n' +
                      read_utf8('doc/CHANGELOG.rst') + '\n\n' +
                      read_utf8('AUTHORS.rst')),
    license='License :: OSI Approved :: Apache Software License',
    author='Steve Menard',
    author_email='devilwolf@users.sourceforge.net',
    maintainer='Luis Nell',
    maintainer_email='cooperate@originell.org',
    url='https://github.com/originell/jpype/',
    platforms=[
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows :: Windows 7',
        'Operating System :: Microsoft :: Windows :: Windows Vista',
        'Operating System :: POSIX :: Linux',
    ],
    classifiers=[
        'Programming Language :: Java',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
    ],
    packages=[
        'jpype', 'jpype.awt', 'jpype.awt.event', 'jpypex', 'jpypex.swing'],
    package_dir={
        'jpype': 'jpype',
        'jpypex': 'jpypex',
    },
    extras_require = {'numpy' : ['numpy>=1.6']},
    cmdclass={'build_ext': my_build_ext},
    zip_safe=False,
    ext_modules=[jpypeLib],
)
