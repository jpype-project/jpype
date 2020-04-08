import os as _os
import glob as _glob

__all__ = ['addClassPath', 'getClassPath']

_CLASSPATHS = []
_SEP = _os.path.pathsep

def addClassPath(path1):
    """ Add a path to the Java class path

    Classpath items can be a java, a directory, or a
    glob pattern.

    Arguments:
      path(str):

    """
    global _CLASSPATHS

    path1 = _os.path.abspath(path1)
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
