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
import warnings
import setupext
import os
import sys

# This handles all of the work to make our platform specific extension options.

platform_specific = {
    'include_dirs': [
        os.path.join('native', 'common', 'include'),
        os.path.join('native', 'python', 'include'),
        os.path.join('build', 'src'),
    ],
    'sources': [
        os.path.join('build', 'src', 'jp_thunk.cpp')
    ] + setupext.utils.find_sources(),
}

fallback_jni = os.path.join('native', 'jni_include')
# try to include JNI first from eventually given JAVA_HOME, then from distributed
java_home = os.getenv('JAVA_HOME', '')
found_jni = False
if os.path.exists(java_home) and sys.platform != "cygwin":
    platform_specific['include_dirs'] += [os.path.join(java_home, 'include')]

    # check if jni.h can be found
    for d in platform_specific['include_dirs']:
        if os.path.exists(os.path.join(d, 'jni.h')):
            print("Found native jni.h at %s" % d)
            found_jni = True
            break

    if not found_jni:
        warnings.warn('Falling back to provided JNI headers, since your provided'
                      ' JAVA_HOME "%s" does not provide jni.h' % java_home)

if not found_jni:
    platform_specific['include_dirs'] += [fallback_jni]

if sys.platform == 'win32':
    platform_specific['libraries'] = ['Advapi32']
    platform_specific['define_macros'] = [('WIN32', 1)]
    if sys.version > '3':
        platform_specific['extra_compile_args'] = [
            '/Zi', '/EHsc', '/std:c++14']
    else:
        platform_specific['extra_compile_args'] = ['/Zi', '/EHsc']
    platform_specific['extra_link_args'] = ['/DEBUG']
    jni_md_platform = 'win32'

elif sys.platform == 'darwin':
    platform_specific['libraries'] = ['dl']
    platform_specific['define_macros'] = [('MACOSX', 1)]
    platform_specific['extra_compile_args'] = ['-g0', '-std=c++11']
    jni_md_platform = 'darwin'

elif sys.platform.startswith('linux'):
    platform_specific['libraries'] = ['dl']
    platform_specific['extra_compile_args'] = ['-g0', '-std=c++11']
    jni_md_platform = 'linux'

elif sys.platform.startswith('aix7'):
    platform_specific['libraries'] = ['dl']
    platform_specific['extra_compile_args'] = ['-g3', '-std=c++11']
    jni_md_platform = 'aix7'

elif sys.platform.startswith('freebsd'):
    jni_md_platform = 'freebsd'

else:
    jni_md_platform = None
    warnings.warn("Your platform %s is not being handled explicitly."
                  " It may work or not!" % sys.platform, UserWarning)

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
