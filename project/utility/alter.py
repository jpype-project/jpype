# Utility used to alter a Java jar file to date it in the future.
# This was needed to intentionally trigger a version error.
import jpype
import jpype.imports
jpype.startJVM()

from java.util.jar import JarOutputStream
from java.util.jar import JarInputStream
from java.util.zip import CRC32
from java.io import File
from java.io import FileInputStream
from java.io import FileOutputStream

jar = JarInputStream(FileInputStream(File("build/lib/org.jpype.jar")))
manifest = jar.getManifest()
target = JarOutputStream(FileOutputStream(File("build/lib/org.jpype2.jar")), manifest)


while 1:
    entry=jar.getNextEntry()
    if not entry:
        break
    out = []
    l3 = 512
    while 1:
        bt=jpype.JArray(jpype.JByte)(l3)
        l = jar.read(bt, 0, l3)
        if l==-1:
            break
        out.append((l,bt))

    if out:
        out[0][1][7] = 57

        crc = CRC32()
        for v in out:
            crc.update(v[1],0,v[0])

        entry.setCrc(crc.getValue())
        entry.setCompressedSize(-1)

    target.putNextEntry(entry)
    for v in out:
        target.write(v[1],0,v[0])
    target.closeEntry()
target.close()

