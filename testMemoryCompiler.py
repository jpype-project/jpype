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
#print(jpype.JClass(cls))

TestClass=memoryCompiler.compile("my.Test","package my; public class Test { public void run() { System.out.println(\"hello world.\"); } }")
print([i for i in TestClass.getConstructors()])
print([str(i.toString()) for i in TestClass.getDeclaredConstructors()])
Test=JClass(TestClass)

print(dir(Test))
test = Test()
test.run()


for i in range(0,100):
    TestClass=memoryCompiler.compile("my.Test%d"%i,"package my; class Test%d { public void run() { System.out.println(\"hello world.\"); } }"%i)



