import os
from pathlib import Path

fspath = getattr(os, 'fspath', str)


def get_hook_dirs():
    return [fspath(Path(__file__).parent)]
