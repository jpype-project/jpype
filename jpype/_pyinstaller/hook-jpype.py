import os
from pathlib import Path

from PyInstaller.utils.hooks import relpath_to_config_or_make

import jpype


fspath = getattr(os, 'fspath', str)

jar_path = Path(jpype.__file__).parent.parent.joinpath('org.jpype.jar')

datas = [[fspath(jar_path), '.']]
