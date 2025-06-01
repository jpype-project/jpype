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
import ctypes.util
import typing

# --- Logging compatibility shim ---
try:
    import distutils.log as _log
    def log_info(msg, *args): _log.info(msg, *args)
    def log_warn(msg, *args): _log.warn(msg, *args)
except ImportError:
    import logging
    logging.basicConfig(level=logging.INFO)
    def log_info(msg, *args): logging.info(msg, *args)
    def log_warn(msg, *args): logging.warning(msg, *args)
# ----------------------------------

def find_python_shared_lib():
    # Get Python version (e.g., '3.11')
    version = '{}{}'.format(sys.version_info.major, sys.version_info.minor)
    # Try sysconfig for the actual library name
    ldlib = sysconfig.get_config_var('LDLIBRARY')
    libdir = sysconfig.get_config_var('LIBDIR')
    if ldlib and libdir:
        candidate = os.path.join(libdir, ldlib)
        if os.path.exists(candidate):
            return candidate
    # Fallback: use ctypes.util.find_library
    if sys.platform == "win32":
        libname = "python" + version
    elif sys.platform == "darwin":
        libname = "python" + version
    else:
        libname = "python" + version
    path = ctypes.util.find_library(libname)
    if path:
        return path
    # Fallback: search common locations
    candidates = []
    if sys.platform == "win32":
        candidates = [
            os.path.join(sys.prefix, 'python{}.dll'.format(version)),
            os.path.join(sys.prefix, 'DLLs', 'python{}.dll'.format(version)),
        ]
    elif sys.platform == "darwin":
        candidates = [
            os.path.join(sys.prefix, 'lib', 'libpython{}.dylib'.format(version)),
            os.path.join(sys.prefix, 'lib', 'python{}.dylib'.format(version)),
        ]
    else:
        candidates = [
            os.path.join(sys.prefix, 'lib', 'libpython{}.so'.format(version)),
            os.path.join(sys.prefix, 'lib', 'libpython{}.so.{}'.format(version, sys.version_info.micro)),
        ]
    for candidate in candidates:
        if os.path.exists(candidate):
            return candidate
    return None

def config_windows(platform_specific, debug=False):
    """
    Configure build settings for the Windows platform (Python 3+ only).

    - Adds 'WIN32' macro for Windows-specific compilation.
    - Uses C++14 standard.
    - Adds optimization, exception handling, and runtime flags.
    - Optionally adds debug info if debug=True.
    - If Python is built without the GIL, defines 'Py_GIL_DISABLED'.

    Args:
        platform_specific (dict): Dictionary to update with platform-specific flags.
        debug (bool): If True, include debug info (/Zi). Default is False.

    Returns:
        str: JNI subdirectory name for Windows ('win32').
    """
    log_info("Add windows settings")
    platform_specific['define_macros'] = [('WIN32', 1)]
    
    # Compile args for release build
    compile_args = ['/O2', '/EHsc', '/MD', '/std:c++14']
    
    # Add debug info if requested
    if debug:
        compile_args.append('/Zi')
    
    platform_specific['extra_compile_args'] = compile_args

    # If Python is built without the Global Interpreter Lock, define macro
    if hasattr(sys, "_is_gil_enabled") and not sys._is_gil_enabled():
        platform_specific['define_macros'].append(('Py_GIL_DISABLED', '1'))
    return 'win32'

def config_darwin(platform_specific):
    """
    Configure build settings for macOS (Darwin).

    - Links against the dynamic loading library ('dl').
    - Defines 'MACOSX' macro for macOS-specific compilation.
    - Uses C++11 standard, disables debug info, and enables optimization.
    Returns:
        str: JNI subdirectory name for macOS ('darwin').
    """
    log_info("Add darwin settings")
    platform_specific['libraries'] = ['dl']
    platform_specific['define_macros'] = [('MACOSX', 1)]
    platform_specific['extra_compile_args'] = ['-g0', '-std=c++11', '-O2']
    return 'darwin'

def config_linux(platform_specific):
    """
    Configure build settings for Linux.

    - Links against the dynamic loading library ('dl').
    - Uses C++11 standard, disables debug info, and enables optimization.
    Returns:
        str: JNI subdirectory name for Linux ('linux').
    """
    log_info("Add linux settings")
    platform_specific['libraries'] = ['dl']
    platform_specific['extra_compile_args'] = ['-g0', '-std=c++11', '-O2']
    return 'linux'

def config_aix7(platform_specific):
    """
    Configure build settings for IBM AIX 7.

    - Links against the dynamic loading library ('dl').
    - Uses C++11 standard, disables debug info, and enables optimization.
    Returns:
        str: JNI subdirectory name for AIX 7 ('aix7').
    """
    log_info("Add aix settings")
    platform_specific['libraries'] = ['dl']
    platform_specific['extra_compile_args'] = ['-g0', '-std=c++11', '-O2']
    return 'aix7'

