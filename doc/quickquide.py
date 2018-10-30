def code(var):
    var = var.replace("<","&lt;")
    var = var.replace(">","&gt;")
    print("<pre>%s</pre>"%var)

def section(Title=None, Desc=None):
    print("<h1>%s</h1>"%Title)
    if Desc:
        print("%s<p>"%Desc)
    print("""
<table border='1' width="100%">
<col width="20%"/><col width="30%"/><col width="30%"/><col width="20%"/>
<thead>
<tr><td>Description</td><td>Java</td><td>Python</td><td>Notes</td></tr></thead>
<tbody>
""")

def endSection():
    print("</table>")

def entry(Desc=None, Java=None, Python=None, Notes=None):
    print("<tr><td>%s</td>"%Desc)
    if not Java: Java = "NA"
    if not Python: Python = "None"
    if not Notes: Notes = ""
    print("<td><pre>%s</pre></td>"%Java)
    print("<td><pre>%s</pre></td>"%Python)
    print("<td>%s</td>"%Notes)
    print("</tr>")

########################################################

print("""
<html>
  <body>
Quick start quide to using JPype.  This quide will show a series of simple examples with the 
corresponding commands in both java and python for using JPype. 
The JPype userguide addition details on the use of the JPype module.
<p>
JPype uses two factory classes (JArray and JClass) to produce class wrappers which can be used to 
create all Java objects.  These serve as both the base class for the corresponding hierarchy and
as the factory to produce new wrappers.  Casting operators are used to construct specify types of Java types
(JObject, JString, JBoolean, JByte, JChar, JShort, JInt, JLong, JFloat, JDouble). Two special
classes serve as the base classes for exceptions (JException) and interfaces (JInterface).
There are a small number of support methods to help in controlling the JVM.  Last, there are 
a few annotations used to create customized wrappers.
<p>
For the purpose of this guide, we will assume that the following classes were defined
in Java.  We will also assume the reader knows enough Java and Python to be 
dangerous.  
<p>
""")

code("""
package org.pkg;

publc class BassClass
{
   public callMember(int i)
   {}
}

public class MyClass extends BaseClass
{
   final public static int CONST_FIELD = 1;
   public static int staticField = 1;
   public int memberField = 2;
   int internalField =3;

   public MyClass() {}
   public MyClass(int i) {}

   public static void callStatic(int i) {}
   public void callMember(int i) {}

   // Python name conflict
   public void pass() {}

   public void throwsException throws java.lang.Exception {}

   // Overloaded methods
   public call(int i) {}
   public call(double d) {}
}
""")

#####################################################################################
section("Starting JPype", 
"""
The hardest thing about using JPype is getting the jars loaded into the JVM.
Java is curiously unfriendly about reporting problems when it is unable to find
a jar.  Instead it will be reported as an ImportError in python.
These patterns will help debug problems regarding jar loading.
<p>
Once the JVM is started Java packages that are within a top level domain (TLD)
are exposed as python modules allowing Java to be treated as part of python.
"""
)
entry("Start Java Virtual Machine (JVM)", None, 
"""
# Import module
import jpype

# Enable Java imports
import jpype.imports

# Pull in types
from jpype.types import *

# Launch the JVM
jvmPath = jpype.getDefaultJVMPath()
jvmArgs = "-Djava.class.path=%" %
  jpype.getClassPath()
jpype.startJVM(jvmPath,jvmArgs)
""","REVISE")

entry("Import default java namespace", None, "import java.lang", "All java.lang.* classes are available.")

entry("Add a set of jars from a directory", None, "jpype.addClassPath('/my/path/*')", "Must happen prior to starting JVM")
entry("Add a specific jar to the classpath",None, "jpype.addClassPath('/my/path/myJar.jar')", "Must happen prior to starting the JVM")

entry("Print JVM CLASSPATH", None, 
"""
from java.lang import System
print(System.getProperty("java.class.path"))
""", "After JVM is started")
endSection()

#####################################################################################
section("Classes/Objects", 
"""
Java classes are presented whereever possible exactly like python classes. The only
major difference is that Java classes and objects are closed and cannot be modified.
As Java is strongly typed, casting operators are used to select specific 
overloads when calling methods.  Classes are either imported using as a module
or loaded with the JClass factory.
""")

# Importing
entry("Import a class", "import org.pkg.MyClass", "from org.pkg import MyClass", "This will report an error if the class is not found.")
entry("Import a class and rename", None, "from org.pkg import MyClass as OurClass", "This will report an error if the class is not found.")
entry("Import multiple classes from a package",None, "from org.pkg import MyClass, AnotherClass", "This will report an error if the classes are not found")
entry("Import a java package for long name access",None, "import org.pkg", "Does not report errors if the package is invalid")
entry("Import a class static","import my.pkg.MyClass.CONST_FIELD", "from org.pkg.MyClass import CONST_FIELD", 'Constants, static fiels, and static methods can be imported.')
entry("Import a class without tld", "import zippy.NonStandard", "NonStandard = JClass('zippy.NonStandard')", "JClass loads any class by name including inner classes.")

