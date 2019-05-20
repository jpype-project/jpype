# *****************************************************************************
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
# *****************************************************************************
from . import _jcustomizer
import sys as _sys


@_jcustomizer.JImplementationFor('java.lang.Comparable')
class _JComparable(object):
    if _sys.version_info < (3,):
        def __cmp__(self, o):
            return self.compareTo(o)

    def __eq__(self, o):
        return self.compareTo(o) == 0

    def __ne__(self, o):
        return self.compareTo(o) != 0

    def __gt__(self, o):
        return self.compareTo(o) > 0

    def __lt__(self, o):
        return self.compareTo(o) < 0

    def __ge__(self, o):
        return self.compareTo(o) >= 0

    def __le__(self, o):
        return self.compareTo(o) <= 0
