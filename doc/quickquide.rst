
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

Starting JPype
--------------

The hardest thing about using JPype is getting the jars loaded into the JVM.
Java is curiously unfriendly about reporting problems when it is unable to find
a jar.  Instead, it will be reported as an ``ImportError`` in python.
These patterns will help debug problems regarding jar loading.

Once the JVM is started Java packages that are within a top level domain (TLD)
are exposed as python modules allowing Java to be treated as part of python.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Start Java Virtual        |                                                         | .. code-block:: python                                  |
| Machine (JVM) [1]_        |                                                         |     # Import module                                     |
|                           |                                                         |     import jpype                                        |
|                           |                                                         |                                                         |
|                           |                                                         |     # Enable Java imports                               |
|                           |                                                         |     import jpype.imports                                |
|                           |                                                         |                                                         |
|                           |                                                         |     # Pull in types                                     |
|                           |                                                         |     from jpype.types import *                           |
|                           |                                                         |                                                         |
|                           |                                                         |     # Launch the JVM                                    |
|                           |                                                         |     jvmPath = jpype.getDefaultJVMPath()                 |
|                           |                                                         |     jvmArgs = "-Djava.class.path=%" %                   |
|                           |                                                         |       jpype.getClassPath()                              |
|                           |                                                         |     jpype.startJVM(jvmPath,jvmArgs)                     |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import default Java       |                                                         | .. code-block:: python                                  |
| namespace [2]_            |                                                         |     import java.lang                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Add a set of jars from a  |                                                         | .. code-block:: python                                  |
| directory [3]_            |                                                         |     jpype.addClassPath('/my/path/*')                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Add a specific jar to the |                                                         | .. code-block:: python                                  |
| classpath [4]_            |                                                         |     jpype.addClassPath('/my/path/myJar.jar')            |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Print JVM CLASSPATH [5]_  |                                                         | .. code-block:: python                                  |
|                           |                                                         |     from java.lang import System                        |
|                           |                                                         |     print(System.getProperty("java.class.path"))        |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
.. [1] REVISE
.. [2] All java.lang.* classes are available.
.. [3] Must happen prior to starting JVM
.. [4] Must happen prior to starting the JVM
.. [5] After JVM is started


Classes/Objects
---------------

