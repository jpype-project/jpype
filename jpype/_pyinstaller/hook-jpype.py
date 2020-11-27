import os
from pathlib import Path

import jpype


fspath = getattr(os, 'fspath', str)

jar_path = Path(jpype.__file__).parent.parent.joinpath('org.jpype.jar')

datas = [[fspath(jar_path), '.']]
