import _jpype
import jpype
from jpype.types import *
import jpype.imports
jpype.startJVM(classpath=['gson-2.8.5.jar', 'gson-2.8.5-javadoc.jar', 'project/jpype_java/dist/*', 'project/jpype_java/jdk-11.0.7_doc-all.zip'])

import org

html = JClass("org.jpype.html.Html")
hw = JClass("org.jpype.html.HtmlWriter")
#jdz = JClass("org.jpype.javadoc.JavadocZip")(Paths.get("project/jpype_java/jdk-8u251-docs-all.zip"))
jde = JClass("org.jpype.javadoc.JavadocExtractor")
#jdf = JClass("org.jpype.javadoc.JavadocTransformer")()
#jdr = JClass("org.jpype.javadoc.JavadocRenderer")()

current = None


def renderClass(cls):
    global current
    jd = jde.getDocumentation(cls)
    #jis = jdz.getInputStream(cls)
    if jd is None:
        return

    print("=========================================================")
    print("CLASS", cls)

    print(jd.description)
    print(jd.ctors)
    print("---------------------------------------------------------")

    if jd.methods is not None:
        for p, v in jd.methods.items():
            print(v)
            print("- - - - - - - - - - - - - - - - - - - - - - - - - - - - -")

    print("---------------------------------------------------------")

    if jd.fields is not None:
        for p, v in jd.fields.items():
            print(v)
            print("- - - - - - - - - - - - - - - - - - - - - - - - - - - - -")
    print("=========================================================")


def renderPackage(pkg):
    for i in dir(pkg):
        print("Test", i)
        try:
            p = getattr(pkg, i)
        except Exception:
            continue
        if isinstance(p, _jpype._JPackage):
            renderPackage(p)
            continue
        if isinstance(p, jpype.JClass):
            renderClass(p)


try:
    renderPackage(jpype.JPackage('com.google.gson'))
except org.jpype.javadoc.JavadocException as ex:
    print("Javadoc Error: ", ex.message())
    print(hw.asString(ex.node))
    raise ex
except Exception as ex:
    print("Error in")
    print(hw.asString(current))
    raise ex
