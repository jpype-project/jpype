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
from pathlib import Path
import sys
import sysconfig
import typing
import distutils.log

# This handles all the work to make our platform specific extension options.


def Platform(*, include_dirs: typing.Sequence[Path], sources: typing.Sequence[Path], platform: str):
    sources = [str(pth) for pth in sources]
    platform_specific = {
        'include_dirs': include_dirs,
        'sources': sources,
    }

    fallback_jni = os.path.join('native', 'jni_include')
    # try to include JNI first from eventually given JAVA_HOME, then from distributed
    java_home = os.getenv('JAVA_HOME', '')
    found_jni = False
    if os.path.exists(java_home):
        platform_specific['include_dirs'] += [
            os.path.join(java_home, 'include')]

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
    cpp_std = "c++11"
    gcc_like_cflags = ['-g0', f'-std={cpp_std}', '-O2']

    if platform == 'win32':
        distutils.log.info("Add windows settings")
        platform_specific['define_macros'] = [('WIN32', 1)]
        if sys.version > '3':
            platform_specific['extra_compile_args'] = [
                '/Zi', '/EHsc', f'/std:c++14']
        else:
            platform_specific['extra_compile_args'] = ['/Zi', '/EHsc']
        if hasattr(sys, "_is_gil_enabled") and not sys._is_gil_enabled():
            # running in free threaded build means build the free threaded one
            # this is only neecessary for windows
            platform_specific['define_macros'].append(('Py_GIL_DISABLED', '1'))

        jni_md_platform = 'win32'

    elif platform == 'darwin':
        distutils.log.info("Add darwin settings")
        platform_specific['libraries'] = ['dl']
        platform_specific['define_macros'] = [('MACOSX', 1)]
        platform_specific['extra_compile_args'] = gcc_like_cflags
        jni_md_platform = 'darwin'

    elif platform.startswith('linux'):
        distutils.log.info("Add linux settings")
        platform_specific['libraries'] = ['dl']
        platform_specific['extra_compile_args'] = gcc_like_cflags
        jni_md_platform = 'linux'

    elif platform.startswith('aix7'):
        distutils.log.info("Add aix settings")
        platform_specific['libraries'] = ['dl']
        platform_specific['extra_compile_args'] = gcc_like_cflags
        jni_md_platform = 'aix7'

    elif platform.startswith('freebsd'):
        distutils.log.info("Add freebsd settings")
        jni_md_platform = 'freebsd'

    elif platform.startswith('openbsd'):
        distutils.log.info("Add openbsd settings")
        jni_md_platform = 'openbsd'

    elif platform.startswith('android'):
        distutils.log.info("Add android settings")
        platform_specific['libraries'] = ['dl', 'c++_shared', 'SDL2']
        platform_specific['extra_compile_args'] = gcc_like_cflags + \
            ['-fexceptions', '-frtti']

        print("PLATFORM_SPECIFIC:", platform_specific)
        jni_md_platform = 'linux'

    elif platform == 'zos':
        distutils.log.info("Add zos settings")
        jni_md_platform = 'zos'

    elif platform == 'sunos5':
        distutils.log.info("Add solaris settings")
        jni_md_platform = 'solaris'

    else:
        jni_md_platform = ''
        distutils.log.warn("Your platform '%s' is not being handled explicitly."
                           " It may work or not!", platform)

   # This code is used to include python library in the build when starting Python from
   # within Java.  It will be used in the future, but is not currently required.
   # if static and sysconfig.get_config_var('BLDLIBRARY') is not None:
   #     platform_specific['extra_link_args'].append(sysconfig.get_config_var('BLDLIBRARY'))

    if found_jni:
        distutils.log.info("Add JNI directory %s" %
                           os.path.join(java_home, 'include', jni_md_platform))
        platform_specific['include_dirs'] += \
            [os.path.join(java_home, 'include', jni_md_platform)]
    return platform_specific
