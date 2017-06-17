#*****************************************************************************
#   Copyright 2017 Karl Einar Nelson
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
#*****************************************************************************
try:
    import unittest2 as unittest
except ImportError:
    import unittest
from inspect import isclass

def importAll():
    # Import in local scope to keep namespace clean
    import os as _os

    # Search through all modules in the toplevel directory
    for _file in _os.listdir(_os.path.dirname(__file__)):

        # Find all modules in the directory
        if _file.startswith('__') or _file[-3:] != '.py':
            continue

        # import module
        _name = _file[:-3]
        exec("from . import %s"% _name)
        _module=globals()[_name]
        for n,cls in _module.__dict__.items():
            if not isclass(cls):
                continue
            if not issubclass(cls, unittest.TestCase):
                continue
            globals()[n]=cls

importAll()
