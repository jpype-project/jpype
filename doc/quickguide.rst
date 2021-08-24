
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



.. code-block:: java

    package org.pkg;

    public class BaseClass
    {
       public void callMember(int i)
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

       public void throwsException() throws java.lang.Exception {}

       // Overloaded methods
       public void call(int i) {}
       public void call(double d) {}
    }

Starting JPype
--------------

The hardest thing about using JPype is getting the jars loaded into the JVM.
Java is curiously unfriendly about reporting problems when it is unable to find
a jar.  Instead, it will be reported as an ``ImportError`` in Python.
These patterns will help debug problems with jar loading.

Once the JVM is started Java packages that are within a top level domain (TLD)
are exposed as Python modules allowing Java to be treated as part of Python.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Start Java Virtual        |                                                         | .. code-block:: python                                  |
| Machine (JVM)             |                                                         |                                                         |
|                           |                                                         |     # Import module                                     |
|                           |                                                         |     import jpype                                        |
|                           |                                                         |                                                         |
|                           |                                                         |     # Enable Java imports                               |
|                           |                                                         |     import jpype.imports                                |
|                           |                                                         |                                                         |
|                           |                                                         |     # Pull in types                                     |
|                           |                                                         |     from jpype.types import *                           |
|                           |                                                         |                                                         |
|                           |                                                         |     # Launch the JVM                                    |
|                           |                                                         |     jpype.startJVM()                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Start Java Virtual        |                                                         | .. code-block:: python                                  |
| Machine (JVM) with a      |                                                         |                                                         |
| classpath                 |                                                         |     # Launch the JVM                                    |
|                           |                                                         |     jpype.startJVM(classpath = ['jars/*'])              |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import default Java       |                                                         | .. code-block:: python                                  |
| namespace [1]_            |                                                         |                                                         |
|                           |                                                         |     import java.lang                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Add a set of jars from a  |                                                         | .. code-block:: python                                  |
| directory [2]_            |                                                         |                                                         |
|                           |                                                         |     jpype.addClassPath("/my/path/*")                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Add a specific jar to the |                                                         | .. code-block:: python                                  |
| classpath [2]_            |                                                         |                                                         |
|                           |                                                         |     jpype.addClassPath('/my/path/myJar.jar')            |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Print JVM CLASSPATH [3]_  |                                                         | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |                                                         |     from java.lang import System                        |
|                           |                                                         |     print(System.getProperty("java.class.path"))        |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+

    .. [1] All ``java.lang.*`` classes are available.
    .. [2] Must happen prior to starting the JVM
    .. [3] After JVM is started


Classes/Objects
---------------

