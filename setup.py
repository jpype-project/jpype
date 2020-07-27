#!/usr/bin/env python
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
import sys
import setupext
from setuptools import setup
from setuptools import Extension

jpypeLib = Extension(name='_jpype', **setupext.platform.platform_specific)

install_requires = ['typing_extensions ; python_version< "3.8"']

setup(
    name='JPype1',
    version='1.0.2',
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
    install_requires=install_requires,
    tests_require=['pytest'],
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
)