Java classes are presented whereever possible exactly like Python classes. The only
major difference is that Java classes and objects are closed and cannot be modified.
As Java is strongly typed, casting operators are used to select specific 
overloads when calling methods.  Classes are either imported using as a module
or loaded with the ``JClass`` factory.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Import a class [1]_       | .. code-block: java                                     | .. code-block: java                                     |
|                           |     import org.pkg.MyClass                              |     from org.pkg import MyClass                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import a class and rename |                                                         | .. code-block: java                                     |
| [1]_                      |                                                         |     from org.pkg import MyClass as OurClass             |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import multiple classes   |                                                         | .. code-block: java                                     |
| from a package [2]_       |                                                         |     from org.pkg import MyClass, AnotherClass           |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import a java package for |                                                         | .. code-block: java                                     |
| long name access [3]_     |                                                         |     import org.pkg                                      |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import a class static     | .. code-block: java                                     | .. code-block: java                                     |
| [4]_                      |     import org.pkg.MyClass.CONST_FIELD                  |     from org.pkg.MyClass import CONST_FIELD             |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import a class without    | .. code-block: java                                     | .. code-block: java                                     |
| tld [5]_                  |     import zippy.NonStandard                            |     NonStandard = JClass('zippy.NonStandard')           |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Construct an object       | .. code-block: java                                     | .. code-block: java                                     |
|                           |     MyClass myObject = new MyClass(1);                  |     myObject = MyClass(1)                               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Constructing a cless with |                                                         | .. code-block:: python                                  |
| full class name           |                                                         |     import org.pkg                                      |
|                           |                                                         |     myObject = org.pkg.MyClass(args)                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get a static field        | .. code-block: java                                     | .. code-block: java                                     |
|                           |     int var = MyClass.staticField;                      |     var = MyClass.staticField                           |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get a member field        | .. code-block: java                                     | .. code-block: java                                     |
|                           |     int var = myObject.memberField;                     |     var = myObject.memberField                          |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Set a static field [6]_   | .. code-block: java                                     | .. code-block: java                                     |
|                           |     MyClass.staticField = 2;                            |     MyClass.staticField = 2                             |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Set a member field [6]_   | .. code-block: java                                     | .. code-block: java                                     |
|                           |     myObject.memberField = 2;                           |     myObject.memberField = 2                            |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Call a static method      | .. code-block: java                                     | .. code-block: java                                     |
|                           |     MyClass.callStatic(1);                              |     MyClass.callStatic(1)                               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Call a member method      | .. code-block: java                                     | .. code-block: java                                     |
|                           |     myObject.callMember(1);                             |     myObject.callMember(1)                              |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Access member with python | .. code-block: java                                     | .. code-block: java                                     |
| naming conflict [7]_      |     myObject.pass()                                     |     myObject.pass_()                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking inheritance      | .. code-block: java                                     | .. code-block: java                                     |
|                           |     if (obj instanceof MyClass) {...}                   |     if (isinstance(obj, MyClass): ...                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if Java class    |                                                         | .. code-block: java                                     |
| wrapper                   |                                                         |     if (isinstance(obj, JClass): ...                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if Java object   |                                                         | .. code-block: java                                     |
| wrapper                   |                                                         |     if (isinstance(obj, JObject): ...                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Casting to a specific     | .. code-block: java                                     | .. code-block: java                                     |
| type                      |     BaseClass b = (BaseClass)myObject;                  |     b = JObject(myObject, BaseClass)                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
.. [1] This will report an error if the class is not found.
.. [2] This will report an error if the classes are not found
.. [3] Does not report errors if the package is invalid
.. [4] Constants, static fields, and static methods can be imported.
.. [5] ``JClass`` loads any class by name including inner classes.
.. [6] Produces error for final fields
.. [7] Underscore is added during wrapping.


Exceptions
----------

Java exceptions extend from python exceptions and can be dealt with no different 
that Python native exceptions. JException serves as the base class for all Java exceptions.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Catch an exception        | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |     try {                                               |     try:                                                |
|                           |        myObject.throwsException();                      |         myObject.throwsException()                      |
|                           |     } catch (java.lang.Exception ex)                    |     except java.lang.Exception as ex:                   |
|                           |     { ... }                                             |         ...                                             |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Throw an exception to     | .. code-block:: java                                    | .. code-block:: python                                  |
| Java                      |     throw new java.lang.Exception("Problem");           |     raise java.lang.Exception("Problem");               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if Java          |                                                         | .. code-block:: pythoe                                  |
| exception wrapper         |                                                         |     if (isinstance(obj, JException): ...                |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Closeable items           | .. code-block: java                                     | .. code-block: python                                   |
|                           |     try (InputStream is = Files.newInputStream(file)    |     with Files.newInputStream(file) as is:              |
|                           |     { ... }                                             |        ...                                              |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+


Primitives
----------

Most python primitives directly map into Java primitives. However, python does not
have the same primitive types, thus sometimes it is necessary to cast to a specific 
Java primitive type especially if there are 
Java overloads that would otherwise be in conflict.  Each of the Java types are
exposed in JPype (``JBoolean``, ``JByte``, ``JChar``, ``JShort``, ``JInt``, ``JLong``, 
``JFloat``, ``JDouble``).

Python int is equivalent to Java long.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Casting to hit an         | .. code-block: java                                     | .. code-block: java                                     |
| overload [1]_             |     myObject.call((int)v);                              |     myObject.call(JInt(v))                              |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create a primitive array  | .. code-block: java                                     | .. code-block: java                                     |
|                           |     int[] array = new int[5]                            |     array = JArray(JInt)(5)                             |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create a primitive array  | .. code-block: java                                     | .. code-block: java                                     |
| [2]_                      |     int[] array = new int[){1,2,3}                      |     array = JArray(JInt)([1,2,3])                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Put a specific primitive  | .. code-block: java                                     | .. code-block: python                                   |
| type on a list            |     List<Integer> myList                                |     from java.util import ArrayList                     |
|                           |       = new ArrayList<>();                              |     myList = ArrayList()                                |
|                           |     myList.add(1);                                      |     myList.add(JInt(1))                                 |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Boxing a primitive [3]_   | .. code-block: java                                     | boxed = JObject(JInt(1))                                |
|                           |     Integer boxed = 1;                                  |                                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
.. [1] ``JInt`` acts as a casting operator
.. [2] list, sequences, or np.array can be used to initialize.
.. [3] ``JInt`` specifies the prmitive type. ``JObject`` boxes the primitive.


Strings
-------

Java strings are similar to python strings.  They are both immutable and
produce a new string when altered.  Most operations can use Java strings
in place of python strings, with minor exceptions as python strings 
are not completely duck typed.  When comparing or using as dictionary keys
JString should be converted to python.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Create a Java string [1]_ | .. code-block: java                                     | .. code-block: java                                     |
|                           |     String javaStr = new String("foo");                 |     myStr = JString("foo")                              |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create a Java string from | .. code-block: java                                     | .. code-block: python                                   |
| bytes [2]_                |     byte[] b;                                           |     b= b'foo'                                           |
|                           |     String javaStr = new String(b, "UTF-8");            |     myStr = JString(b, "UTF-8")                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Converting Java string    |                                                         | .. code-block: java                                     |
|                           |                                                         |     str(javaStr)                                        |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Comparing Python and Java |                                                         | .. code-block: java                                     |
| strings [3]_              |                                                         |     str(javaStr) == pyString                            |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Comparing Java strings    | .. code-block: java                                     | .. code-block: java                                     |
|                           |     javaStr.equals("foo")                               |     javaStr == "foo"                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if java string   |                                                         | .. code-block: java                                     |
|                           |                                                         |     if (isinstance(obj, JString): ...                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
.. [1] ``JString`` constructs a ``java.lang.String``
.. [2] All ``java.lang.String`` constuctors work.
.. [3] ``str()`` converts the object for comparison


Arrays
------

Arrays are create using JArray class factory. They operate like python lists, but they are 
fixed in size.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Create a single dimension | .. code-block: java                                     | .. code-block: java                                     |
| array                     |     MyClass[] array = new MyClass[5];                   |     array = JArray(MyClass)(5)                          |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Create a multi  dimension | .. code-block: java                                     | .. code-block: java                                     |
| array                     |     MyClass[][] array2 = new MyClass[5][];              |     array2 = JArray(MyClass, 2)(5)                      |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Access an element         | .. code-block: java                                     | .. code-block: java                                     |
|                           |     array[0] = new MyClass()                            |     array[0] = MyClass()                                |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Size of an array          | .. code-block: java                                     | .. code-block: java                                     |
|                           |     array.length                                        |     len(array)                                          |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Convert to python list    |                                                         | .. code-block: java                                     |
|                           |                                                         |     pylist = list(array)                                |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Iterate elements          | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |     for (MyClass element: array)                        |     for element in array:                               |
|                           |     {...}                                               |       ...                                               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Checking if java array    |                                                         | .. code-block:: python                                  |
| wrapper                   |                                                         |     if (isinstance(obj, JArray): ...                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+


Collections
-----------

Java standard containers are available and are overloaded with python syntax where 
possible to operate in a similar fashion to python objects.  It is not
currently possible to specify the template types for generic containers, but
that will be introduced in Java 9.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Import list type          | .. code-block: java                                     | .. code-block: java                                     |
|                           |     import java.util.ArrayList;                         |     from java.util import ArrayList                     |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Construct a list          | .. code-block: java                                     | .. code-block: java                                     |
|                           |     List<Integer> myList=new ArrayList<>();             |     myList=ArrayList()                                  |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get length of list        | .. code-block: java                                     | .. code-block: java                                     |
|                           |     int sz = myList.size();                             |     sz = len(myList)                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get list item             | .. code-block: java                                     | .. code-block: java                                     |
|                           |     Integer i = myList.get(0)                           |     i = myList[0]                                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Set list item [1]_        | .. code-block: java                                     | .. code-block: java                                     |
|                           |     myList.set(0, 1)                                    |     myList[0]=Jint(1)                                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Iterate list elements     | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |     for (Integer element: myList)                       |     for element in myList:                              |
|                           |     {...}                                               |       ...                                               |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Import map type           | .. code-block: java                                     | .. code-block: java                                     |
|                           |     import java.util.HashMap;                           |     from java.util import HashMap                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Construct a map           | .. code-block: java                                     | .. code-block: java                                     |
|                           |     Map<String,Integer> myMap=new HashMap<>();          |     myMap=HashMap()                                     |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get length of map         | .. code-block: java                                     | .. code-block: java                                     |
|                           |     int sz = myMap.size();                              |     sz = len(myMap)                                     |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Get map item              | .. code-block: java                                     | .. code-block: java                                     |
|                           |     Integer i = myMap.get("foo")                        |     i = myMap["foo"]                                    |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Set map item [1]_         | .. code-block: java                                     | .. code-block: java                                     |
|                           |     myMap.set("foo", 1)                                 |     myMap["foo"]=Jint(1)                                |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Iterate map entries       | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |     for (Map.Entry<String,Integer> e                    |     for e in myMap.entrySet():                          |
|                           |       : myMap.entrySet())                               |       ...                                               |
|                           |       {...}                                             |                                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
.. [1] Casting is required to box primitives to the correct type.


Reflection
----------

For operations that are outside the scope of the JPype syntax, Using
Java reflection, any Java operation include calling a specific overload
or even accessing private methods and fields.


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Access Java reflection    | .. code-block:: java                                    | .. code-block:: java                                    |
| class                     |     MyClass.class                                       |     MyClass.class_                                      |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Access a private field by |                                                         | .. code-block:: python                                  |
| name                      |                                                         |     cls = myObject.class_                               |
|                           |                                                         |     field = cls.getDeclaredField("internalField")       |
|                           |                                                         |     field.setAccessible(True)                           |
|                           |                                                         |     field.get()                                         |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Accessing a specific      |                                                         | .. code-block:: python                                  |
| overload [1]_             |                                                         |     cls = MyClass.class_                                |
|                           |                                                         |     cls.getDeclaredMethod("call", JInt)                 |
|                           |                                                         |     cls.invoke(myObject, JInt(1))                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Convert a                 |                                                         | .. code-block:: python                                  |
| ``java.lang.Class`` into  |                                                         |     # Something returned a java.lang.Class              |
| python wrapper [2]_       |                                                         |     MyClassJava = getClassMethod()                      |
|                           |                                                         |                                                         |
|                           |                                                         |     # Convert to it to Python                           |
|                           |                                                         |     MyClass = JClass(myClassJava)                       |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Load a class with a       | .. code-block:: java                                    | .. code-block:: python                                  |
| external class loader     |     ClassLoader cl = new ExternalClassLoader();         |     cl = ExternalClassLoader()                          |
|                           |     Class cls = Class.forName("External", True, cl)     |     cls = JClass("External", loader=cl)                 |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         |                                                         |
| Accessing base method     |                                                         | .. code-block:: python                                  |
| implementation            |                                                         |     from org.pkg import BaseClass, MyClass              |
|                           |                                                         |     myObject = MyClass(1)                               |
|                           |                                                         |     BaseClass.callMember(myObject, 2)                   |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
.. [1] types must be exactly specified.
.. [2] Rarely required unless the class was supplied external such as generics.


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


+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
| Description               | Java                                                    | Python                                                  |
+===========================+=========================================================+=========================================================+
|                           |                                                         |                                                         |
| Implement an interface    | .. code-block:: java                                    | .. code-block:: python                                  |
|                           |     public class PyImpl implements MyInterface          |     @JImplements(MyInterface)                           |
|                           |     {                                                   |     class PyImpl(object):                               |
|                           |       public void call() {...}                          |         @JOverride                                      |
|                           |     }                                                   |         def call(self):                                 |
|                           |                                                         |           pass                                          |
|                           |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         | None                                                    |
| Extending classes [1]_    |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
|                           |                                                         | None                                                    |
| Lambdas [1]_              |                                                         |                                                         |
+---------------------------+---------------------------------------------------------+---------------------------------------------------------+
.. [1] Support for use of python function as Java 8 lambda is WIP.



Don't like the formatting? Feel the guide is missing something? Submit a pull request 
at the project page.