def config_freebsd(platform_specific):
    """
    Configure build settings for FreeBSD.

    No additional macros, libraries, or compile arguments are needed.
    Returns:
        str: JNI subdirectory name for FreeBSD ('freebsd').
    """
    log_info("Add freebsd settings")
    return 'freebsd'

def config_openbsd(platform_specific):
    """
    Configure build settings for OpenBSD.

    No additional macros, libraries, or compile arguments are needed.
    Returns:
        str: JNI subdirectory name for OpenBSD ('openbsd').
    """
    log_info("Add openbsd settings")
    return 'openbsd'

def config_android(platform_specific):
    """
    Configure build settings for Android.

    - Links against 'dl', 'c++_shared', and 'SDL2' libraries.
    - Uses C++11 standard, disables debug info, enables optimization,
      and ensures exceptions and RTTI are enabled.
    Returns:
        str: JNI subdirectory name for Android ('linux').
    """
    log_info("Add android settings")
    platform_specific['libraries'] = ['dl', 'c++_shared', 'SDL2']
    platform_specific['extra_compile_args'] = [
        '-g0', '-std=c++11', '-O2', '-fexceptions', '-frtti'
    ]
    return 'linux'

def config_zos(platform_specific):
    """
    Configure build settings for IBM z/OS.

    No additional macros, libraries, or compile arguments are needed.
    Returns:
        str: JNI subdirectory name for z/OS ('zos').
    """
    log_info("Add zos settings")
    return 'zos'

def config_sunos5(platform_specific):
    """
    Configure build settings for Solaris (SunOS 5).

    No additional macros, libraries, or compile arguments are needed.
    Returns:
        str: JNI subdirectory name for Solaris ('solaris').
    """
    log_info("Add solaris settings")
    return 'solaris'

def config_default(platform_specific):
    """
    Default configuration for unrecognized platforms.

    Issues a warning that the platform is not explicitly handled.
    Returns:
        str: Empty string, since no JNI subdirectory is specified.
    """
    log_warn("Your platform is not being handled explicitly. It may work or not!")
    return ''

# Map platform start strings to config functions
PLATFORM_CONFIGS = [
    ('win32', config_windows),
    ('darwin', config_darwin),
    ('linux', config_linux),
    ('aix7', config_aix7),
    ('freebsd', config_freebsd),
    ('openbsd', config_openbsd),
    ('android', config_android),
    ('zos', config_zos),
    ('sunos5', config_sunos5),
]

def get_platform_config(platform):
    for prefix, func in PLATFORM_CONFIGS:
        if platform.startswith(prefix):
            return func
    return config_default

def Platform(*, include_dirs: typing.Sequence[Path], sources: typing.Sequence[Path], platform: str):
    # Convert Path objects in sources to strings, as required by build systems
    sources = [str(pth) for pth in sources]

    # Initialize platform-specific build configuration dictionary
    platform_specific = {
        'include_dirs': list(include_dirs),  # Start with provided include directories
        'sources': sources,                  # Source files for compilation
    }

    # Define the fallback location for JNI headers if JAVA_HOME is not set or incomplete
    fallback_jni = os.path.join('native', 'jni_include')
    java_home = os.getenv('JAVA_HOME', '')
    found_jni = False

    # If JAVA_HOME is set and exists, try to use its JNI headers
    if os.path.exists(java_home):
        # Add the JAVA_HOME/include directory to the include path
        platform_specific['include_dirs'].append(os.path.join(java_home, 'include'))
        # Check if any of the include directories contains jni.h
        for d in platform_specific['include_dirs']:
            if os.path.exists(os.path.join(str(d), 'jni.h')):
                log_info("Found native jni.h at %s", d)
                found_jni = True
                break
        # If JAVA_HOME/include does not contain jni.h, issue a warning and fall back
        if not found_jni:
            log_warn(
                'Falling back to provided JNI headers, since your provided JAVA_HOME "%s" does not provide jni.h',
                java_home)
    # If JAVA_HOME is not set or jni.h was not found, use the fallback JNI headers
    if not found_jni:
        platform_specific['include_dirs'].append(fallback_jni)

    # Initialize extra linker arguments (may be populated by platform-specific config)
    platform_specific['extra_link_args'] = []

    # Select and apply the platform-specific configuration function
    config_func = get_platform_config(platform)
    jni_md_platform = config_func(platform_specific)

    # For POSIX platforms (not Windows), attempt to locate and link the Python shared library
    # This code is necessary if the module is started before Python when used with JNI
    if platform != 'win32':
        shared = find_python_shared_lib()
        if shared is not None:
            platform_specific['extra_link_args'].append(shared)

    # If JAVA_HOME JNI headers were found, add the platform-specific subdirectory (e.g., 'linux', 'win32')
    if found_jni:
        jni_md_dir = os.path.join(java_home, 'include', jni_md_platform)
        log_info("Add JNI directory %s", jni_md_dir)
        platform_specific['include_dirs'].append(jni_md_dir)

    # Return the fully populated platform-specific configuration dictionary
    return platform_specific
