import jpype
from jpype.types import *
import jpype.imports
jpype.startJVM(classpath='project/jpype_java/dist/*')

from java.nio.file import Paths, Files

jdz = JClass("org.jpype.html.JavadocZip")(Paths.get("project/jpype_java/jdk-8u251-docs-all.zip"))
jde = JClass("org.jpype.html.JavadocExtractor")
#jip = jdz.getPath(JClass("java.lang.Double"))
#lines = Files.readAllLines(jip)
# for l in lines:
#    print(l)

jis = jdz.getInputStream(JClass("java.lang.Double"))
jd = jde.extractStream(jis)
print("=========================================================")
print("Description:")
print(jd.description)
print("=========================================================")
print("Ctors:")
for c in jd.ctors:
    print(c)
print("---------------------------------------------------------")

print("=========================================================")

print("Fields:")
for c in jd.fields:
    print(c)
    print("---------------------------------------------------------")

print("=========================================================")

print("Methods:")
for c in jd.methods:
    print(c)
    print("---------------------------------------------------------")

print("=========================================================")
