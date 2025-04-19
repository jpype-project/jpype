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

from setuptools import Extension
from setuptools import setup

# Add our setupext package to the path, and import it.
sys.path.append(str(Path(__file__).parent))
import setupext

if '--android' in sys.argv:
    platform = 'android'
    sys.argv.remove('--android')
else:
    platform = sys.platform


jpypeLib = Extension(name='_jpype', **setupext.platform.Platform(
    include_dirs=[Path('native', 'common', 'include'),
                  Path('native', 'python', 'include')],
    sources=sorted(
        list(Path('native', 'common').glob('*.cpp')) +
        list(Path('native', 'python').glob('*.cpp')) 
    ),
    platform=platform,
))

p = [ i for i in Path("native", "jpype_module", "src", "main", "java").glob("**/*.java")]
javaSrc = [i for i in p if not "exclude" in str(i)]
jpypeJar = Extension(name="org.jpype",
                     sources=sorted(map(str, javaSrc)),
                     language="java",
                     libraries=["lib/asm-8.0.1.jar"]
                     )


setup(
    # Non-standard, and extension behaviour of setup() - project information
    # should be put in pyproject.toml wherever possible. See also:
    # https://setuptools.pypa.io/en/latest/userguide/pyproject_config.html#setuptools-specific-configuration
    platforms=[
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Operating System :: Unix',
        'Operating System :: MacOS',
    ],
    packages=['jpype', 'jpype._pyinstaller'],
    package_dir={'jpype': 'jpype', },
    package_data={'jpype': ['*.pyi']},
    cmdclass={
        'build_ext': setupext.build_ext.BuildExtCommand,
        'develop': setupext.develop.Develop,
        'test_java': setupext.test_java.TestJavaCommand,
        'sdist': setupext.sdist.BuildSourceDistribution,
    },
    zip_safe=False,
    ext_modules=[jpypeLib, jpypeJar],
)
