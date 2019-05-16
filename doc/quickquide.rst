
JPype QuickStart Guide
======================

Quick start quide to using JPype.  This quide will show a series of simple examples with the 
corresponding commands in both java and python for using JPype. 
The JPype userguide addition details on the use of the JPype module.

JPype uses two factory classes (``JArray`` and ``JClass``) to produce class 
wrappers which can be used to create all Java objects.  These serve as both 
the base class for the corresponding hierarchy and as the factory to produce 
new wrappers.  Casting operators are used to construct specify types of Java
types (``JObject``, ``JString``, ``JBoolean``, ``JByte``, ``JChar``, 
``JShort``, ``JInt``, ``JLong``, ``JFloat``, ``JDouble``). Two special
classes serve as the base classes for exceptions (``JException``) and 
interfaces (``JInterface``).
There are a small number of support methods to help in controlling the JVM.  
Last, there are a few annotations used to create customized wrappers.

For the purpose of this guide, we will assume that the following classes were defined
in Java.  We will also assume the reader knows enough Java and Python to be 
dangerous.  



.. code-block:: java
    package org.pkg;

    public class BassClass
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

Starting JPype
--------------

The hardest thing about using JPype is getting the jars loaded into the JVM.
Java is curiously unfriendly about reporting problems when it is unable to find
a jar.  Instead, it will be reported as an ``ImportError`` in python.
These patterns will help debug problems regarding jar loading.

Once the JVM is started Java packages that are within a top level domain (TLD)
are exposed as python modules allowing Java to be treated as part of python.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
| Description               | Java                                                    | Python                                                  | Notes                                    |
+===========================+=========================================================+=========================================================+==========================================+
|                           |                                                         |                                                         |                                          |
| Start Java Virtual        |                                                         | .. code-block:: python                                  | REVISE                                   |
| Machine (JVM)             |                                                         |     # Import module                                     |                                          |
|                           |                                                         |     import jpype                                        |                                          |
|                           |                                                         |                                                         |                                          |
|                           |                                                         |     # Enable Java imports                               |                                          |
|                           |                                                         |     import jpype.imports                                |                                          |
|                           |                                                         |                                                         |                                          |
|                           |                                                         |     # Pull in types                                     |                                          |
|                           |                                                         |     from jpype.types import *                           |                                          |
|                           |                                                         |                                                         |                                          |
|                           |                                                         |     # Launch the JVM                                    |                                          |
|                           |                                                         |     jvmPath = jpype.getDefaultJVMPath()                 |                                          |
|                           |                                                         |     jvmArgs = "-Djava.class.path=%" %                   |                                          |
|                           |                                                         |       jpype.getClassPath()                              |                                          |
|                           |                                                         |     jpype.startJVM(jvmPath,jvmArgs)                     |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Import default Java       |                                                         | .. code-block:: python                                  | All java.lang.* classes are available.   |
| namespace                 |                                                         |     import java.lang                                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Add a set of jars         |                                                         | .. code-block:: python                                  | Must happen prior to starting JVM        |
| from a directory          |                                                         |     jpype.addClassPath('/my/path/\*')                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Add a specific jar        |                                                         | .. code-block:: python                                  | Must happen prior to starting the JVM    |
| to the classpath          |                                                         |     jpype.addClassPath('/my/path/myJar.jar')            |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Print JVM CLASSPATH       |                                                         | .. code-block:: python                                  | After JVM is started                     |
|                           |                                                         |     from java.lang import System                        |                                          |
|                           |                                                         |     print(System.getProperty("java.class.path"))        |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+


Classes/Objects
---------------

