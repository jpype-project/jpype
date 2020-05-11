import textwrap

footnotes = []
footnotecounter = 1


def section(Title=None, Desc=None):
    print("%s" % Title)
    print("-" * len(Title))
    if Desc:
        print("%s" % Desc)
        print()
    print("+" + ("-" * 27) + "+" + ("-" * 57) + "+" + ("-" * 57) + "+")
    print("| %-25s | %-55s | %-55s |" %
          ("Description", "Java", "Python"))
    print("+" + ("=" * 27) + "+" + ("=" * 57) + "+" + ("=" * 57) + "+")


def endSection():
    print()
    global footnotes, footnotecounter
    for i in range(0, len(footnotes)):
        print("    .. [%d] %s" % (i + footnotecounter, footnotes[i]))
    footnotecounter += len(footnotes)
    footnotes = []
    print()
    print()


def java(s):
    return """
.. code-block:: java

    %s
""" % s


def python(s):
    return """
.. code-block:: python

    %s
""" % s


def entry(Desc=None, Java=None, Python=None, Notes=None):
    global footnotes, footnotecounter
    if not Java:
        Java = ""
    if not Python:
        Python = "None"
    if Notes:
        global footnotes
        if Notes in footnotes:
            Desc += " [%d]_" % (footnotes.index(Notes) + footnotecounter)
        else:
            Desc += " [%d]_" % (len(footnotes) + footnotecounter)
            footnotes.append(Notes)
    DescLines = textwrap.wrap(Desc, 25)
    DescLines.insert(0, "")
    JavaLines = Java.split("\n")
    PythonLines = Python.split("\n")

    while (len(DescLines) > 0 or len(JavaLines) > 0 or len(PythonLines) > 0):
        if len(DescLines) > 0:
            print("| %-25s |" % DescLines.pop(0), end="")
        else:
            print("| %-25s |" % "", end="")

        if len(JavaLines) > 0:
            print(" %-55s |" % JavaLines.pop(0), end="")
        else:
            print(" %-55s |" % "", end="")

        if len(PythonLines) > 0:
            print(" %-55s |" % PythonLines.pop(0))
        else:
            print(" %-55s |" % "")

    print("+" + ("-" * 27) + "+" + ("-" * 57) + "+" + ("-" * 57) + "+")

########################################################


print("""
Java QuickStart Guide
=====================

This is a quick start guide to using JPype with Java.  This guide will show a
series of snippets with the corresponding commands in both Java and Python for
using JPype.  The :doc:`userguide` and :doc:`api` have additional details on
the use of the JPype module.

JPype uses two factory classes (``JArray`` and ``JClass``) to produce class
wrappers which can be used to create all Java objects.  These serve as both the
base class for the corresponding hierarchy and as the factory to produce new
wrappers.  Casting operators are used to construct specify types of Java types
(``JObject``, ``JString``, ``JBoolean``, ``JByte``, ``JChar``, ``JShort``,
``JInt``, ``JLong``, ``JFloat``, ``JDouble``). Two special classes serve as the
base classes for exceptions (``JException``) and interfaces (``JInterface``).
There are a small number of support methods to help in controlling the JVM.
Lastly, there are a few annotations used to create customized wrappers.

For the purpose of this guide, we will assume that the following classes were
defined in Java.  We will also assume the reader knows enough Java and Python
to be dangerous.  

""")

print("""
.. code-block:: java

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
a jar.  Instead, it will be reported as an ``ImportError`` in Python.
These patterns will help debug problems with jar loading.

Once the JVM is started Java packages that are within a top level domain (TLD)
are exposed as Python modules allowing Java to be treated as part of Python.
"""
        )
entry("Start Java Virtual Machine (JVM)", None,
      """
.. code-block:: python

    # Import module
    import jpype

    # Enable Java imports
    import jpype.imports

    # Pull in types
    from jpype.types import *

    # Launch the JVM
    jpype.startJVM()
""")
entry("Start Java Virtual Machine (JVM) with a classpath", None,
      """
.. code-block:: python
    # Launch the JVM
    jpype.startJVM(classpath = ['jars/*'])
""")


entry("Import default Java namespace", None,
      python("import java.lang"),
      "All ``java.lang.*`` classes are available.")

