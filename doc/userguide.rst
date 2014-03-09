User Guide
==========

Overview
--------

JPype is an effort to allow python programs full access to java class
libraries. This is achieved not through re-implementing Python, as
Jython/JPython has done, but rather through interfacing at the native level
in both Virtual Machines.

Eventually, it should be possible to replace Java with python in many, though
not all, situations. JSP, Servlets, RMI servers and IDE plugins are all good
candidates.

Once this integration is achieved, a second phase will be started to separate
the Java logic from the Python logic, eventually allowing the bridging
technology to be used in other environments, I.E. Ruby, Perl, COM, etc ...

Why such a project?
~~~~~~~~~~~~~~~~~~~

As much as I enjoy programming in Python, there is no denying that Java has
the bulk of the mindshare. Just look on Sourceforge, at the time of creation
of this project, there were 3267 Python-related projects, and 12126 Java-
related projects. And that's not counting commercial interests.

Server-side Python is also pretty weak. Zope may be a great application
server, but I have never been able to figure it out. Java, on the other hand,
shines on the server.

So in order to both enjoy the language, and have access to the most popular
libraries, I have started this project.

What about Jython?
~~~~~~~~~~~~~~~~~~

Jython (formerly known as JPython) is a great idea. However, it suffers from
a large number of drawbacks, i.e. it always lags behind CPython, it is slow
and it does not allow access to most Python extensions.

My idea allows using both kinds of libraries in tandem, so the developer is
free to pick and choose.

Using JPype
~~~~~~~~~~~

Here is a sample program to demonstrate how to use JPype : ::

  from jpype import *
  startJVM("d:/tools/j2sdk/jre/bin/client/jvm.dll", "-ea")
  java.lang.System.out.println("hello world")
  shutdownJVM()

This is of course a simple **hello world** type of application. Yet it shows
the 2 most important calls : **startupJVM** and **shutdownJVM**.

The rest will be explained in more details in the next sections.

Core Ideas
----------

Threading
---------

Any non-trivial application will have need of threading. Be it implicitly by
using a GUI, or because you're writing a multi-user server. Or explicitly for
performance reason.

The only real problem here is making sure Java thread and Python threads
cooperate correctly. Thankfully, this is pretty easy to to.

Python Threads
~~~~~~~~~~~~~~

For the most part, python threads based on OS level threads (i.e posix
threads), will work without problem. The only thing to remember is to call
**jpype.attachThreadToJVM()** in the thread body to make the JVM usable from
that thread. For threads that you do not start yourself, you can call
**isThreadAttachedToJVM()** to check.

Java Threads
~~~~~~~~~~~~

At the moment, it is not possible to use threads created from Java, since
there is no **callback** available.

Other Threads
~~~~~~~~~~~~~

Some python libraries offer other kinds of thread, (i.e. microthreads). How
they interact with Java depends on their nature. As stated earlier, any OS-
level threads will work without problem. Emulated threads, like microthreads,
will appear as a single thread to java, so special care will have to be taken
for synchronization.

Synchronization
~~~~~~~~~~~~~~~

Java synchronization support can be split into 2 categories. The first is the
**synchronized** keyword, both as prefix on a method and as a block inside a
method. The second arre the different methods available on the Object class
(**notify, notifyAll, wait**).

To support the **synchronized** functionality, jpype defines a method called
synchronized(O). O has to be a java object or Java class, or a Java wrapper
that corresponds to an object (JString and JObject). The return value is a
monitor object that will keep the synchronization on as long as the object is
kept alive. the lock will be broken as soon as the monitor is GCd. So make
sure to hang on to it as long as you need it.

The other methods are available as is on any _JavaObject.

For synchronization that does not have to be shared with java code, I suggest
using python support instead of Java's, as it'll be more natural and easier

Performance
-----------

JPype uses JNI, which is well known in the Java world as not being the most
efficient of interfaces. Further, JPype bridges two very different runtime
environment, performing conversion back and forth as needed. Both of these
can impose rather large performance bottlenecks.

