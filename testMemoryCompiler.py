# This is work in progress code for allowing extensions of java classes from within
# jpype.
#
# Theory of operation:
# - The jvm must be started so that we have access to reflection.
# - We have a class which is a mock up for the extension class with 
#   annotations for the class we are extending and for each method
#   we wish to override.
# - When the class is encountered, it writes a section of java code
#   which creates a new class for which the methods are pointed to 
#   an invocation handler.
# - The python class creates a special invocation handler similar to the 
#   proxy which is static in the class and points to python class.
# - When the new class is constucted from within python, the class will get
#   a pointer to the python object and a reference to hold scope.
#
# Issues:
# - Generating a code segment to cover all the cases will be difficult.
# - Exceptions need to be passed through, thus constructors with exceptions
#   require complete specification.
# - Extending a generic will be challenging as reflection does not provide
#   everything in terms of names at least in some versions.
# - Writting code stubs that apply for many versions of java may be an issue.
#   I doubt that I can make it work for 1.6.
# - Compiling stubs from within java is rather slow.  But this will be low
#   usage as we only require is when a class is extended.
# - Name collisions can be an issue.
# - Python extension objects can't be serialized or cloned as they lack
#   a way to hold the python object.
#
# Even if the large amount of plumbing that is required for full support of this
# concept takes time, some pieces of this concept are useful immediately.  The
# memory compiler can be used to apply small bits of java code and thus allow
# simple extension where needed to bypass some current issues (such as the security
# handler)


import jpype
import jpype.imports
from jpype import JClass

jpype.startJVM(jpype.getDefaultJVMPath())
from java.lang import Class

# Fetch the internal class loader for jpype
cl = JClass("org.jpype.classloader.JPypeClassLoader")

# Get a class from jpype class loader
cls = Class.forName("org.jpype.compiler.MemoryCompiler", False, cl.getInstance())

# Convert it to a python class
memoryCompiler = JClass(cls)()

# Test compiling
TestClass=memoryCompiler.compile("my.Test","package my; public class Test { public void run() { System.out.println(\"hello world.\"); } }")
Test=JClass(TestClass)
test = Test()
test.run()

def JOverride(*args, **kwargs):
    # Check if called bare
    if len(args)==1 and callable(args[0]):
        object.__setattr__(args[0], "__joverride__", {})
        return args[0]

    # Otherwise apply arguments as needed
    def f(method):
        object.__setattr__(method, "__joverride__", kwargs)
        return method
    return f

_jreturnMap= {
        "void":"",
        "int":"return (Integer)",
        "short":"return (Short)",
        "long":"return (Long)",
        "float":"return (Float)",
        "double":"return (Double)",
        "byte":"return (Byte)",
        "char":"return (Character)",
        "boolean":"return (Boolean)",
}


def JExtendsDefineClass(javaClass, cls):
    overrides = {}
    for k,v in cls.__dict__.items():
        try:
            attr = object.__getattribute__(v, "__joverride__")
            overrides[k]=(v, attr)
        except AttributeError:
            pass

    # Find the methods we are overriding
    jc = javaClass.class_
    methods = []
    for method in jc.getMethods():
        if str(method.getName()) in overrides:
            methods.append(method)
    className = "%s$$Extends$0"%jc.getSimpleName()
    # Build the extension class
    section1 = []
    section2 = []
    section3 = []
    section4 = []
    for ctor in jc.getDeclaredConstructors():
        paramDecl = []
        paramArgs = []

        for p in ctor.getParameters():
            paramDecl.append("%s %s"%(p.getType().getName(), p.getName()))
            paramArgs.append(str(p.getName()))
        paramDecl = ",".join(paramDecl)
        paramArgs = ",".join(paramArgs)
        section2.append("public %s(%s)"%(className, paramDecl))
        section2.append("  { super(%s); }"%(paramArgs))

    i=0
    for method in methods:
        methodName = method.getName()
        paramDecl = []
        paramArgs = []
        paramClass = ['"%s"'%methodName]
        shadowName = "%s$Method$%d"%(methodName,i)
        i+=1

        for p in method.getParameters():
            paramDecl.append("%s %s"%(p.getType().getName(), p.getName()))
            paramArgs.append(str(p.getName()))
            paramClass.append("%s.class"%p.getType().getName())
        paramDecl = ",".join(paramDecl)
        paramArgs = ",".join(paramArgs)
        paramClass = ",".join(paramClass)

        retType = str(method.getReturnType())
        ret = _jreturnMap.get(retType, "return ")
        if retType != "void":
            ret2 = " return"
        else:
            ret2 = ""

        section4.append("private java.lang.reflect.Method %s;"%shadowName);
        section3.append("    %s = %s.class.getMethod(%s);"%(shadowName, jc.getName(),paramClass))

        section2.append("public %s %s(%s) {"%(method.getReturnType(),methodName, paramDecl))
        section2.append("  try {%s _handler.invoke(this, %s, new Object[] {%s}); }"
                %(ret, shadowName, paramArgs))
        section2.append("  catch (UnsupportedOperationException ex) {%s super.%s(%s); }"
                %(ret2, methodName, paramArgs))
        section2.append("  catch (Throwable ex) { throw new RuntimeException(ex); }")

    section1.append("package %s;"%jc.getPackage().getName())
    section1.append("public class %s extends %s {"%(className, jc.getName()))
    section1.append("public java.lang.reflect.InvocationHandler _hander = null;")
    section1.append("\n".join(section4))
    section1.append("  { try {")
    section1.append("\n".join(section3))
    section1.append("    } catch (Exception ex) {throw new RuntimeException(ex); } ")
    section1.append("  }")
    section1.append("\n".join(section2))
    section1.append("}")
    print("\n".join(section1))



def JExtends(javaClass, *args, **kwargs):
    def f(cls):
        JExtendsDefineClass(javaClass, cls)
        return cls
    return f

from java.util import ArrayList

@JExtends(ArrayList)
class MyList(object):

    @JOverride
    def add(super, *args):
        pass



