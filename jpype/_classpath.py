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
import os as _os

__all__ = ['addClassPath', 'getClassPath']

_CLASSPATHS = []
_SEP = _os.path.pathsep

_ourClassLoader = None
File = None

def addClassPath(path1):
    """ Add a path to the Java class path

    Classpath items can be a java, a directory, or a
    glob pattern.  Relative paths are relative to the 
    caller location.

    Arguments:
      path(str):

    """
    # We are deferring these imports until here as we only need them
    # if this function is used.
    from pathlib import Path
    import inspect
    global _CLASSPATHS

    # Convert to an absolute path.  Note that
    # relative paths will be resolve based on the location
    # of the caller rather than the JPype directory.
    path1 = Path(path1)
    if not path1.is_absolute():
        path2 = Path(inspect.stack(1)[1].filename).parent.resolve()
        path1 = path2.joinpath(path1)

    _CLASSPATHS.append(path1)
    if _ourClassLoader:
        _ourClassLoader.addFile(File(str(path1)).toPath())


def getClassPath(env=True):
    """ Get the full Java class path.

    Includes user added paths and the environment CLASSPATH.

    Arguments:
      env(Optional, bool): If true then environment is included.
        (default True)
    """
    from pathlib import Path
    global _CLASSPATHS
    global _SEP

    # Merge the evironment path
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
