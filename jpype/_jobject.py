#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#*****************************************************************************
from . import _jclass

def _initialize():
    _jclass.registerClassCustomizer(ComparableCustomizer())

class ComparableCustomizer(object):
    _METHODS = {
            "__cmp__": lambda self, o: self.compareTo(o)
    }

    def canCustomize(self, name, jc):
        return name == 'java.lang.Comparable'

    def customize(self, name, jc, bases, members):
        members.update(ComparableCustomizer._METHODS)