entry("Add a set of jars from a directory", None,
      python('jpype.addClassPath("/my/path/*")'),
      "Must happen prior to starting the JVM")

entry("Add a specific jar to the classpath", None,
      python("jpype.addClassPath('/my/path/myJar.jar')"),
      "Must happen prior to starting the JVM")

entry("Print JVM CLASSPATH", None,
      """
.. code-block:: python

    from java.lang import System
    print(System.getProperty("java.class.path"))
""", "After JVM is started")
endSection()

#####################################################################################
section("Classes/Objects",
        """
Java classes are presented wherever possible similar to Python classes. The
only major difference is that Java classes and objects are closed and cannot be
modified.  As Java is strongly typed, casting operators are used to select
specific overloads when calling methods.  Classes are either imported using a
module, loaded using ``JPackage`` or loaded with the ``JClass`` factory.
""")

# Importing
entry("Import a class",
      java("import org.pkg.MyClass"),
      python("from org.pkg import MyClass"),
      "This will report an error if the class is not found.")

entry("Import a class and rename", None,
      python("from org.pkg import MyClass as OurClass"),
      "This will report an error if the class is not found.")

entry("Import multiple classes from a package", None,
      python("from org.pkg import MyClass, AnotherClass"),
      "This will report an error if the classes are not found.")

entry("Import a java package for long name access", None,
      python("import org.pkg"),
      "Does not report errors if the package is invalid.")

entry("Import a class static",
      java("import org.pkg.MyClass.CONST_FIELD"),
      python("from org.pkg.MyClass import CONST_FIELD"),
      'Constants, static fields, and static methods can be imported.')

entry("Import a class without tld",
      java("import zippy.NonStandard"),
      python("NonStandard = JClass('zippy.NonStandard')"),
      "``JClass`` loads any class by name including inner classes.")

# Construction
entry("Construct an object",
      java("MyClass myObject = new MyClass(1);"),
      python("myObject = MyClass(1)"))

entry("Constructing a cless with full class name", None, """
.. code-block:: python

    import org.pkg 
    myObject = org.pkg.MyClass(args)
""")

# Fields
entry("Get a static field",
      java("int var = MyClass.staticField;"),
      python("var = MyClass.staticField"))
entry("Get a member field",
      java("int var = myObject.memberField;"),
      python("var = myObject.memberField"))
entry("Set a static field",
      java("MyClass.staticField = 2;"),
      python("MyClass.staticField = 2"),
      "This produces an error for final fields.")
entry("Set a member field",
      java("myObject.memberField = 2;"),
      python("myObject.memberField = 2"),
      "This produces an error for final fields.")

# Methods
entry("Call a static method",
      java("MyClass.callStatic(1);"),
      python("MyClass.callStatic(1)"))
entry("Call a member method",
      java("myObject.callMember(1);"),
      python("myObject.callMember(1)"))
entry("Access member with Python naming conflict",
      java("myObject.pass()"),
      python("myObject.pass_()"),
      "Underscore is added during wrapping.")
entry("Checking inheritance",
      java("if (obj instanceof MyClass) {...}"),
      python("if (isinstance(obj, MyClass): ..."))
entry("Checking if Java class wrapper", None,
      python("if (isinstance(obj, JClass): ..."))
entry("Checking if Java object wrapper", None,
      python("if (isinstance(obj, JObject): ..."))

# Casting
entry("Casting to a specific type",
      java("BaseClass b = (BaseClass)myObject;"),
      python("b = JObject(myObject, BaseClass)"))
endSection()

#####################################################################################
section("Exceptions",
        """
Java exceptions extend from Python exceptions and can be dealt with in the same
way as Python native exceptions. JException serves as the base class for all
Java exceptions.
""")

entry("Catch an exception",
      """
.. code-block:: java

    try {
       myObject.throwsException();
    } catch (java.lang.Exception ex)
    { ... }
""",
      """
.. code-block:: python

    try:
        myObject.throwsException()
    except java.lang.Exception as ex:
        ...
""")

entry("Throw an exception to Java",
      """
.. code-block:: java

  throw new java.lang.Exception(
          "Problem");
          
""",
      """
.. code-block:: python

  raise java.lang.Exception(
          "Problem")

""")

