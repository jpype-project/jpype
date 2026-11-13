# *****************************************************************************
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
#   See NOTICE file for details.
#
# *****************************************************************************
from __future__ import annotations # for py38

import os as _os
import threading
from pathlib import Path
from typing import Union

import _jpype

_CLASSPATHS: list[Path] = []
_SEP = _os.path.pathsep

# Guards _CLASSPATHS and the "is the JVM started" decision in addClassPath().
# startJVM holds this lock for the whole start (see _core._jvm_start_locked),
# so a concurrent addClassPath() cannot slip in between the classpath snapshot
# and the JVM becoming available.  Reentrant because a JVM initializer may call
# back into the classpath API.  Never call into Java while holding it: Java
# calls can need the type factory monitor, whose holder may be a customizer
# calling addClassPath().
_classpath_lock = threading.RLock()

__all__ = ['addClassPath', 'getClassPath']

def addClassPath(path1: Union[str, _os.PathLike]) -> None:
    """ Add a path to the Java class path

    Classpath items can be a java, a directory, or a
    glob pattern.  Relative paths are relative to the 
    caller location.

    Arguments:
      path(str):

    """
    # We are deferring these imports until here as we only need them
    # if this function is used.
    import inspect
    global _CLASSPATHS

    # Convert to an absolute path.  Note that
    # relative paths will be resolve based on the location
    # of the caller rather than the JPype directory.
    path1 = Path(path1)
    if not path1.is_absolute():
        path2 = Path(inspect.stack(1)[1].filename).parent.resolve()
        path1 = path2.joinpath(path1)

    with _classpath_lock:
        # Before the JVM runs the path is only recorded; startJVM reads it.
        # startJVM holds the lock until the JVM is up, so once this sees the
        # JVM as started the snapshot is taken and the path must be loaded.
        if not _jpype.isStarted():
            _CLASSPATHS.append(path1)
            return

    # The JVM is running: load the path into the JPypeClassLoader.  This must
    # not hold _classpath_lock.  The Java calls below may create class
    # wrappers, which takes the type factory monitor, and the thread holding
    # that monitor may be running a customizer that calls addClassPath().
    Paths = _jpype.JClass('java.nio.file.Paths')
    JContext = _jpype.JClass('org.jpype.JPypeContext')
    classLoader = JContext.getInstance().getClassLoader()
    if path1.name == "*":
        paths = list(path1.parent.glob("*.jar"))
        if len(paths) == 0:
            return
        for path in paths:
            classLoader.addPath(Paths.get(str(path)))
    else:
        classLoader.addPath(Paths.get(str(path1)))
    with _classpath_lock:
        _CLASSPATHS.append(path1)


def getClassPath(env: bool = True) -> str:
    """ Get the full Java class path.

    Includes user added paths and the environment CLASSPATH.

    Arguments:
      env(Optional, bool): If true then environment is included.
        (default True)
    """
    global _CLASSPATHS
    global _SEP

    with _classpath_lock:
        # Merge the environment path
        classPath = list(_CLASSPATHS)
        envPath = _os.environ.get("CLASSPATH")
        if env and envPath:
            classPath.extend([Path(i) for i in envPath.split(_SEP)])

        out = []
        for path in classPath:
            if path == '':
                continue
            if path.name == "*":
                paths = list(path.parent.glob("*.jar"))
                if len(paths) == 0:
                    continue
                out.extend(paths)
            else:
                out.append(path)
        return _SEP.join([str(i) for i in out])
