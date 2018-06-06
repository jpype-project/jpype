# -*- coding: utf-8 -*-
import os
import codecs

def read_utf8(path, *parts):
    filename = os.path.join(os.path.dirname(path), *parts)
    return codecs.open(filename, encoding='utf-8').read()

def find_sources():
    cpp_files = []
    for dirpath, dirnames, filenames in os.walk(os.path.join('native')):
        for filename in filenames:
            if filename.endswith('.cpp') or filename.endswith('.c'):
                cpp_files.append(os.path.join(dirpath, filename))
    return cpp_files