JNI is the standard native interface for most, if not all, JVMs, so there is
no getting around it. down the road, it is possible that interfacing with CNI
(GCC's java native interface). The only to minimize the JNI cost is to move
some code over to Java.

Follow the regular python philosophy : **Write it all in Python, then write
only those parts that need it in C.** Except this time, its write the parts
that need it in Java.

For the conversion costs, again, nothing much can be done. In cases where a
given object (be it a string, an object, an array, etc ...) is passed often
into Java, you can pre-convert it once using the wrappers, and then pass in
the wrappers. For most situations, this should solve the problem.

As a final note, awhile a JPype program will likely be slower than a pure
java counterpart, it has a good chance of being faster than pure python
version of it. The JVM is a memory hog, but does a good job of optimizing
code execution speeds.

Inner Classes
-------------

For the most part, inner classes can be used like normal classesm, with thw
following differences :

- Inner classes in java natively use $ to separate the outer class from
  the inner class. For example, inner class Foo defined inside class Bar is
  called Bar.Foo in java, but its real native name is Bar$Foo.
- Because of this name mangling, you cannot use the standard package
  access method to get them. Use the method __getclass__ in the JPackage to
  load them.
- Non-static inner classes cannot be instantiated from Python code.
  Instances received from Java code thay can be used without problem.

Arrays
------

JPype has full support for receiving java arrays and passing them to java
methods. Java arrays, wrapped in the JArray wrapper class, behave like Python
lists, except that their size is fixed, and that the contents are of a
specific type.

Multi-dimension arrays (array of arrays) also work without problem.

Creating Java arrays form Python
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The JArray wrapper is used to create Arrays form Python code. The code to
create an arrays is like this : ::

  JArray(type, num_dims)(sz or sequence)

Type is either a Java Class (as a String or a JavaClass object) or a Wrapper
type. num_dims is the number of dimensions to build the array and defaults to
1.

sz is the actual number of elements in the arrays, and sequence is a sequence
to initialize the array with.

The logic behind this is that JArray(type, ndims) returns an Array Class,
which can then be called like any other class to create an instance.

Type conversion
---------------

One of the most complex part of a bridge system like JPype is finding a way
to seemlessly translate between python types and Java types. The following
table will show what implicit conversions occur, both Python to Java and Java
to Python. Explicit conversion, which happens when a python object is
wrapped, is convered in each wrapper.

Conversion from Python to Java
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This type of conversion happens when a Python object is used either as a
parameter to a Java method or to set the value of a java field.

Type Matching
~~~~~~~~~~~~~~

JPype defines different leels of "match" between Python objects and Java
types. These levels are :

- **none**, There is no way to convert.
- **explicit (E)**, JPype canconvert ot the desired type, but only
  explicitly via the wrapper classes. This means the proper wrapper class
  will access this type as argument.
- **implicit (I)**, JPype will convert as needed.
- **exact> (X)**, Like implicit, but when deciding with method overload
  to use, one wj=here all the paramters match "exact" will take precedence
  over "implicit" matches.

============ ========== ========= =========== ========= ========== ========== =========== ========= ========== ========== =========== =========
Python\\Java    byte      short       int       long       float     double     boolean     char      String      Array     Object      Class   
============ ========== ========= =========== ========= ========== ========== =========== ========= ========== ========== =========== =========
    int       I [1]_     I [1]_       X          I                             X [10]_                                               
   long       I [1]_     I [1]_     I [1]_       X                                                                                  
   float                                                  I [1]_       X                                                            
 sequence                                                                                                                           
dictionary                                                                                                                          
  string                                                                                   I [2]_       X                           
  unicode                                                                                  I [2]_       X                           
   JByte        X                                                                                                                   
  JShort                   X                                                                                                        
   JInt                               X                                                                                             
   JLong                                         X                                                                                  
  JFloat                                                    X                                                                       
  JDouble                                                              X                                                            
 JBoolean                                                                         X                                                 
  JString                                                                                               X                   I [3]_
   JChar                                                                                     X                                      
  JArray                                                                                                        I/X [4]_    I [5]_   
  JObject                                                                                                       I/X [6]_    I/X [7]_
JavaObject                                                                                                                  I [8]_
 JavaClass                                                                                                                  I [9]_        X     
============ ========== ========= =========== ========= ========== ========== =========== ========= ========== ========== =========== =========

.. [1] Conversion will occur if the Python value fits in the java
       native type.

.. [2] Conversion occurs if the python string or unicode is of
       length 1.

.. [3] The required object must be of a type compatible with
       ``java.lang.String(java.lang.Object, java.util.Comparable)``.

.. [4] Number of dimensions must match, and the types must be
       compatible

.. [5] Only when the required type is ``java.lang.Object``.

.. [6] Only if the JObject wrapper's specified type is an compatible
       array class.

.. [7] Only if the required type is compatible with the wrappers's
       specified type. The actual type of the java object is not
       considered.

.. [8] Only if the requireds type is compatible with the Java Object
       actual type.

.. [9] Only when the required type is ``java.lang.Object`` or
       ``java.lang.Class``.

.. [10] Only the values True and False are implitly converted to
        booleans.


Converting from java to python
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The rules here are much simpler.

Java **byte, short and int** are converted to python **int**.

Java **long** are conversion to Python **long**

Java **float and double **are converted to python **float**.

Java **boolean** are converted to python **int** of value 1 or 0

Java **char** are converted to python **unicode** of length 1.

Java **String** are converteds to python **unicode.**

Java **arrays** are converteds to **JArray**

All other java object are converted to **JavaObject**.

Java **Class** are converted to **JavaClass.**

Java array **Class** are converted to **JavaArrayClass.**


JProxy
------

The JPoxy allows python code to "implement" any number of java interfaces, so
as to receive callbacks through them.

Using JProxy is simple. The constructor takes 2 arguments. The first is one
or a sequence of string of JClass objects, defining the interfaces to be
"implemented". The second must be a keyword argument, and be either **dict**
or **inst**. If **dict** is specified, then the 2nd argument must be a
dictionary, with the keys the method names as defined in the interface(s),
and the values calllable objects. If **inst** an object instance must be
given, with methods defined for the methods declared in the interface(s).
Either way, when Java calls the interface method, the corresponding Python
callable is looked up and called.

Of course, this is not the same as subclassing Java classes in Python.
However, Most Java APIs are built so that subclassing in non needed. Good
examples of this are AWT and SWING. Except for relatively advanced features,
it is possible to build complete UIs without creating a single subclass.

For those cases where subclassing is absolutely necessary (I.E. using Java's
SAXP classes), it is generally easy to create an interfaces and a simple
subclass that delegates the calls to that interface.


Sample code :
~~~~~~~~~~~~~

Assume a Java interface like : ::

  public interface ITestInterface2
  {
          int testMethod();
          String testMethod2();
  }

You can create a proxy *implementing* this interface in 2 ways.
First, with a class : ::

  class C :
          def testMethod(self) :
                  return 42

          def testMethod2(self) :
                  return "Bar"

  c = C()
  proxy = JProxy("ITestInterface2", inst=c)

or you can do it with a dictionary ::

  def _testMethod() :
  return 32
  
  def _testMethod2() :
  return "Fooo!"	
  	
  d = {
  	'testMethod' : _testMethod,
  	'testMethod2' : _testMethod2,
  }
  proxy = JProxy("ITestInterface2", dict=d)


Java Exceptions
---------------

Error handling is a very important part of any non-trivial program. So
bridging Java's exception mechanism and Python's is very important.

Java exception classes are regular classes that extend, directly or
indirectly, the java.lang.Throwable class. Python exception are classes that
extend, directly or indirectly, the Exception class. On the surface they are
similar, at the C-API elvel, Python Exceptions are completely different from
regular Python classes. This contributes to the fact that it is not possible
to catch java exceptions in a completely straightforward way.

All Java exception thrown end up throwing the jpype.JavaException exception.
you can then use the message(), stackTrace() and javaClass() to access
extended information.

Here is an example : ::

  try :
          # Code that throws a java.lang.RuntimeException
  except JavaException, ex :
          if JavaException.javaClass() is java.lang.RuntimeException :
                  print "Caught the runtime exception : ", JavaException.message()
                  print JavaException.stackTrace()

Alternately, you can catch the REAL java exception directly by using
the JException wrapper. ::

        try :
                # Code that throws a java.lang.RuntimeException
        except jpype.JException(java.lang.RuntimeException), ex :
                print "Caught the runtime exception : ", JavaException.message()
                print JavaException.stackTrace()


Known limitations
-----------------

This section list those limitations that are unlikely to change, as they come
from external sources.


Unloading the JVM
~~~~~~~~~~~~~~~~~

The JNI API defines a method called destroyJVM(). However, this method does
not work. That is, Sun's JVMs do nto allow unloading. For this reason, after
calling shutdownJVM(), if you attemps calling startupJVM() again you will get
a non-specific exception. There is nothing wrong (that I can see) in JPype.
So if Sun get's around to supporting its own properly, or if you use JPype
with a non-SUN JVM that does (I beleive IBM's JVM support JNI invocation, but
tI do not know if their destoyJVM work properly), JPype will be able to take
advantage of it. As the time of writing, the latest stable SUN JVM was
1.4.2_04.


Methods dependent on "current" class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are a few methods in the Java libraries that rely on finding
information on the calling class. So these methods, if called directly form
Python code, will fail because there is no calling java class, and the JNI
api does not provide methods to simulate one.

At the moment, the methods known to fail are :


java.lang.Class.forName(String classname)
:::::::::::::::::::::::::::::::::::::::::

This method relies on the current class's classloader to do it loading. it
can easily be replaced with **Class.forName(classname, True,
ClassLoader.getSystemClassLoader())**.


java.sql.DriverManager.getConnection(...)
:::::::::::::::::::::::::::::::::::::::::

For some reason, this class verifies that the driver class as loaded in teh
"current" classloader is the same as previously registered. Since there is no
"current" classloader, it default to the internal classloader, which
typically does not find the driver. To remedy, simply instantiate the driver
yourself and call it's connect(...) method.


Module Reference
----------------


startupJVM method
~~~~~~~~~~~~~~~~~

This method MUST be called before any other jpype features can be used. It
will initialize the specified JVM.

Arguments
:::::::::

-   vmPath - Must be the path to the jvm.dll (or jvm.so, depending on
    platform)
-   misc arguments - All arguments after the first are optional, and are
    given as it to the JVM. Pretty much any command-line argument you can
    give the JVM can be passed here. A caveat, multi-part arguments (like
    -classpath) do not seem to work, and must e passed in as a -D option.
    Option **-classpath a;b;c** becomes **-Djava.class.path=a;b;c**


Return value
::::::::::::

None


Exceptions
::::::::::

On failure, a RuntimeException is raised.


shutdownJVM method
~~~~~~~~~~~~~~~~~~

For the most part, this method does nto have to be called. It will be
automatically executed when the jpype module is unloaded at python's exit.


Arguments
:::::::::

None


Return value
::::::::::::

None


Exceptions
::::::::::

On failure, a RuntimeException is raised.


attachThreadToJVM method
~~~~~~~~~~~~~~~~~~~~~~~~

For the most part, this method does nto have to be called. It will be
automatically executed when the jpype module is unloaded at python's exit.


Arguments
:::::::::

None


Return value
::::::::::::

None


Exceptions
::::::::::

On failure, a RuntimeException is raised.


isThreadAttachedToJVM method
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For the most part, this method does nto have to be called. It will be
automatically executed when the jpype module is unloaded at python's exit.


Arguments
:::::::::

None


Return value
::::::::::::

None


Exceptions
::::::::::

On failure, a RuntimeException is raised.


detachThreadFromJVM method
~~~~~~~~~~~~~~~~~~~~~~~~~~

For the most part, this method does nto have to be called. It will be
automatically executed when the jpype module is unloaded at python's exit.


Arguments
:::::::::

None


Return value
::::::::::::

None


Exceptions
::::::::::

On failure, a RuntimeException is raised.


synchronized method
~~~~~~~~~~~~~~~~~~~

For the most part, this method does nto have to be called. It will be
automatically executed when the jpype module is unloaded at python's exit.


Arguments
:::::::::

None


Return value
::::::::::::

None


Exceptions
::::::::::

On failure, a RuntimeException is raised.


JPackage class
~~~~~~~~~~~~~~

This packages allows structured access to java packages and classes. It is
very similar to a python import statement.

Only the root of the package tree need be declared with the JPackage
constructor. sub-packages will be created on demand.

For example, to import the w3c DOM package : ::

  Document = JPackage('org').w3c.dom.Document


Predefined Java packages
::::::::::::::::::::::::

For convenience, the jpype modules predefines the following JPackages :
**java, javax**

They can be used as is, without needing to resort to the JPackage
class.

Wrapper classes
~~~~~~~~~~~~~~~

The main problem with exposing Java classes and methods to python, is that
Java allows overloading a method. That is, 2 methods can have the same name
as long as they have different parameters. Python does not allow that. Most
of the time, this is not a problem. Most overloaded methods have very
different parameters and no confusion take place.

When jpype is unable to decide with overload of a method to call, the user
must resolve the ambiguity. Thats where the wrapper classes come in.

Take for example the java.io.PrintStream class. This class has a variant of
the print and println methods!

So for the following code : ::

  from jpype import *
  startJVM("d:/tools/j2sdk/jre/bin/client/jvm.dll", "-ea")
  java.lang.System.out.println(1)
  shutdownJVM()

JPype will automatically choose the println(int) method, bcause pytho
int - >java int is an exact match, while all the other integral types
are only "implicit" matches. However, if that not the version you
wanted to call ...

Changing the line thus : ::

  from jpype import *
  startJVM("d:/tools/j2sdk/jre/bin/client/jvm.dll", "-ea")
  java.lang.System.out.println(JByte(1)) # <--- wrap the 1 in a JByte
  shutdownJVM()

tells jpype to choose the byte version.

Note that wrapped object will only match to a method which takes EXACTLY that
type, even if the type is compatible. Using a JByte wrapper to call a method
requiring an int will fail.

One other area where wrappers help is performance. Native types convert quite
fast, but strings, and later tuples, maps, etc ... conversions can be very
costly.

If you're going to make many java calls with a complex object, wrapping it
once and then using the wrapper will make a huge difference.

Lasty, wrappers allow you to pass in a structure to java to have it modified.
an implicitly converted tuple will not come back modified, even if the java
method HAS changed the contents. An explicitly wrapped tuple will be
modified, so that those modifications are visible to the python program.

Lasty, wrappers allow you to pass in a structure to java to have it modified.
an implicitly converted tuple will not come back modified, even if the java
method HAS changed the contents. An explicitly wrapped tuple will be
modified, so that those modifications are visible to the python program.

The available native wrappers are : **JChar, JByte, JShort, JInt,
JLong, JFloat, JDouble, JBoolean and JString.**


JObject wrapper
:::::::::::::::

The JObject wrapper serves a few additional purposes on top of what the other
wrappers do.

While the native wrappers help to resolve ambiguities between native types,
it is impossible to create one JObject wrapper for each java Class to do the
same thing.

So, the JObject wrapper accepts 2 parameters. The first is any convertable
object. The second is the class to convert it to. It can be the name of the
class in a string or a JavaClass object. If omitted, the second parameter
will be deduced from the first.

Like other wrappers, the method called will only match EXACTLY. a JObject
wrapper of type java.lang.Int will not work when calling a method requiring a
java.lang.Number.
