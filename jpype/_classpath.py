import os as _os
import glob as _glob

__all__ = ['addClassPath', 'getClassPath']

_CLASSPATHS = []
_SEP = _os.path.pathsep


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


def getClassPath(env=True):
    """ Get the full java class path.

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