entry("Checking if Java exception wrapper", None,
      python('if (isinstance(obj, JException): ...'))

entry("Closeable items",
      """
.. code-block:: java

    try (InputStream is 
      = Files.newInputStream(file))
    { ... }
""",
      """
.. code-block:: python

    with Files.newInputStream(file) as is:
       ...
""")
endSection()

#####################################################################################
section("Primitives",
        """
Most Python primitives directly map into Java primitives. However, Python does
not have the same primitive types, and it is necessary to cast to a
specific Java primitive type whenever there are Java overloads that would
otherwise be in conflict.  Each of the Java types are exposed in JPype
(``JBoolean``, ``JByte``, ``JChar``, ``JShort``, ``JInt``, ``JLong``,
``JFloat``, ``JDouble``).

""")
entry("Casting to hit an overload",
      java("myObject.call((int)v);"),
      python("myObject.call(JInt(v))"),
      "``JInt`` acts as a casting operator")

entry("Create a primitive array",
      java("int[] array = new int[5]"),
      python("array = JArray(JInt)(5)"))

entry("Create an initialized primitive array",
      java("int[] array = new int[]{1,2,3}"),
      python("array = JArray(JInt)([1,2,3])"),
      "list, sequences, or np.array can be used to initialize.")
entry("Put a specific primitive type on a list",
      """
.. code-block:: java

    List<Integer> myList 
      = new ArrayList<>();
    myList.add(1);

""",
      """
.. code-block:: python

    from java.util import ArrayList
    myList = ArrayList()
    myList.add(JInt(1))

""")

entry("Boxing a primitive",
      java("Integer boxed = 1;"),
      python("boxed = JObject(JInt(1))"),
      "``JInt`` specifies the prmitive type. ``JObject`` boxes the primitive.")
endSection()

#####################################################################################

section("Strings",
        """
Java strings are similar to Python strings.  They are both immutable and
produce a new string when altered.  Most operations can use Java strings
in place of Python strings, with minor exceptions as Python strings 
are not completely duck typed.  When comparing or using as dictionary keys,
all JString objects should be converted to Python.
""")
entry("Create a Java string", java('String javaStr = new String("foo");'), python(
    'myStr = JString("foo")'), "``JString`` constructs a ``java.lang.String``")
entry("Create a Java string from bytes",
      '''
.. code-block:: java

    byte[] b;
    String javaStr = new String(b, "UTF-8");
''',
      '''
.. code-block:: python

    b= b'foo'
    myStr = JString(b, "UTF-8")
''', "All ``java.lang.String`` constuctors work.")

entry("Converting Java string", None, python("str(javaStr)"))
entry("Comparing Python and Java strings", None, python(
    "str(javaStr) == pyString"), "``str()`` converts the object for comparison")
entry("Comparing Java strings", java(
    'javaStr.equals("foo")'), python('javaStr == "foo"'))
entry("Checking if java string", None, python(
    "if (isinstance(obj, JString): ..."))
endSection()

#####################################################################################

section("Arrays",
        """
Arrays are create using the JArray class factory. They operate like Python lists, but they are 
fixed in size.
""")
entry("Create a single dimension array",
      java("MyClass[] array = new MyClass[5];"),
      python("array = JArray(MyClass)(5)"))
entry("Create a multi  dimension array",
      java("MyClass[][] array2 = new MyClass[5][];"),
      python("array2 = JArray(MyClass, 2)(5)"))
entry("Access an element",
      java("array[0] = new MyClass()"),
      python("array[0] = MyClass()"))
entry("Size of an array",
      java("array.length"),
      python("len(array)"))
entry("Convert to Python list", None,
      python("pylist = list(array)"))

entry("Iterate elements",
      """
.. code-block:: java

    for (MyClass element: array) 
    {...}
""",
      """
.. code-block:: python

    for element in array:
      ...
""")

entry("Checking if java array wrapper", None,
      python("if (isinstance(obj, JArray): ..."))

endSection()

#####################################################################################

section("Collections",
        """
Java standard containers are available and are overloaded with Python syntax where 
possible to operate in a similar fashion to Python objects.  
""")
entry("Import list type",
      java("import java.util.ArrayList;"),
      python("from java.util import ArrayList"))
