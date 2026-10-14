"""Local override of python-for-android's own numpy recipe, adding one
patch this build harness needs. See jpype1's recipe docstring
(project/android/recipes/jpype1/__init__.py) for why local recipes live
under project/android/recipes/ at all, and doc/android_build.rst's
"Adding numpy" section for how this fits into the overall build.

numpy 2.3.0's src/multiarray/unique.cpp uses std::unordered_map (building
the per-dtype dispatch table for np.unique) but only #includes
<unordered_set> - relies on that header transitively pulling in
unordered_map, which happens to hold for libstdc++ (desktop Linux/glibc
builds) but not for the NDK's libc++, where the build fails with "no
template named 'unordered_map' in namespace 'std'". A real missing-include
bug in numpy's own source, not anything Android- or JPype-specific -
confirmed by grep: the file has no other unordered_map include anywhere.
unordered_map_include.patch adds the missing #include directly; check
whether a numpy release newer than 2.3.0 has already fixed this upstream
before bumping NumpyRecipe.version and dropping this patch.
"""
import sys
from os.path import dirname, join

# p4a loads each recipe's __init__.py by raw file path
# (pythonforandroid.util.load_source, spec_from_file_location +
# module_from_spec + exec_module) without registering it in sys.modules -
# so `pythonforandroid` itself is NOT already importable as a normal
# package at this point, even though this very file is being loaded BY
# code that lives inside it. Add its checkout to sys.path first so the
# `from pythonforandroid.recipes.numpy import ...` below is a genuine,
# separate, ordinary import (confirmed safe: it does not re-enter this
# file - p4a's own manual loader for *this* recipe never touches
# sys.modules, so there is nothing for a normal import to collide with).
sys.path.insert(0, join(dirname(dirname(dirname(__file__))), 'testapp',
                         '.buildozer', 'android', 'platform', 'python-for-android'))
from pythonforandroid.recipes.numpy import NumpyRecipe as _UpstreamNumpyRecipe


class NumpyRecipe(_UpstreamNumpyRecipe):
    patches = ['unordered_map_include.patch']


recipe = NumpyRecipe()
