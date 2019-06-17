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
    long_description=open('README.rst').read(),
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
        'Programming Language :: Java :: 8',
        'Programming Language :: Java :: 9',
        'Programming Language :: Java :: 11',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
    ],
    packages=[
        'jpype'],
    package_dir={
        'jpype': 'jpype',
    },
    setup_requires=['setuptools_scm'],
    tests_require=['pytest', 'mock', 'unittest2'],
    extras_require={'numpy': ['numpy>=1.6']},
    cmdclass={
        'build_java': setupext.build_java.BuildJavaCommand,
        'build_thunk': setupext.build_thunk.BuildThunkCommand,
        'build_ext': setupext.build_ext.BuildExtCommand,
        'test_java': setupext.test_java.TestJavaCommand,
        'sdist': setupext.sdist.BuildSourceDistribution,
        'test': setupext.pytester.PyTest,
    },
    zip_safe=False,
    ext_modules=[jpypeLib],
    distclass=setupext.dist.Distribution,
    use_scm_version = True,
)
