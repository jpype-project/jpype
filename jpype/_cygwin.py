#*****************************************************************************
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
#*****************************************************************************

from . import _jvmfinder
from ._jvmfinder import JVMNotSupportedException
import struct as _struct
import sys as _sys

# ------------------------------------------------------------------------------

def _checkJVMArch(jvmPath):
    IMAGE_FILE_MACHINE_I386=332
    IMAGE_FILE_MACHINE_IA64=512
    IMAGE_FILE_MACHINE_AMD64=34404

    is64 = _sys.maxsize > 2**32
    with open(jvmPath, "rb") as f:
        s=f.read(2)
        if s!=b"MZ":
            raise JVMNotSupportedException("JVM not valid")
        else:
            f.seek(60)
            s=f.read(4)
            header_offset=_struct.unpack("<L", s)[0]
            f.seek(header_offset+4)
            s=f.read(2)
            machine=_struct.unpack("<H", s)[0]

    if machine==IMAGE_FILE_MACHINE_I386:
        if is64:
            raise JVMNotSupportedException("JVM mismatch, python is 64 bit and JVM is 32 bit.")
    elif machine==IMAGE_FILE_MACHINE_IA64 or machine==IMAGE_FILE_MACHINE_AMD64:
        if not is64:
            raise JVMNotSupportedException("JVM mismatch, python is 32 bit and JVM is 64 bit.")
    else:
        raise JVMNotSupportedException("Unable to deterime JVM Type")

class WindowsJVMFinder(_jvmfinder.JVMFinder):
    """
    Windows JVM library finder class
    """
    def __init__(self):
        """
        Sets up members
        """
        # Call the parent constructor
        _jvmfinder.JVMFinder.__init__(self)

        # Library file name
        self._libfile = "jvm.dll"

        # Search methods
        self._methods = (self._get_from_java_home, self._get_from_registry)

    def check(self, jvm):
        _checkJVMArch(jvm)

    def _get_from_registry(self):
        """
        Retrieves the path to the default Java installation stored in the
        Windows registry

        :return: The path found in the registry, or None
        """
        try:
            jreKey = "/proc/registry/HKEY_LOCAL_MACHINE/SOFTWARE/JavaSoft/Java Runtime Environment"
            with open(jreKey + "/CurrentVersion") as f:
                cv = f.read().split('\x00')
            versionKey = jreKey + "/" + cv[0]

            with open(versionKey + "/RunTimeLib") as f:
                cv = f.read().split('\x00')

            return cv[0]

        except OSError:
            return None
