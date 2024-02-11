from ._core import getDefaultJVMPath
import os.path

_JPypeJarPath = "file://" + os.path.join(os.path.dirname(os.path.dirname(__file__)), "org.jpype.jar")
_JPypeJVMPath = getDefaultJVMPath()
