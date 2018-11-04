#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
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

import collections
import sys

import _jpype
from . import _jclass
from . import JClassUtil

if sys.version > '3':
    unicode = str

def _initialize():
    _jpype.setResource('ProxyClass',JProxy)

class JProxy(object):
    def __init__(self, intf, dict=None, inst=None):
        actualIntf = None

        if isinstance(intf, str) or isinstance(intf, unicode):
            actualIntf = [_jclass.JClass(intf)]
        elif isinstance(intf, _jclass._JavaClass):
            actualIntf = [intf]
        elif isinstance(intf, collections.Sequence):
            actualIntf = []
            for i in intf:
                if isinstance(i, str) or isinstance(i, unicode):
                    actualIntf.append(_jclass.JClass(i))
                elif isinstance(i, _jclass._JavaClass):
                    actualIntf.append(i)
                else:
                    raise TypeError("JProxy requires java interface classes "
                                    "or the names of java interfaces classes")
        else:
            raise TypeError("JProxy requires java interface classes "
                            "or the names of java interfaces classes")

        for i in actualIntf:
            if not JClassUtil.isInterface(i):
                raise TypeError("JProxy requires java interface classes "
                                "or the names of java interfaces classes: {0}"
                                .format(i.__name__))

        if dict is not None and inst is not None:
            raise RuntimeError("Specify only one of dict and inst")

        self._dict = dict
        self._inst = inst

        self._proxy = _jpype.createProxy(self, actualIntf)

    def getCallable(self, name):
        if self._dict is not None:
            return self._dict[name]
        else:
            return getattr(self._inst, name)
