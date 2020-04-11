import os as _os
import sys as _sys

__all__ = ['addClassPath', 'getClassPath']

_CLASSPATHS = []
_SEP = _os.path.pathsep
if _sys.platform == 'cygwin':
    _SEP = ';'


# Cygwin needs to convert to windows paths
if _sys.platform == 'cygwin':
    _root = None

    def _get_root():
        global _root
        if _root != None:
            return _root
        import subprocess
        proc = subprocess.Popen("cygpath -wa /", shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, close_fds=True)
        _root = proc.stdout.read().strip().decode('utf-8')
        return _root

    def _splitpath(path):
        parts = []
        (path, tail) = _os.path.split(path)
        while path and tail:
            parts.insert(0, tail)
            (path, tail) = _os.path.split(path)
        return parts

    def _posix2win(directory):
        from pathlib import Path
        directory = str(directory)
        if len(directory) > 3 and directory[1:3] == ":\\":
            return Path(directory)
        root = _get_root()
        directory = _os.path.abspath(directory)
        paths = _splitpath(directory)
        if paths[0] == "cygdrive":
            paths.pop(0)
            drive = paths.pop(0)
            paths.insert(0, "%s:" % drive)
            return Path('\\'.join(paths))
        paths.insert(0, root)
        return Path('\\'.join(paths))
    # needed for testing
    __all__.append("_posix2win")


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

    if _sys.platform == 'cygwin':
        path1 = _posix2win(path1)

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
            paths = path.parent.glob("*.jar")
            if len(paths) == 0:
                continue
            out.extend(paths)
        else:
            out.append(path)
    return _SEP.join([str(i) for i in out])
