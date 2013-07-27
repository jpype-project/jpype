#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
from glob import glob
import platform

from distutils.core import setup
from distutils.core import Extension


def sources():
    cpp_files = []
    for dirpath, dirnames, filenames in os.walk('src'):
        for filename in filenames:
            if filename.endswith('.cpp'):
                cpp_files.append(os.path.join(dirpath, filename))
    return cpp_files

ext = {}
ext['sources'] = sources()

if sys.platform == 'win32':
    java_home = os.getenv('JAVA_HOME')
    if not java_home:
        raise SystemExit('environment variable JAVA_HOME must be set')
    ext['libraries'] = ['Advapi32']
    ext['library_dir'] = [os.path.join(java_home, 'lib')]
    ext['define_macros'] = [('WIN32', 1)]
    ext['extra_compile_args'] = ['/EHsc']
    ext['include_dirs'] = [
        'src/native/common/include',
        'src/native/python/include',
        os.path.join(java_home, 'include'),
        os.path.join(java_home, 'include', 'win32')
    ]
elif sys.platform == 'darwin':
    # Changes according to:
    # http://stackoverflow.com/questions/8525193/cannot-install-jpype-on-os-x-lion-to-use-with-neo4j
    # and
    # http://blog.y3xz.com/post/5037243230/installing-jpype-on-mac-os-x
    osx = platform.mac_ver()[0][:4]
    java_home = '/Library/Java/Home'
    if osx == '10.6':
        # I'm not sure if this really works on all 10.6 - confirm please :)
        java_home = ('/Developer/SDKs/MacOSX10.6.sdk/System/Library/'
                     'Frameworks/JavaVM.framework/Versions/1.6.0/')
    elif osx in ('10.7', '10.8'):
        java_home = ('/System/Library/Frameworks/JavaVM.framework/'
                     'Versions/Current/')
    ext['libraries'] = ['dl']
    ext['library_dir'] = [os.path.join(java_home, 'Libraries')]
    ext['define_macros'] = [('MACOSX', 1)]
    ext['include_dirs'] = [
        'src/native/common/include',
        'src/native/python/include',
        os.path.join(java_home, 'Headers'),
    ]
else:
    java_home = os.getenv('JAVA_HOME')
    if not java_home:
        possible_homes = glob('/usr/lib/jvm/*')  # (almost) standard in GNU/Linux
        possible_homes += glob('/usr/java/*')    # Java oracle in some cases
        for home in possible_homes:
            include_path = os.path.join(home, 'include')
            if os.path.exists(include_path):
                java_home = home
                break
        else:
            raise RuntimeError(
                "No Java/JDK could be found. I looked in the following "
                "directories: %s\nPlease check that you have it installed. "
                "If you have and the destination is not in the above list "
                "please consider opening a ticket or creating a pull request "
                "on github: https://github.com/originell/jpype/"
                % '\n' + '\n'.join(possible_homes))

    ext['libraries'] = ['dl']
    ext['library_dir'] = [os.path.join(java_home, 'lib')]
    ext['include_dirs'] = [
        'src/native/common/include',
        'src/native/python/include',
        os.path.join(java_home, 'include'),
        os.path.join(java_home, 'include', 'linux'),
    ]

jpypeLib = Extension(name='_jpype',
                     sources=ext.get('sources'),
                     libraries=ext.get('libraries'),
                     define_macros=ext.get('define_macros'),
                     include_dirs=ext.get('include_dirs'),
                     library_dirs=ext.get('library_dir'),
                     extra_compile_args=ext.get('extra_compile_args'),
                     )

setup(
    name='JPype',
    version='0.5.4.2',
    description='Python-Java bridge',
    author='Steve Menard',
    author_email='devilwolf@users.sourceforge.net',
    url='http://jpype.sourceforge.net/',
    packages=[
        'jpype', 'jpype.awt', 'jpype.awt.event', 'jpypex', 'jpypex.swing'],
    package_dir={
        'jpype': 'src/python/jpype',
        'jpypex': 'src/python/jpypex',
    },
    ext_modules=[jpypeLib],
)
