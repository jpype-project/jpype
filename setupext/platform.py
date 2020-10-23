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
import setupext
import os
import sys
import sysconfig
import distutils.log

# This handles all of the work to make our platform specific extension options.


def Platform(include_dirs=None, sources=None, platform=sys.platform):
    if include_dirs is None:
        include_dirs = []
    if sources is None:
        sources = []

    platform_specific = {
        'include_dirs': include_dirs,
        'sources': setupext.utils.find_sources(sources),
    }

    fallback_jni = os.path.join('native', 'jni_include')
    # try to include JNI first from eventually given JAVA_HOME, then from distributed
    java_home = os.getenv('JAVA_HOME', '')
    found_jni = False
    if os.path.exists(java_home):
        platform_specific['include_dirs'] += [os.path.join(java_home, 'include')]

        # check if jni.h can be found
        for d in platform_specific['include_dirs']:
            if os.path.exists(os.path.join(str(d), 'jni.h')):
                distutils.log.info("Found native jni.h at %s", d)
                found_jni = True
                break

            if not found_jni:
                distutils.log.warn('Falling back to provided JNI headers, since your provided'
                                   ' JAVA_HOME "%s" does not provide jni.h', java_home)

    if not found_jni:
        platform_specific['include_dirs'] += [fallback_jni]

    platform_specific['extra_link_args'] = []
    distutils.log.info("Configure platform to", platform)

    static = True
    if platform == 'win32':
        distutils.log.info("Add windows settings")
        platform_specific['libraries'] = ['Advapi32']
        platform_specific['define_macros'] = [('WIN32', 1)]
        if sys.version > '3':
            platform_specific['extra_compile_args'] = [
                '/Zi', '/EHsc', '/std:c++14']
        else:
            platform_specific['extra_compile_args'] = ['/Zi', '/EHsc']
        platform_specific['extra_link_args'] = ['/DEBUG']
        jni_md_platform = 'win32'

    elif platform == 'darwin':
        distutils.log.info("Add darwin settings")
        platform_specific['libraries'] = ['dl']
        platform_specific['define_macros'] = [('MACOSX', 1)]
        platform_specific['extra_compile_args'] = ['-g0', '-std=c++11', '-O2']
        jni_md_platform = 'darwin'

    elif platform.startswith('linux'):
        distutils.log.info("Add linux settings")
        platform_specific['libraries'] = ['dl']
        platform_specific['extra_compile_args'] = ['-g0', '-std=c++11', '-O2']
        jni_md_platform = 'linux'

    elif platform.startswith('aix7'):
        distutils.log.info("Add aix settings")
        platform_specific['libraries'] = ['dl']
        platform_specific['extra_compile_args'] = ['-g3', '-std=c++11', '-O2']
        jni_md_platform = 'aix7'

    elif platform.startswith('freebsd'):
        distutils.log.info("Add freebsd settings")
        jni_md_platform = 'freebsd'

    elif platform.startswith('android'):
        distutils.log.info("Add android settings")
        platform_specific['libraries'] = ['dl', 'c++_shared', 'SDL2']
        platform_specific['extra_compile_args'] = ['-g0', '-std=c++11', '-fexceptions', '-frtti', '-O2']

        print("PLATFORM_SPECIFIC:", platform_specific)
        jni_md_platform = 'linux'
        static = False

    else:
        jni_md_platform = None
        distutils.log.warn("Your platform '%s' is not being handled explicitly."
                           " It may work or not!", platform)

# This code is used to include python library in the build when starting Python from
# within Java.  It will be used in the future, but is not currently required.
#    if static and sysconfig.get_config_var('BLDLIBRARY') is not None:
#        platform_specific['extra_link_args'].append(sysconfig.get_config_var('BLDLIBRARY'))

    if found_jni:
        distutils.log.info("Add JNI directory %s" % os.path.join(java_home, 'include', jni_md_platform))
        platform_specific['include_dirs'] += \
            [os.path.join(java_home, 'include', jni_md_platform)]
    return platform_specific


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
