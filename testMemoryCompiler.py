import jpype
import jpype.imports
import _jpype

jpype.startJVM(jpype.getDefaultJVMPath())
from java.lang import Class

# Fetch the internal class loader for jpype
cl = jpype.JClass("org.jpype.classloader.JPypeClassLoader")

# Get a class from jpype class loader
cls = Class.forName("org.jpype.compiler.MemoryCompiler", False, cl.getInstance())

# Convert it to a python class
MemoryCompiler = jpype.JClass(cls.__javaclass__)
#print(jpype.JClass(cls))


