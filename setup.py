#!/usr/bin/env python
# -*- coding: utf-8 -*-
import warnings
import setupext
from setuptools import setup
from setuptools import Extension

jpypeLib = Extension(name='_jpype', **setupext.platform.platform_specific)

setup(
    name='JPype1',
    version='0.7.0',
    description='A Python to Java bridge.',
    long_description=(setupext.utils.read_utf8(__file__, 'README.rst') + '\n\n' +
                      setupext.utils.read_utf8(__file__, 'doc/CHANGELOG.rst') + '\n\n' +
                      setupext.utils.read_utf8(__file__, 'AUTHORS.rst')),
    license='License :: OSI Approved :: Apache Software License',
    author='Steve Menard',
    author_email='devilwolf@users.sourceforge.net',
    maintainer='Luis Nell',
    maintainer_email='cooperate@originell.org',
    url='https://github.com/jpype-project/jpype',
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
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
    ],
    packages=[
        'jpype', 'jpypex', 'jpypex.swing'],
    package_dir={
        'jpype': 'jpype',
        'jpypex': 'jpypex',
    },
    extras_require={'numpy': ['numpy>=1.6']},
    cmdclass={
        'build_java': setupext.build_java.BuildJavaCommand,
        'build_thunk': setupext.build_thunk.BuildThunkCommand,
        'build_ext': setupext.build_ext.BuildExtCommand,
        'test_java': setupext.test_java.TestJavaCommand,
        'sdist': setupext.sdist.BuildSourceDistribution,
    },
    zip_safe=False,
    ext_modules=[jpypeLib],
    distclass=setupext.dist.Distribution,
    use_scm_version = True,
)