Java classes are presented whereever possible exactly like Python classes. The only
major difference is that Java classes and objects are closed and cannot be modified.
As Java is strongly typed, casting operators are used to select specific 
overloads when calling methods.  Classes are either imported using as a module
or loaded with the ``JClass`` factory.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
| Description               | Java                                                    | Python                                                  | Notes                                    |
+===========================+=========================================================+=========================================================+==========================================+
|                           |                                                         |                                                         |                                          |
| Import a class            | .. code-block: java                                     | .. code-block: java                                     | This will report an error                |
|                           |     import org.pkg.MyClass                              |     from org.pkg import MyClass                         | if the class                             |
|                           |                                                         |                                                         | is not found.                            |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Import a class            |                                                         | .. code-block: java                                     | This will report an error if the class   |
| and rename                |                                                         |     from org.pkg import MyClass as OurClass             | is not found.                            |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Import multiple classes   |                                                         | .. code-block: java                                     | This will report an error if the classes |
| from a package            |                                                         |     from org.pkg import MyClass, AnotherClass           | are not found                            |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Import a java package for |                                                         | .. code-block: java                                     | Does not report errors if the package    |
| long name access          |                                                         |     import org.pkg                                      | is invalid                               |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Import a class static     | .. code-block: java                                     | .. code-block: java                                     | Constants, static fields, and            |
|                           |     import org.pkg.MyClass.CONST_FIELD                  |     from org.pkg.MyClass import CONST_FIELD             | static methods can be imported.          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Import a class            | .. code-block: java                                     | .. code-block: java                                     | ``JClass`` loads any class by name       |
| without tld               |     import zippy.NonStandard                            |     NonStandard = JClass('zippy.NonStandard')           | including inner classes.                 |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Construct an object       | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     MyClass myObject = new MyClass(1);                  |     myObject = MyClass(1)                               |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Constructing a cless      |                                                         | .. code-block:: python                                  |                                          |
| with full class name      |                                                         |     import org.pkg                                      |                                          |
|                           |                                                         |     myObject = org.pkg.MyClass(args)                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Get a static field        | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     int var = MyClass.staticField;                      |     var = MyClass.staticField                           |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Get a member field        | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     int var = myObject.memberField;                     |     var = myObject.memberField                          |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Set a static field        | .. code-block: java                                     | .. code-block: java                                     | Produces error for final fields          |
|                           |     MyClass.staticField = 2;                            |     MyClass.staticField = 2                             |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Set a member field        | .. code-block: java                                     | .. code-block: java                                     | Produces error for final fields          |
|                           |     myObject.memberField = 2;                           |     myObject.memberField = 2                            |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Call a static method      | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     MyClass.callStatic(1);                              |     MyClass.callStatic(1)                               |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Call a member method      | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     myObject.callMember(1);                             |     myObject.callMember(1)                              |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Access member with python | .. code-block: java                                     | .. code-block: java                                     | Underscore is added during wrapping.     |
| naming conflict           |     myObject.pass()                                     |     myObject.pass_()                                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Checking inheritance      | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     if (obj instanceof MyClass) {...}                   |     if (isinstance(obj, MyClass): ...                   |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Checking if Java          |                                                         | .. code-block: java                                     |                                          |
| class wrapper             |                                                         |     if (isinstance(obj, JClass): ...                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Checking if Java          |                                                         | .. code-block: java                                     |                                          |
| object wrapper            |                                                         |     if (isinstance(obj, JObject): ...                   |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Casting to a              | .. code-block: java                                     | .. code-block: java                                     |                                          |
| specific type             |     BaseClass b = (BaseClass)myObject;                  |     b = JObject(myObject, BaseClass)                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+


Exceptions
----------

Java exceptions extend from python exceptions and can be dealt with no different 
that Python native exceptions. JException serves as the base class for all Java exceptions.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
| Description               | Java                                                    | Python                                                  | Notes                                    |
+===========================+=========================================================+=========================================================+==========================================+
|                           |                                                         |                                                         |                                          |
| Catch an exception        | .. code-block:: java                                    | .. code-block:: python                                  |                                          |
|                           |     try {                                               |     try:                                                |                                          |
|                           |        myObject.throwsException();                      |         myObject.throwsException()                      |                                          |
|                           |     } catch (java.lang.Exception ex)                    |     except java.lang.Exception as ex:                   |                                          |
|                           |     { ... }                                             |         ...                                             |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Throw an exception        | .. code-block:: java                                    | .. code-block:: python                                  |                                          |
| to Java                   |     throw new java.lang.Exception("Problem");           |     raise java.lang.Exception("Problem");               |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Checking if Java          |                                                         | .. code-block:: pythoe                                  |                                          |
| exception wrapper         |                                                         |     if (isinstance(obj, JException): ...                |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Closeable items           | .. code-block: java                                     | .. code-block: python                                   |                                          |
|                           |     try (InputStream is = Files.newInputStream(file)    |     with Files.newInputStream(file) as is:              |                                          |
|                           |     { ... }                                             |        ...                                              |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+


Primitives
----------

Most python primitives directly map into Java primitives. However, python does not
have the same primitive types, thus sometimes it is necessary to cast to a specific 
Java primitive type especially if there are 
Java overloads that would otherwise be in conflict.  Each of the Java types are
exposed in JPype (``JBoolean``, ``JByte``, ``JChar``, ``JShort``, ``JInt``, ``JLong``, 
``JFloat``, ``JDouble``).

Python int is equivalent to Java long.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
| Description               | Java                                                    | Python                                                  | Notes                                    |
+===========================+=========================================================+=========================================================+==========================================+
|                           |                                                         |                                                         |                                          |
| Casting to hit            | .. code-block: java                                     | .. code-block: java                                     | ``JInt`` acts as a casting operator      |
| an overload               |     myObject.call((int)v);                              |     myObject.call(JInt(v))                              |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Create a primitive array  | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     int[] array = new int[5]                            |     array = JArray(JInt)(5)                             |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Create a primitive array  | .. code-block: java                                     | .. code-block: java                                     | list, sequences, or np.array             |
|                           |     int[] array = new int[){1,2,3}                      |     array = JArray(JInt)([1,2,3])                       | can be used to initialize.               |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Put a specific            | .. code-block: java                                     | .. code-block: python                                   |                                          |
| primitive type on a list  |     List<Integer> myList                                |     from java.util import ArrayList                     |                                          |
|                           |       = new ArrayList<>();                              |     myList = ArrayList()                                |                                          |
|                           |     myList.add(1);                                      |     myList.add(JInt(1))                                 |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Boxing a primitive        | .. code-block: java                                     | boxed = JObject(JInt(1))                                | ``JInt`` specifies the prmitive type.    |
|                           |     Integer boxed = 1;                                  |                                                         | ``JObject`` boxes the primitive.         |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+


Strings
-------

Java strings are similar to python strings.  They are both immutable and
produce a new string when altered.  Most operations can use Java strings
in place of python strings, with minor exceptions as python strings 
are not completely duck typed.  When comparing or using as dictionary keys
JString should be converted to python.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
| Description               | Java                                                    | Python                                                  | Notes                                    |
+===========================+=========================================================+=========================================================+==========================================+
|                           |                                                         |                                                         |                                          |
| Create a Java string      | .. code-block: java                                     | .. code-block: java                                     | ``JString`` constructs a                 |
|                           |     String javaStr = new String("foo");                 |     myStr = JString("foo")                              | ``java.lang.String``                     |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Create a Java string      | .. code-block: java                                     | .. code-block: python                                   | All ``java.lang.String``                 |
| from bytes                |     byte[] b;                                           |     b= b'foo'                                           | constuctors work.                        |
|                           |     String javaStr = new String(b, "UTF-8");            |     myStr = JString(b, "UTF-8")                         |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Converting Java string    |                                                         | .. code-block: java                                     |                                          |
|                           |                                                         |     str(javaStr)                                        |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Comparing Python and      |                                                         | .. code-block: java                                     | ``str()`` converts the object for        |
| Java strings              |                                                         |     str(javaStr) == pyString                            | comparison                               |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Comparing Java strings    | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     javaStr.equals("foo")                               |     javaStr == "foo"                                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Checking if java string   |                                                         | .. code-block: java                                     |                                          |
|                           |                                                         |     if (isinstance(obj, JString): ...                   |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+


Arrays
------

Arrays are create using JArray class factory. They operate like python lists, but they are 
fixed in size.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
| Description               | Java                                                    | Python                                                  | Notes                                    |
+===========================+=========================================================+=========================================================+==========================================+
|                           |                                                         |                                                         |                                          |
| Create a single           | .. code-block: java                                     | .. code-block: java                                     |                                          |
| dimension array           |     MyClass[] array = new MyClass[5];                   |     array = JArray(MyClass)(5)                          |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Create a multi            | .. code-block: java                                     | .. code-block: java                                     |                                          |
|  dimension array          |     MyClass[][] array2 = new MyClass[5][];              |     array2 = JArray(MyClass, 2)(5)                      |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Access an element         | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     array[0] = new MyClass()                            |     array[0] = MyClass()                                |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Size of an array          | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     array.length                                        |     len(array)                                          |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Convert to python list    |                                                         | .. code-block: java                                     |                                          |
|                           |                                                         |     pylist = list(array)                                |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Iterate elements          | .. code-block:: java                                    | .. code-block:: python                                  |                                          |
|                           |     for (MyClass element: array)                        |     for element in array:                               |                                          |
|                           |     {...}                                               |       ...                                               |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Checking if java array    |                                                         | .. code-block:: python                                  |                                          |
| wrapper                   |                                                         |     if (isinstance(obj, JArray): ...                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+


Collections
-----------

Java standard containers are available and are overloaded with python syntax where 
possible to operate in a similar fashion to python objects.  It is not
currently possible to specify the template types for generic containers, but
that will be introduced in Java 9.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
| Description               | Java                                                    | Python                                                  | Notes                                    |
+===========================+=========================================================+=========================================================+==========================================+
|                           |                                                         |                                                         |                                          |
| Import list type          | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     import java.util.ArrayList;                         |     from java.util import ArrayList                     |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Construct a list          | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     List<Integer> myList=new ArrayList<>();             |     myList=ArrayList()                                  |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Get length of list        | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     int sz = myList.size();                             |     sz = len(myList)                                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Get list item             | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     Integer i = myList.get(0)                           |     i = myList[0]                                       |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Set list item             | .. code-block: java                                     | .. code-block: java                                     | Casting is required to box primitives    |
|                           |     myList.set(0, 1)                                    |     myList[0]=Jint(1)                                   | to the correct type.                     |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Iterate list elements     | .. code-block:: java                                    | .. code-block:: python                                  |                                          |
|                           |     for (Integer element: myList)                       |     for element in myList:                              |                                          |
|                           |     {...}                                               |       ...                                               |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Import map type           | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     import java.util.HashMap;                           |     from java.util import HashMap                       |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Construct a map           | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     Map<String,Integer> myMap=new HashMap<>();          |     myMap=HashMap()                                     |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Get length of map         | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     int sz = myMap.size();                              |     sz = len(myMap)                                     |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Get map item              | .. code-block: java                                     | .. code-block: java                                     |                                          |
|                           |     Integer i = myMap.get("foo")                        |     i = myMap["foo"]                                    |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Set map item              | .. code-block: java                                     | .. code-block: java                                     | Casting is required to box primitives    |
|                           |     myMap.set("foo", 1)                                 |     myMap["foo"]=Jint(1)                                | to the correct type.                     |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Iterate map entries       | .. code-block:: java                                    | .. code-block:: python                                  |                                          |
|                           |     for (Map.Entry<String,Integer> e                    |     for e in myMap.entrySet():                          |                                          |
|                           |       : myMap.entrySet())                               |       ...                                               |                                          |
|                           |       {...}                                             |                                                         |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+


Reflection
----------

For operations that are outside the scope of the JPype syntax, Using
Java reflection, any Java operation include calling a specific overload
or even accessing private methods and fields.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
| Description               | Java                                                    | Python                                                  | Notes                                    |
+===========================+=========================================================+=========================================================+==========================================+
|                           |                                                         |                                                         |                                          |
| Access Java reflection    | .. code-block:: java                                    | .. code-block:: java                                    |                                          |
| class                     |     MyClass.class                                       |     MyClass.class_                                      |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Access a private field    |                                                         | .. code-block:: python                                  |                                          |
| by name                   |                                                         |     cls = myObject.class_                               |                                          |
|                           |                                                         |     field = cls.getDeclaredField("internalField")       |                                          |
|                           |                                                         |     field.setAccessible(True)                           |                                          |
|                           |                                                         |     field.get()                                         |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Accessing a specific      |                                                         | .. code-block:: python                                  | types must be exactly specified.         |
| overload                  |                                                         |     cls = MyClass.class_                                |                                          |
|                           |                                                         |     cls.getDeclaredMethod("call", JInt)                 |                                          |
|                           |                                                         |     cls.invoke(myObject, JInt(1))                       |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Convert a                 |                                                         | .. code-block:: python                                  | Rarely required unless the               |
| ``java.lang.Class``       |                                                         |     # Something returned a java.lang.Class              | class was supplied external such         |
| into python wrapper       |                                                         |     MyClassJava = getClassMethod()                      | as generics.                             |
|                           |                                                         |                                                         |                                          |
|                           |                                                         |     # Convert to it to Python                           |                                          |
|                           |                                                         |     MyClass = JClass(myClassJava)                       |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Load a class with a       | .. code-block:: java                                    | .. code-block:: python                                  |                                          |
| external class loader     |     ClassLoader cl = new ExternalClassLoader();         |     cl = ExternalClassLoader()                          |                                          |
|                           |     Class cls = Class.forName("External", True, cl)     |     cls = JClass("External", loader=cl)                 |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         |                                                         |                                          |
| Accessing base method     |                                                         | .. code-block:: python                                  |                                          |
| implementation            |                                                         |     from org.pkg import BaseClass, MyClass              |                                          |
|                           |                                                         |     myObject = MyClass(1)                               |                                          |
|                           |                                                         |     BaseClass.callMember(myObject, 2)                   |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+


Implements and Extension
------------------------

JPype can implement a Java interface by annotating a python class.  Each
method that is required must be implemented.

JPype does not support extending a class directly in python.  Where it is
necessary to exend a Java class, it is required to create a Java extension
with an interface for each methods that are to be accessed from python.
For some deployments this may be be an option.  If that is the case, 
the JPype inline compiler can be used to create the dynamic class on the 
fly.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
| Description               | Java                                                    | Python                                                  | Notes                                    |
+===========================+=========================================================+=========================================================+==========================================+
|                           |                                                         |                                                         |                                          |
| Implement an interface    | .. code-block:: java                                    | .. code-block:: python                                  |                                          |
|                           |     public class PyImpl implements MyInterface          |     @JImplements(MyInterface)                           |                                          |
|                           |     {                                                   |     class PyImpl(object):                               |                                          |
|                           |       public void call() {...}                          |         @JOverride                                      |                                          |
|                           |     }                                                   |         def call(self):                                 |                                          |
|                           |                                                         |           pass                                          |                                          |
|                           |                                                         |                                                         |                                          |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         | None                                                    |                                          |
| Extending classes         |                                                         |                                                         | Support for use of python function       |
|                           |                                                         |                                                         | as Java 8 lambda is WIP.                 |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+
|                           |                                                         | None                                                    |                                          |
| Lambdas                   |                                                         |                                                         | Support for use of python function       |
|                           |                                                         |                                                         | as Java 8 lambda is WIP.                 |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+------------------------------------------+



Don't like the formatting? Feel the guide is missing something? Submit a pull request 
at the project page.

