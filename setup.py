#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys
import codecs
import platform

from distutils.core import setup
from distutils.core import Extension


def read_utf8(*parts):
    filename = os.path.join(os.path.dirname(__file__), *parts)
    return codecs.open(filename, encoding='utf-8').read()


def find_sources():
    cpp_files = []
    for dirpath, dirnames, filenames in os.walk('src'):
        for filename in filenames:
            if filename.endswith('.cpp'):
                cpp_files.append(os.path.join(dirpath, filename))
    return cpp_files


platform_specific = {
    'include_dirs': [
        os.path.join('src', 'native', 'common', 'include'),
        os.path.join('src', 'native', 'python', 'include'),
    ],
    'sources': find_sources(),
}

java_home = os.getenv('JAVA_HOME')
if sys.platform == 'win32':
    if not java_home:
        sys.exit('\nEnvironment Variable JAVA_HOME must be set.\n')
    platform_specific['libraries'] = ['Advapi32']
    platform_specific['library_dir'] = [os.path.join(java_home, 'lib')]
    platform_specific['define_macros'] = [('WIN32', 1)]
    platform_specific['extra_compile_args'] = ['/EHsc']
    platform_specific['include_dirs'] += [
        os.path.join(java_home, 'include'),
        os.path.join(java_home, 'include', 'win32')
    ]
elif sys.platform == 'darwin':
    # Changes according to:
    # http://stackoverflow.com/questions/8525193/cannot-install-jpype-on-os-x-lion-to-use-with-neo4j
    # and
    # http://blog.y3xz.com/post/5037243230/installing-jpype-on-mac-os-x
    osx = platform.mac_ver()[0][:4]
    if not java_home:
        print "No JAVA_HOME Environment Variable set. Trying to guess it..."
        java_home = '/Library/Java/Home'
    if osx == '10.6':
        # I'm not sure if this really works on all 10.6 - confirm please :)
        java_home = ('/Developer/SDKs/MacOSX10.6.sdk/System/Library/'
                     'Frameworks/JavaVM.framework/Versions/1.6.0/')
    elif osx in ('10.7', '10.8'):
        java_home = ('/System/Library/Frameworks/JavaVM.framework/'
                     'Versions/Current/')
    platform_specific['libraries'] = ['dl']
    platform_specific['library_dir'] = [os.path.join(java_home, 'Libraries')]
    platform_specific['define_macros'] = [('MACOSX', 1)]
    platform_specific['include_dirs'] += [
        os.path.join(java_home, 'Headers'),
    ]
else:
    if not java_home:
        print "No JAVA_HOME Environment Variable set. Trying to guess it..."
        possible_homes = [
            '/usr/lib/jvm/default-java',
            '/usr/lib/jvm/java-6-sun',
            '/usr/lib/jvm/java-1.5.0-gcj-4.4',
            '/usr/lib/jvm/jdk1.6.0_30',
            '/usr/lib/jvm/java-1.5.0-sun-1.5.0.08',
            '/usr/java/jdk1.5.0_05',
            '/usr/lib/jvm/java-6-openjdk-amd64',   # xubuntu 12.10
            '/usr/lib/jvm/java-7-openjdk-amd64'    # java 7 ubuntu 12.04
        ]
        for home in possible_homes:
            include_path = os.path.join(home, 'include')
            if os.path.exists(include_path):
                java_home = home
                break
        else:
            raise RuntimeError(
                "No Java/JDK could be found. I looked in the following "
                "directories: \n\n%s\n\nPlease check that you have it "
                "installed.\n\nIf you have and the destination is not in the "
                "above list, please find out where your java's home is, "
                "set your JAVA_HOME environment variable to that path and "
                "retry the installation.\n"
                "If this still fails please open a ticket or create a "
                "pull request with a fix on github: "
                "https://github.com/originell/jpype/"
                % '\n'.join(possible_homes))

    platform_specific['libraries'] = ['dl']
    platform_specific['library_dir'] = [os.path.join(java_home, 'lib')]
    platform_specific['include_dirs'] += [
        os.path.join(java_home, 'include'),
        os.path.join(java_home, 'include', 'linux'),
        os.path.join(java_home, '..', 'include'),
        os.path.join(java_home, '..', 'include', 'linux'),
    ]


jpypeLib = Extension(name='_jpype', **platform_specific)


setup(
    name='JPype1',
    version='0.5.4.3',
    description='Friendly jpype fork with focus on easy installation.',
    long_description=read_utf8('README.rst'),
    license='License :: OSI Approved :: Apache Software License',
    author='Steve Menard',
    author_email='devilwolf@users.sourceforge.net',
    maintainer='Luis Nell',
    maintainer_email='cooperate@originell.org',
    url='https://github.com/originell/jpype/',
    platforms=[
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows :: Windows 7',
        'Operating System :: Microsoft :: Windows :: Windows Vista',
        'Operating System :: POSIX :: Linux',
    ],
    classifiers=[
        'Programming Language :: Java',
        'Programming Language :: Python :: 2.5',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
    ],
    packages=[
        'jpype', 'jpype.awt', 'jpype.awt.event', 'jpypex', 'jpypex.swing'],
    package_dir={
        'jpype': 'src/python/jpype',
        'jpypex': 'src/python/jpypex',
    },
    ext_modules=[jpypeLib],
)