entry("Construct a list",
      java("List<Integer> myList=new ArrayList<>();"),
      python("myList=ArrayList()"))
entry("Get length of list",
      java("int sz = myList.size();"),
      python("sz = len(myList)"))
entry("Get list item",
      java('Integer i = myList.get(0)'),
      python('i = myList[0]'))
entry("Set list item",
      java('myList.set(0, 1)'),
      python('myList[0]=Jint(1)'),
      "Casting is required to box primitives to the correct type.")
entry("Iterate list elements",
      """
.. code-block:: java

    for (Integer element: myList) 
    {...}
""",
      """
.. code-block:: python

    for element in myList:
      ...
""")

entry("Import map type",
      java("import java.util.HashMap;"),
      python("from java.util import HashMap"))

entry("Construct a map",
      java("Map<String,Integer> myMap=new HashMap<>();"),
      python("myMap=HashMap()"))

entry("Get length of map",
      java("int sz = myMap.size();"),
      python("sz = len(myMap)"))

entry("Get map item",
      java('Integer i = myMap.get("foo")'),
      python('i = myMap["foo"]'))

entry("Set map item",
      java('myMap.set("foo", 1)'),
      python('myMap["foo"]=Jint(1)'),
      "Casting is required to box primitives to the correct type.")

entry("Iterate map entries",
      """
.. code-block:: java

    for (Map.Entry<String,Integer> e
      : myMap.entrySet()) 
      {...}
""",
      """
.. code-block:: python

    for e in myMap.entrySet():
      ...
""")
endSection()

#####################################################################################

section("Reflection",
        """
Java reflection can be used to access operations that are outside the scope of
the JPype syntax.  This includes calling a specific overload or even accessing
private methods and fields.
""")
entry("Access Java reflection class",
      java("MyClass.class"),
      python("MyClass.class_"))

entry("Access a private field by name", None,
      """
.. code-block:: python

    cls = myObject.class_
    field = cls.getDeclaredField(
        "internalField")
    field.setAccessible(True)
    field.get()
""", """This is prohibited after Java 8""")

entry("Accessing a specific overload", None,
      """
.. code-block:: python

    cls = MyClass.class_
    cls.getDeclaredMethod("call", JInt)
    cls.invoke(myObject, JInt(1))
""", "types must be exactly specified.")

entry("Convert a ``java.lang.Class`` into Python wrapper", None,
      """
.. code-block:: python

    # Something returned a java.lang.Class
    MyClassJava = getClassMethod()

    # Convert to it to Python
    MyClass = JClass(myClassJava)
""", "Rarely required unless the class was supplied external such as generics.")

entry("Load a class with a external class loader",
      """
.. code-block:: java

    ClassLoader cl 
      = new ExternalClassLoader();
    Class cls 
      = Class.forName("External", 
                      True, cl)
""",
      """
.. code-block:: python

    cl = ExternalClassLoader()
    cls = JClass("External", loader=cl)
""")

entry("Accessing base method implementation", None,
      """
.. code-block:: python

    from org.pkg import \\
            BaseClass, MyClass
    myObject = MyClass(1)
    BaseClass.callMember(myObject, 2)
""")
endSection()

section("Implements and Extension",
        """
JPype can implement a Java interface by annotating a Python class.  Each
method that is required must be implemented.

JPype does not support extending a class directly in Python.  Where it is
necessary to exend a Java class, it is required to create a Java extension
with an interface for each methods that are to be accessed from Python.
""")

entry("Implement an interface",
      """
.. code-block:: java

    public class PyImpl 
      implements MyInterface
    {
      public void call() 
      {...}
    }
""",
      """
.. code-block:: python

    @JImplements(MyInterface)
    class PyImpl(object):
        @JOverride
        def call(self):
          pass
""",
      "")

entry("Extending classes", None, None,
      """Support for use of Python function as Java 8 lambda is WIP.""")

entry("Lambdas", None, None,
      """Support for use of Python function as Java 8 lambda is WIP.""")
endSection()

print(
    """
Don't like the formatting? Feel the guide is missing something? Submit a pull request 
at the project page.
""")
