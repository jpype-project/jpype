import jpype
import sys
from jpype.types import *
import jpype.imports
jpype.startJVM(classpath='project/jpype_java/dist/*')

from java.nio.file import Paths, Files

html = JClass("org.jpype.html.Html")
hw = JClass("org.jpype.html.HtmlWriter")
jdz = JClass("org.jpype.html.JavadocZip")(Paths.get("project/jpype_java/jdk-8u251-docs-all.zip"))
jde = JClass("org.jpype.html.JavadocExtractor")
#jip = jdz.getPath(JClass("java.lang.Double"))
#lines = Files.readAllLines(jip)
# for l in lines:
#    print(l)
# for k,v in html.ENTITIES.items():
#    print(k, html.decode("&"+str(k)+";").charAt(0))
#print(html.decode("public static&trade;double&nbsp;sum(double&nbsp;a,double&nbsp;b)"))
# sys.exit(0)

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
    print(hw.asString(jd.processNode(c)))
    print("---------------------------------------------------------")

print("=========================================================")