# Construction
entry("Construct an object","MyClass myObject = new MyClass(1);","myObject = MyClass(1)")
entry("Constructing a cless with full class name", None, """
import org.pkg 
myObject = org.pkg.MyClass(args)
""")

# Fields
entry("Get a static field", "int var = MyClass.staticField;", "var = MyClass.staticField")
entry("Get a member field", "int var = myObject.memberField;", "var = myObject.memberField")
entry("Set a static field", "MyClass.staticField = 2;", "MyClass.staticField = 2", "Produces error for final fields")
entry("Set a member field", "myObject.memberField = 2;", "myObject.memberField = 2", "Produces error for final fields")

# Methods
entry("Call a static method", "MyClass.callStatic(1);", "MyClass.callStatic(1)")
entry("Call a member method", "myObject.callMember(1);", "myObject.callMember(1)")
entry("Access member with python naming conflict", "myObject.pass()", "myObject.pass_()","Underscore is added during wrapping.")
entry("Checking inheritance", "if (obj instanceof MyClass) {...}", "if (isinstance(obj, MyClass): ...")
entry("Checking if java class wrapper", None, "if (isinstance(obj, JClass): ...")
entry("Checking if java object wrapper", None, "if (isinstance(obj, JObject): ...")

# Casting
entry("Casting to a specific type", "BaseClass b = (BaseClass)myObject;", "b = JObject(myObject, BaseClass)")
endSection()

#####################################################################################
section("Exceptions",
"""
Java exceptions extend from python exceptions and can be dealt with no different 
that Python native exceptions. JException serves as the base class for all Java exceptions.
""")

entry("Catch an exception", 
"""
try {
   myObject.throwsException();
} catch (java.lang.Exception ex)
{ ... }
""", 
"""
try:
    myObject.throwsException()
except java.lang.Exception as ex:
    ...
""")

entry("Throw an exception to Java",
"""
throw new java.lang.Exception("Problem");
"""
,
"""
raise java.lang.Exception("Problem");
"""
)
entry("Checking if java exception wrapper", None, "if (isinstance(obj, JException): ...")
entry("Closeable items",
"""
try (InputStream is = Files.newInputStream(file)
{
  ...
}
""",
"""
with Files.newInputStream(file) as is:
   ...
""")
endSection()

#####################################################################################
section("Primitives",
"""
Most python primitives directly map into Java primitives. However, python does not
have the same primitive types, thus sometimes it is necessary to cast to a specific 
Java primitive type especially if there are 
Java overloads that would otherwise be in conflict.  Each of the Java types are
exposed in JPype (JBoolean, JByte, JChar, JShort, JInt, JLong, JFloat, JDouble).
<p>
Python int is equivalent to Java long.
""")
entry("Casting to hit an overload", "myObject.call((int)v);", "myObject.call(JInt(v))","JInt acts as a casting operator")
entry("Create a primitive array", "int[] array = new int[5]", "array = JArray(JInt)(5)")
entry("Create a primitive array", "int[] array = new int[){1,2,3}", "array = JArray(JInt)([1,2,3])",
        "list, sequences, or np.array can be used to initialize.")
entry("Put a specific primitive type on a list", 
"""
List<Integer> myList 
  = new ArrayList<>();
myList.add(1);
""",
"""
from java.util import ArrayList
myList = ArrayList()
myList.add(JInt(1))
""")

entry("Boxing a primitive", "Integer boxed = 1;", 
"""
boxed = JObject(JInt(1))
""",
"JInt specifies the prmitive type. JObject boxes the primitive.")
endSection()

#####################################################################################

section("Strings",
"""
Java strings are similar to python strings.  They are both immutable and
produce a new string when altered.  Most operations can use Java strings
in place of python strings, with minor exceptions as python strings 
are not completely duck typed.  When comparing or using as dictionary keys
JString should be converted to python.
""")
entry("Create a Java string", 'String javaStr = new String("foo");', 'myStr = JString("foo")', "JString constructs a java.lang.String")
entry("Create a Java string from bytes", 
'''
byte[] b;
String javaStr = new String(b, "UTF-8");
''', 
'''
b= b'foo'
myStr = JString(b, "UTF-8")
''', "All java.lang.String constuctors work.")

entry("Converting Java string", None, "str(javaStr)")
entry("Comparing python and Java strings", None, "str(javaStr) == pyString", "str() converts the object for comparison")
entry("Comparing Java strings", 'javaStr.equals("foo")', 'javaStr == "foo"')
entry("Checking if java string", None, "if (isinstance(obj, JString): ...")
endSection()

#####################################################################################

