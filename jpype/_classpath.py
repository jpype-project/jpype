import os as _os
import sys as _sys
import glob as _glob

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
        root = _get_root()
        directory = _os.path.abspath(directory)
        paths = _splitpath(directory)
        if paths[0] == "cygdrive":
            paths.pop(0)
            drive = paths.pop(0)
            paths.insert(0, "%s:" % drive)
            return '\\'.join(paths)
        paths.insert(0, root)
        return '\\'.join(paths)
    # needed for testing
    __all__.append("_posix2win")


def addClassPath(path1):
    """ Add a path to the Java class path

    Classpath items can be a java, a directory, or a
    glob pattern.

    Arguments:
      path(str): 

    """
    global _CLASSPATHS

    path1 = _os.path.abspath(path1)
    if _sys.platform == 'cygwin':
        path1 = _posix2win(path1)
    _CLASSPATHS.append(str(path1))


def getClassPath(env=True):
    """ Get the full java class path.

    Includes user added paths and the environment CLASSPATH.

    Arguments:
      env(Optional, bool): If true then environment is included. 
        (default True)
    """
    global _CLASSPATHS
    global _SEP

    # Merge the evironment path
    classPath = list(_CLASSPATHS)
    envPath = _os.environ.get("CLASSPATH")
    if env and envPath:
        classPath.extend(envPath.split(_SEP))

    out = []
    for path in classPath:
        if path == '':
            continue
        if path.endswith('*'):
            paths = _glob.glob(path+".jar")
            if len(path) == 0:
                continue
            out.extend(paths)
        else:
            out.append(path)
    return _SEP.join(out)
