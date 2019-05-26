# *****************************************************************************
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
# *****************************************************************************
from ._jpackage import *
from ._jproxy import *
from ._core import *
from ._gui import *
from ._classpath import *
from ._jclass import *
from .types import *
from ._jcustomizer import *
from . import reflect
from . import nio
from . import types
from ._jcustomizer import *


__all__ = ['java', 'javax', 'JException', 'JOverride']
__all__.extend(_core.__all__)
__all__.extend(_classpath.__all__)
__all__.extend(types.__all__)
__all__.extend(_jproxy.__all__)
__all__.extend(_jpackage.__all__)
__all__.extend(_jclass.__all__)
__all__.extend(_jcustomizer.__all__)
__all__.extend(_gui.__all__)

__version_info__ = (0, 7, 0)
__version__ = ".".join(str(i) for i in __version_info__)


@_core.deprecated
def JIterator(it):
    """Deprecated"""
    return it


# FIXME these should be deprecated. The old JPackage system is only for
#  python2 series and generates lots of deceptive classes.  At some point
#  these two are going to have to go away.
java = JPackage("java", strict=True)
javax = JPackage("javax", strict=True)
