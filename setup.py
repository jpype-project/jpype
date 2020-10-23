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
from pathlib import Path
from setuptools import setup
from setuptools import Extension
import glob

if sys.version_info[0] < 3 and sys.version_info[1] < 5:
    raise RuntimeError("JPype requires Python 3.5 or later")

import setupext


if '--android' in sys.argv:
    platform = 'android'
    sys.argv.remove('--android')
else:
    platform = sys.platform


jpypeLib = Extension(name='_jpype', **setupext.platform.Platform(
    include_dirs=[Path('native', 'common', 'include'),
                  Path('native', 'python', 'include'),
                  Path('native', 'embedded', 'include')],
    sources=[Path('native', 'common', '*.cpp'),
             Path('native', 'python', '*.cpp'),
             Path('native', 'embedded', '*.cpp')], platform=platform,
))
jpypeJar = Extension(name="org.jpype",
                     sources=glob.glob(str(Path("native", "java", "**", "*.java")), recursive=True),
                     language="java",
                     libraries=["lib/asm-8.0.1.jar"]
                     )


setup(
    name='JPype1',
    version='1.1.2',
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
    packages=['jpype'],
    package_dir={'jpype': 'jpype', },
    install_requires=['typing_extensions ; python_version< "3.8"'],
    tests_require=['pytest'],
    cmdclass={
        'build_ext': setupext.build_ext.BuildExtCommand,
        'test_java': setupext.test_java.TestJavaCommand,
        'sdist': setupext.sdist.BuildSourceDistribution,
        'test': setupext.pytester.PyTest,
    },
    zip_safe=False,
    ext_modules=[jpypeJar, jpypeLib, ],
    distclass=setupext.dist.Distribution,
)