Java classes are presented wherever possible similar to Python classes. The
only major difference is that Java classes and objects are closed and cannot be
modified.  As Java is strongly typed, casting operators are used to select
specific overloads when calling methods.  Classes are either imported using a
module, loaded using ``JPackage`` or loaded with the ``JClass`` factory.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Import a class [4]_       | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     import org.pkg.MyClass                              |     from org.pkg import MyClass                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import a class and rename |                                                         | .. code-block:: python                                  |
| [4]_                      |                                                         |                                                         |
|                           |                                                         |     from org.pkg import MyClass as OurClass             |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import multiple classes   |                                                         | .. code-block:: python                                  |
| from a package [5]_       |                                                         |                                                         |
|                           |                                                         |     from org.pkg import MyClass, AnotherClass           |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import a java package for |                                                         | .. code-block:: python                                  |
| long name access [6]_     |                                                         |                                                         |
|                           |                                                         |     import org.pkg                                      |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import a class static     | .. code-block:: java                                    | .. code-block:: python                                  |
| [7]_                      |                                                         |                                                         |
|                           |     import org.pkg.MyClass.CONST_FIELD                  |     from org.pkg.MyClass import CONST_FIELD             |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import a class without    | .. code-block:: java                                    | .. code-block:: python                                  |
| tld [8]_                  |                                                         |                                                         |
|                           |     import zippy.NonStandard                            |     NonStandard = JClass('zippy.NonStandard')           |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Construct an object       | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     MyClass myObject = new MyClass(1);                  |     myObject = MyClass(1)                               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Constructing a class with |                                                         | .. code-block:: python                                  |
| full class name           |                                                         |                                                         |
|                           |                                                         |     import org.pkg                                      |
|                           |                                                         |     myObject = org.pkg.MyClass(args)                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get a static field        | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     int var = MyClass.staticField;                      |     var = MyClass.staticField                           |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get a member field        | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     int var = myObject.memberField;                     |     var = myObject.memberField                          |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Set a static field [9]_   | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     MyClass.staticField = 2;                            |     MyClass.staticField = 2                             |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Set a member field [9]_   | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     myObject.memberField = 2;                           |     myObject.memberField = 2                            |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Call a static method      | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     MyClass.callStatic(1);                              |     MyClass.callStatic(1)                               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Call a member method      | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     myObject.callMember(1);                             |     myObject.callMember(1)                              |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Access member with Python | .. code-block:: java                                    | .. code-block:: python                                  |
| naming conflict [10]_     |                                                         |                                                         |
|                           |     myObject.pass()                                     |     myObject.pass_()                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking inheritance      | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     if (obj instanceof MyClass) {...}                   |     if (isinstance(obj, MyClass): ...                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if Java class    |                                                         | .. code-block:: python                                  |
| wrapper                   |                                                         |                                                         |
|                           |                                                         |     if (isinstance(obj, JClass): ...                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if Java object   |                                                         | .. code-block:: python                                  |
| wrapper                   |                                                         |                                                         |
|                           |                                                         |     if (isinstance(obj, JObject): ...                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Casting to a specific     | .. code-block:: java                                    | .. code-block:: python                                  |
| type [11]_                |                                                         |                                                         |
|                           |     BaseClass b = (BaseClass)myObject;                  |     b = (BaseClass) @ myObject                          |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+

    .. [4] This will report an error if the class is not found.
    .. [5] This will report an error if the classes are not found.
    .. [6] Does not report errors if the package is invalid.
    .. [7] Constants, static fields, and static methods can be imported.
    .. [8] ``JClass`` loads any class by name including inner classes.
    .. [9] This produces an error for final fields.
    .. [10] Underscore is added during wrapping.
    .. [11] Matmul(@) is used as the casting operator.


Exceptions
----------

Java exceptions extend from Python exceptions and can be dealt with in the same
way as Python native exceptions. JException serves as the base class for all
Java exceptions.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Catch an exception        | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     try {                                               |     try:                                                |
|                           |        myObject.throwsException();                      |         myObject.throwsException()                      |
|                           |     } catch (java.lang.Exception ex)                    |     except java.lang.Exception as ex:                   |
|                           |     { ... }                                             |         ...                                             |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Throw an exception to     | .. code-block:: java                                    | .. code-block:: python                                  |
| Java                      |                                                         |                                                         |
|                           |   throw new java.lang.Exception(                        |   raise java.lang.Exception(                            |
|                           |           "Problem");                                   |           "Problem")                                    |
|                           |                                                         |                                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if Java          |                                                         | .. code-block:: python                                  |
| exception wrapper         |                                                         |                                                         |
|                           |                                                         |     if (isinstance(obj, JException): ...                |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Closeable items           | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     try (InputStream is                                 |     with Files.newInputStream(file) as is:              |
|                           |       = Files.newInputStream(file))                     |        ...                                              |
|                           |     { ... }                                             |                                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+



Primitives
----------

Most Python primitives directly map into Java primitives. However, Python does
not have the same primitive types, and it is necessary to cast to a
specific Java primitive type whenever there are Java overloads that would
otherwise be in conflict.  Each of the Java types are exposed in JPype
(``JBoolean``, ``JByte``, ``JChar``, ``JShort``, ``JInt``, ``JLong``,
``JFloat``, ``JDouble``).



+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Casting to hit an         | .. code-block:: java                                    | .. code-block:: python                                  |
| overload [12]_            |                                                         |                                                         |
|                           |     myObject.call((int)v);                              |     myObject.call(JInt(v))                              |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create a primitive array  | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     int[] array = new int[5]                            |     array = JInt[5]                                     |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create a rectangular      | .. code-block:: java                                    | .. code-block:: python                                  |
| primitive array           |                                                         |                                                         |
|                           |     int[][] array = new int[5][10]                      |     array = JInt[5, 10]                                 |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create an array of arrays | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     int[][] array = new int[5][]                        |     array = JInt[5, :]                                  |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create an initialized     | .. code-block:: java                                    | .. code-block:: python                                  |
| primitive array [13]_     |                                                         |                                                         |
|                           |     int[] array = new int[]{1,2,3}                      |     array = JInt[:]([1,2,3])                            |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create an initialized     | .. code-block:: java                                    | .. code-block:: python                                  |
| boxed array [13]_         |                                                         |                                                         |
|                           |     Integer[] array = new Integer[]{1,2,3}              |     array = java.lang.Integer[:]([1,2,3])               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Put a specific primitive  | .. code-block:: java                                    | .. code-block:: python                                  |
| type on a list            |                                                         |                                                         |
|                           |     List<Integer> myList                                |     from java.util import ArrayList                     |
|                           |       = new ArrayList<>();                              |     myList = ArrayList()                                |
|                           |     myList.add(1);                                      |     myList.add(JInt(1))                                 |
|                           |                                                         |                                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Boxing a primitive [14]_  | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     Integer boxed = 1;                                  |     boxed = JObject(JInt(1))                            |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+

    .. [12] ``JInt`` acts as a casting operator
    .. [13] list, sequences, or np.array can be used to initialize.
    .. [14] ``JInt`` specifies the prmitive type. ``JObject`` boxes the primitive.


Strings
-------

Java strings are similar to Python strings.  They are both immutable and
produce a new string when altered.  Most operations can use Java strings
in place of Python strings, with minor exceptions as Python strings 
are not completely duck typed.  When comparing or using as dictionary keys,
all JString objects should be converted to Python.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Create a Java string      | .. code-block:: java                                    | .. code-block:: python                                  |
| [15]_                     |                                                         |                                                         |
|                           |     String javaStr = new String("foo");                 |     myStr = JString("foo")                              |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create a Java string from | .. code-block:: java                                    | .. code-block:: python                                  |
| bytes [16]_               |                                                         |                                                         |
|                           |     byte[] b;                                           |     b= b'foo'                                           |
|                           |     String javaStr = new String(b, "UTF-8");            |     myStr = JString(b, "UTF-8")                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Converting Java string    |                                                         | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |                                                         |     str(javaStr)                                        |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Comparing Python and Java |                                                         | .. code-block:: python                                  |
| strings [17]_             |                                                         |                                                         |
|                           |                                                         |     str(javaStr) == pyString                            |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Comparing Java strings    | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     javaStr.equals("foo")                               |     javaStr == "foo"                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if Java string   |                                                         | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |                                                         |     if (isinstance(obj, JString): ...                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+

    .. [15] ``JString`` constructs a ``java.lang.String``
    .. [16] All ``java.lang.String`` constuctors work.
    .. [17] ``str()`` converts the object for comparison


Arrays
------

Arrays are create using the JArray class factory. They operate like Python lists, but they are 
fixed in size.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Create a single dimension | .. code-block:: java                                    | .. code-block:: python                                  |
| array                     |                                                         |                                                         |
|                           |     MyClass[] array = new MyClass[5];                   |     array = MyClass[5]                                  |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create a multi dimension  | .. code-block:: java                                    | .. code-block:: python                                  |
| array (old)               |                                                         |                                                         |
|                           |     MyClass[][] array2 = new MyClass[5][];              |     array2 = JArray(MyClass, 2)(5)                      |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create a multi dimension  | .. code-block:: java                                    | .. code-block:: python                                  |
| array (new)               |                                                         |                                                         |
|                           |     MyClass[][] array2 = new MyClass[5][];              |     array2 = MyClass[5,:]                               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Access an element         | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     array[0] = new MyClass()                            |     array[0] = MyClass()                                |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Size of an array          | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     array.length                                        |     len(array)                                          |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get last element          | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     MyClass a = array[array.length];                    |     a = array[-1]                                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Slice an array [18]_      |                                                         | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |                                                         |     a = array[2:5]                                      |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Clone an array            | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     MyClass[] a = array.clone();                        |     a = array.clone()                                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Convert to Python list    |                                                         | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |                                                         |     pylist = list(array)                                |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Iterate elements          | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     for (MyClass element: array)                        |     for element in array:                               |
|                           |     {...}                                               |       ...                                               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if java array    |                                                         | .. code-block:: python                                  |
| wrapper                   |                                                         |                                                         |
|                           |                                                         |     if (isinstance(obj, JArray): ...                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+

    .. [18] A Slice is a view and changes will be reflected on original.  Slices passed to Java will clone.


Collections
-----------

Java standard containers are available and are overloaded with Python syntax where 
possible to operate in a similar fashion to Python objects.  


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Import list type          | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     import java.util.ArrayList;                         |     from java.util import ArrayList                     |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Construct a list          | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     List<Integer> myList=new ArrayList<>();             |     myList=ArrayList()                                  |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get length of list        | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     int sz = myList.size();                             |     sz = len(myList)                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get list item             | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     Integer i = myList.get(0)                           |     i = myList[0]                                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Set list item [19]_       | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     myList.set(0, 1)                                    |     myList[0]=Jint(1)                                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Iterate list elements     | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     for (Integer element: myList)                       |     for element in myList:                              |
|                           |     {...}                                               |       ...                                               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import map type           | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     import java.util.HashMap;                           |     from java.util import HashMap                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Construct a map           | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     Map<String,Integer> myMap = new HashMap<>();        |     myMap = HashMap()                                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get length of map         | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     int sz = myMap.size();                              |     sz = len(myMap)                                     |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get map item              | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     Integer i = myMap.get("foo")                        |     i = myMap["foo"]                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Set map item [19]_        | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     myMap.set("foo", 1)                                 |     myMap["foo"] = Jint(1)                              |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Iterate map entries       | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     for (Map.Entry<String,Integer> e                    |     for e in myMap.entrySet():                          |
|                           |       : myMap.entrySet())                               |       ...                                               |
|                           |       {...}                                             |                                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+

    .. [19] Casting is required to box primitives to the correct type.


Reflection
----------

Java reflection can be used to access operations that are outside the scope of
the JPype syntax.  This includes calling a specific overload or even accessing
private methods and fields.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Access Java reflection    | .. code-block:: java                                    | .. code-block:: python                                  |
| class                     |                                                         |                                                         |
|                           |     MyClass.class                                       |     MyClass.class_                                      |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Access a private field by |                                                         | .. code-block:: python                                  |
| name [20]_                |                                                         |                                                         |
|                           |                                                         |     cls = myObject.class_                               |
|                           |                                                         |     field = cls.getDeclaredField(                       |
|                           |                                                         |         "internalField")                                |
|                           |                                                         |     field.setAccessible(True)                           |
|                           |                                                         |     field.get()                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Accessing a specific      |                                                         | .. code-block:: python                                  |
| overload [21]_            |                                                         |                                                         |
|                           |                                                         |     cls = MyClass.class_                                |
|                           |                                                         |     cls.getDeclaredMethod("call", JInt)                 |
|                           |                                                         |     cls.invoke(myObject, JInt(1))                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Convert a                 |                                                         | .. code-block:: python                                  |
| ``java.lang.Class`` into  |                                                         |                                                         |
| Python wrapper [22]_      |                                                         |     # Something returned a java.lang.Class              |
|                           |                                                         |     MyClassJava = getClassMethod()                      |
|                           |                                                         |                                                         |
|                           |                                                         |     # Convert to it to Python                           |
|                           |                                                         |     MyClass = JClass(myClassJava)                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Load a class with a       | .. code-block:: java                                    | .. code-block:: python                                  |
| external class loader     |                                                         |                                                         |
|                           |     ClassLoader cl                                      |     cl = ExternalClassLoader()                          |
|                           |       = new ExternalClassLoader();                      |     cls = JClass("External", loader=cl)                 |
|                           |     Class cls                                           |                                                         |
|                           |       = Class.forName("External",                       |                                                         |
|                           |                       True, cl)                         |                                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Accessing base method     |                                                         | .. code-block:: python                                  |
| implementation            |                                                         |                                                         |
|                           |                                                         |     from org.pkg import \                               |
|                           |                                                         |             BaseClass, MyClass                          |
|                           |                                                         |     myObject = MyClass(1)                               |
|                           |                                                         |     BaseClass.callMember(myObject, 2)                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+

    .. [20] This is prohibited after Java 8
    .. [21] types must be exactly specified.
    .. [22] Rarely required unless the class was supplied external such as generics.


Implements and Extension
------------------------

JPype can implement a Java interface by annotating a Python class.  Each
method that is required must be implemented.

JPype does not support extending a class directly in Python.  Where it is
necessary to exend a Java class, it is required to create a Java extension
with an interface for each methods that are to be accessed from Python.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Implement an interface    | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     public class PyImpl                                 |     @JImplements(MyInterface)                           |
|                           |       implements MyInterface                            |     class PyImpl(object):                               |
|                           |     {                                                   |         @JOverride                                      |
|                           |       public void call()                                |         def call(self):                                 |
|                           |       {...}                                             |           pass                                          |
|                           |     }                                                   |                                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         | None                                                    |
| Extending classes [23]_   |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Lambdas [24]_             | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |                                                         |                                                         |
|                           |     DoubleUnaryOperator u = (p->p*2);                   |     u=DoubleUnaryOperator@(lambda x: x*2)               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+

    .. [23] Support for use of Python function as Java 8 lambda is WIP.
    .. [24] Any Java functional interface can take a lambda or callable.



Don't like the formatting? Feel the guide is missing something? Submit a pull request 
at the project page.