section("Arrays",
"""
Arrays are create using JArray class factory. They operate like python lists, but they are 
fixed in size.
""")
entry("Create a single dimension array", "MyClass[] array = new MyClass[5];", "array = JArray(MyClass)(5)")
entry("Create a multi dimension array", "MyClass[][] array2 = new MyClass[5][];", "array2 = JArray(MyClass, 2)(5)")
entry("Access an element", "array[0] = new MyClass()", "array[0] = MyClass()")
entry("Size of an array", "array.length", "len(array)")
entry("Convert to python list", None, "pylist = list(array)")
entry("Iterate elements", 
"""
for (MyClass element: array) {...}
""",
"""
for element in array:
  ...
""")
entry("Checking if java array wrapper", None, "if (isinstance(obj, JArray): ...")
endSection()

#####################################################################################

section("Collections",
"""
Java standard containers are available and are overloaded with python syntax where 
possible to operate in a similar fashion to python objects.  It is not
currently possible to specify the template types for generic containers, but
that will be introduced in Java 9.
""")
entry("Import list type", "import java.util.ArrayList;","from java.util import ArrayList")
entry("Construct a list", "List<Integer> myList=new ArrayList<>();","myList=ArrayList()")
entry("Get length of list", "int sz = myList.size();","sz = len(myList)")
entry("Get list item", 'Integer i = myList.get(0)', 'i = myList[0]')
entry("Set list item", 'myList.set(0, 1)', 'myList[0]=Jint(1)', "Casting is required to box primitives to the correct type.")
entry("Iterate list elements", 
"""
for (Integer element: myList) {...}
""",
"""
for element in myList:
  ...
""")

entry("Import map type", "import java.util.HashMap;","from java.util import HashMap")
entry("Construct a map", "Map<String,Integer> myMap=new HashMap<>();","myMap=HashMap()")
entry("Get length of map", "int sz = myMap.size();","sz = len(myMap)")
entry("Get map item", 'Integer i = myMap.get("foo")', 'i = myMap["foo"]')
entry("Set map item", 'myMap.set("foo", 1)', 'myMap["foo"]=Jint(1)', "Casting is required to box primitives to the correct type.")
entry("Iterate map entries", 
"""
for (Map.Entry<String,Integer> e
  : myMap.entrySet()) 
  {...}
""",
"""
for e in myMap.entrySet():
  ...
""")
endSection()

#####################################################################################

section("Reflection",
"""
For operations that are outside the scope of the JPype syntax, Using
Java reflection, any java operation include calling a specific overload
or even accessing private methods and fields.
""")
entry("Access Java reflection class","MyClass.class","MyClass.class_")
entry("Access a private field by name", None, 
"""
cls = myObject.class_
field = cls.getDeclaredField("internalField")
field.setAccessible(True)
field.get()
""")

entry("Accessing a specific overload", None,
"""
cls = MyClass.class_
cls.getDeclaredMethod("call", JInt)
cls.invoke(myObject, JInt(1))
""","types must be exactly specified.")

entry("Convert a java.lang.Class into python wrapper", None, 
"""
# Something returned a java.lang.Class
MyClassJava = getClassMethod()

# Convert to it to Python
MyClass = JClass(myClassJava)
""", "Rarely required unless the class was supplied external such as generics.")

entry("Load a class with a external class loader", 
"""
ClassLoader cl = new ExternalClassLoader();
Class cls = Class.forName("External", True, cl)
""",
"""
cl = ExternalClassLoader()
cls = JClass("External", loader=cl)
""")

entry("Accessing base method implementation",None,
"""
from org.pkg import BaseClass, MyClass
myObject = MyClass(1)
BaseClass.callMember(myObject, 2)
""")
endSection()

section("Implements and Extension",
"""
JPype can implement a Java interface by annotating a python class.  Each
method that is required must be implemented.
<p>
JPype does not support extending a class directly in python.  Where it is
necessary to exend a Java class, it is required to create a Java extension
with an interface for each methods that are to be accessed from python.
For some deployments this may be be an option.  If that is the case, 
the JPype inline compiler can be used to create the dynamic class on the 
fly.
""")

entry("Implement an interface",
"""
public class PyImpl implements MyInterface
{
  public void call() {...}
}
""",
"""
@JImplments(MyInterface)
class PyImpl(object):
    @JOverride
    def call(self):
      pass
""", 
"FIXME this pull request is missing?")

entry("Extending a class",None,
"""
FIXME make pull require for this.
from jpype.compiler import Compiler
comp = Compiler()
MyExtension = comp.compile(\"\"\"

  class MyExtension extends MyClass
  { ... }
\"\"\")
obj = MyExtension()
""", "WIP")

entry("Lambdas", None, None, "Support for use of python function as Java 8 lambda is WIP.")
endSection()

print(
"""
<i>Don't like the formatting? Feel the guide is missing something? Submit a pull request 
at the project page.</i>
  </body>
</html>
""")

