import os
from pathlib import Path


fspath = getattr(os, 'fspath', str)


_pyinstaller_path = Path(__file__).parent


def get_hook_dirs():
    return [fspath(str(_pyinstaller_path))]


def get_PyInstaller_tests():
    return [fspath(_pyinstaller_path)]
