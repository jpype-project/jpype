import os as _os
import sys as _sys
import glob as _glob

__all__=['addClassPath', 'getClassPath']

_CLASSPATHS=set()
_SEP = _os.path.pathsep
if _sys.platform=='cygwin':
    _SEP=';'

def _init():
    global _CLASSPATHS
    global _SEP
    classpath=_os.environ.get("CLASSPATH")
    if classpath:
        _CLASSPATHS|=set(classpath.split(_SEP))

_init()

# Cygwin needs to convert to windows paths
if _sys.platform=='cygwin':
    _root=None
    def _get_root():
        global _root
        if _root!=None:
            return _root
        import subprocess
        proc = subprocess.Popen("cygpath -wa /", shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, close_fds=True)
        _root=proc.stdout.read().strip().decode('utf-8')
        return _root

    def _splitpath(path):
        parts=[]
        (path, tail)=_os.path.split( path)
        while path and tail:
             parts.insert(0,tail)
             (path,tail)=_os.path.split(path)
        return parts

    def _posix2win(directory):
        root=_get_root()
        directory=_os.path.abspath(directory)
        paths=_splitpath(directory)
        if paths[0]=="cygdrive":
            paths.pop(0)
            drive=paths.pop(0)
            paths.insert(0, "%s:"%drive)
            return '\\'.join(paths)
        paths.insert(0,root)
        return '\\'.join(paths)
    # needed for testing
    __all__.append("_posix2win")

def addClassPath(path1):
    """ Add a path to the java class path"""
    global _CLASSPATHS
    path1=_os.path.abspath(path1)
    if _sys.platform=='cygwin':
        path1=_posix2win(path1)
    _CLASSPATHS.add(str(path1))

def getClassPath():
    """ Get the full java class path.

    Includes user added paths and the environment CLASSPATH.
    """
    global _CLASSPATHS
    global _SEP
    out=[]
    for path in _CLASSPATHS:
        if path=='':
            continue
        if path.endswith('*'):
            paths=_glob.glob(path+".jar")
            if len(path)==0:
                continue 
            out.extend(paths)
        else:
            out.append(path)
    return _SEP.join(out)


#print(getClassPath())
