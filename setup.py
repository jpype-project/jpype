#!/usr/bin/env python
# -*- coding: utf-8 -*-
import setupext
from setuptools import setup
from setuptools import Extension

jpypeLib = Extension(name='_jpype', **setupext.platform.platform_specific)

setup(
    name='JPype1',
    version='0.7.4',
    description='A Python to Java bridge.',
    long_description=open('README.rst').read(),
    license='License :: OSI Approved :: Apache Software License',
    author='Steve Menard',
    author_email='devilwolf@users.sourceforge.net',
    maintainer='Luis Nell',
    maintainer_email='cooperate@originell.org',
    url='https://github.com/jpype-project/jpype',
    platforms=[
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Operating System :: Unix',
        'Operating System :: MacOS',
    ],
    classifiers=[
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Topic :: Software Development',
        'Topic :: Scientific/Engineering',
    ],
    packages=[
        'jpype'],
    package_dir={
        'jpype': 'jpype',
    },
    tests_require=['pytest', 'mock', 'unittest2'],
    cmdclass={
        'build_java': setupext.build_java.BuildJavaCommand,
        'build_thunk': setupext.build_thunk.BuildThunkCommand,
        'build_ext': setupext.build_ext.BuildExtCommand,
        'build_makefile': setupext.build_makefile.BuildMakefileCommand,
        'test_java': setupext.test_java.TestJavaCommand,
        'sdist': setupext.sdist.BuildSourceDistribution,
        'test': setupext.pytester.PyTest,
    },
    zip_safe=False,
    ext_modules=[jpypeLib],
    distclass=setupext.dist.Distribution,
    use_scm_version=True,
)
