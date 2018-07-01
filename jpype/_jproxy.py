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

__all__ = ["JProxy"]

import _jpype
from . import _jclass

if sys.version > '3':
    unicode = str

class JProxy(object):
    def __init__(self, intf, dict=None, inst=None):
        actualIntf = None

        # We operate on lists of interfaces, so a single element is promoted 
        # to a list
        if not isinstance(intf, collections.Sequence):
            intf = [intf]

        # Verify that the list contains the required types
        actualIntf = []
        for i in intf:
            if isinstance(i, str) or isinstance(i, unicode):
                actualIntf.append(_jclass.JClass(i))
            elif isinstance(i, _jclass.JClass):
                actualIntf.append(i)
            else:
                raise TypeError("JProxy requires java interface classes "
                        "or the names of java interfaces classes: {0}".format(i.__name))

        # Check that all are interfaces
        for i in actualIntf:
            if not issubclass(i, _jclass.JInterface):
                raise TypeError("JProxy requires java interface classes "
                                "or the names of java interfaces classes: {0}"
                                .format(i.__name__))

        if dict is not None and inst is not None:
            raise RuntimeError("Specify only one of dict and inst")

        if dict is not None:
            def lookup(d, name):
              return d[name]
            self.__javaproxy__ = _jpype.PyJPProxy(dict, lookup, actualIntf)

        if inst is not None:
            def lookup(d, name):
              return getattr(d, name)
            self.__javaproxy__ = _jpype.PyJPProxy(inst, lookup, actualIntf)
