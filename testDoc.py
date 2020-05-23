import _jpype
import jpype
import sys
from jpype.types import *
import jpype.imports
jpype.startJVM(classpath='project/jpype_java/dist/*')

from java.nio.file import Paths, Files

html = JClass("org.jpype.html.Html")
hw = JClass("org.jpype.html.HtmlWriter")
jdz = JClass("org.jpype.javadoc.JavadocZip")(Paths.get("project/jpype_java/jdk-8u251-docs-all.zip"))
jde = JClass("org.jpype.javadoc.JavadocExtractor")
jdf = JClass("org.jpype.javadoc.JavadocFormatter")()
jdr = JClass("org.jpype.javadoc.JavadocRenderer")()

current = None


def renderClass(cls):
    global current
    jis = jdz.getInputStream(cls)
    if jis is None:
        return

    print("=========================================================")
    print("CLASS", cls)
    jd = jde.extractStream(jis)
    if jd.methods is not None:
        for c in jd.methods:
            current = c
            jdf.transformMember(c)
            print(jdr.renderMember(c))
            print("---------------------------------------------------------")

    if jd.fields is not None:
        for c in jd.fields:
            current = c
            jdf.transformMember(c)
            print(jdr.renderMember(c))
            print("---------------------------------------------------------")
    print("=========================================================")


def renderPackage(pkg):
    print(type(pkg))
    print(dir(pkg))
    for i in dir(pkg):
        p = getattr(pkg, i)
        if isinstance(p, _jpype._JPackage):
            renderPackage(p)
            continue
        if isinstance(p, jpype.JClass):
            renderClass(p)


jlang = jpype.JPackage("java")
try:
    renderPackage(jlang)
except Exception as ex:
    print("Error in")
    print(hw.asString(current))
    raise ex

# print("=========================================================")
