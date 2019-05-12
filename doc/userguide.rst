User Guide
==========

Overview
--------

JPype is an effort to allow Python programs full access to Java class
libraries. This is achieved not through re-implementing Python, as
Jython/JPython has done, but rather through interfacing at the native level
in both virtual machines.

Eventually, it should be possible to replace Java with Python in many, though
not all, situations. JSP, Servlets, RMI servers and IDE plugins are all good
candidates.


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

Here is a sample program to demonstrate how to use JPype: ::

  from jpype import *
  startJVM(getDefaultJVMPath(), "-ea")
  java.lang.System.out.println("hello world")
  shutdownJVM()

This is of course a simple ``hello world`` type of application. Yet it shows
the 2 most important calls: ``startJVM`` and ``shutdownJVM``.

The rest will be explained in more detail in the next sections.

Core Ideas
----------

Threading
---------

Any non-trivial application will have need of threading. Be it implicitly by
using a GUI, or because you're writing a multi-user server. Or explicitly for
performance reason.

The only real problem here is making sure Java threads and Python threads
cooperate correctly. Thankfully, this is pretty easy to do.

Python Threads
~~~~~~~~~~~~~~

For the most part, Python threads based on OS level threads (i.e. posix
threads) will work without problem. The only thing to remember is to call
``jpype.attachThreadToJVM()`` in the thread body to make the JVM usable from
that thread. For threads that you do not start yourself, you can call
``isThreadAttachedToJVM()`` to check.

Java Threads
~~~~~~~~~~~~

At the moment, it is not possible to use threads created from Java, since
there is no ``callback`` available.

Other Threads
~~~~~~~~~~~~~

Some Python libraries offer other kinds of thread, (i.e. microthreads). How
they interact with Java depends on their nature. As stated earlier, any OS-
level threads will work without problem. Emulated threads, like microthreads,
will appear as a single thread to Java, so special care will have to be taken
for synchronization.

Synchronization
~~~~~~~~~~~~~~~

Java synchronization support can be split into 2 categories. The first is the
``synchronized`` keyword, both as prefix on a method and as a block inside a
method. The second are the different methods available on the Object class
(``notify, notifyAll, wait``).

To support the ``synchronized`` functionality, JPype defines a method called
``synchronized(O)``. O has to be a Java object or Java class, or a Java wrapper
that corresponds to an object (``JString`` and ``JObject``). The return value is a
monitor object that will keep the synchronization on as long as the object is
kept alive. The lock will be broken as soon as the monitor is GCd. So make
sure to hang on to it as long as you need it.

The other methods are available as-is on any ``_JavaObject``.

For synchronization that does not have to be shared with Java code, I suggest
using Python's support instead of Java's, as it'll be more natural and easier.

Performance
-----------

JPype uses JNI, which is well known in the Java world as not being the most
efficient of interfaces. Further, JPype bridges two very different runtime
environments, performing conversion back and forth as needed. Both of these
can impose rather large performance bottlenecks.

JNI is the standard native interface for most, if not all, JVMs, so there is
no getting around it. Down the road, it is possible that interfacing with CNI
(GCC's java native interface) may be used. The only way to minimize the JNI 
cost is to move some code over to Java.

Follow the regular Python philosophy : ``Write it all in Python, then write
only those parts that need it in C.`` Except this time, it's write the parts
that need it in Java.

For the conversion costs, again, nothing much can be done. In cases where a
given object (be it a string, an object, an array, etc ...) is passed often
into Java, you can pre-convert it once using the wrappers, and then pass in
the wrappers. For most situations, this should solve the problem.

As a final note, while a JPype program will likely be slower than its pure
Java counterpart, it has a good chance of being faster than the pure Python
version of it. The JVM is a memory hog, but does a good job of optimizing
code execution speeds.

Inner Classes
-------------

For the most part, inner classes can be used like normal classes, with the
following differences:

- Inner classes in Java natively use $ to separate the outer class from
  the inner class. For example, inner class Foo defined inside class Bar is
  called Bar.Foo in Java, but its real native name is Bar$Foo.
- Inner classes appear as member of the containing class.  Thus 
  to access them simply import the outer class and call them as 
  members.
- Non-static inner classes cannot be instantiated from Python code.
  Instances received from Java code that can be used without problem.

Arrays
------

JPype has full support for receiving Java arrays and passing them to Java
methods. Java arrays, wrapped in the JArray wrapper class, behave like Python
lists, except that their size is fixed, and that the contents are of a
specific type.

Multi-dimensional arrays (array of arrays) also work without problem.

As of version 0.5.5.3 we use NumPy arrays to interchange data with Java. This 
is much faster than using lists, since we do not need to handle every single 
array element but can process all data at once.

If you do not want this optional feature, because eg. it depends on NumPy, you
can opt it out in the installation process by passing *"--disable-numpy"* to 
*setup.py*. To opt out with pip you need to append the additional argument
*"--global-option='--disable-numpy'*. This possibility exists since version 
0.5.6.

Creating Java arrays from Python
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The JArray wrapper is used to create Arrays from Python code. The code to
create an array is like this: ::

  JArray(type, num_dims)(sz or sequence)

Type is either a Java Class (as a String or a Java Class object) or a Wrapper
type. num_dims is the number of dimensions to build the array and defaults to
1.

sz is the actual number of elements in the arrays, and sequence is a sequence
to initialize the array with.

The logic behind this is that ``JArray(type, ndims)`` returns an Array Class,
which can then be called like any other class to create an instance.

Type conversion
---------------

One of the most complex parts of a bridge system like JPype is finding a way
to seamlessly translate between Python types and Java types. The following
table will show what implicit conversions occur, both Python to Java and Java
to Python. Explicit conversion, which happens when a Python object is
wrapped, is converted in each wrapper.

Conversion from Python to Java
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This type of conversion happens when a Python object is used either as a
parameter to a Java method or to set the value of a Java field.

Type Matching
~~~~~~~~~~~~~

JPype defines different levels of "match" between Python objects and Java
types. These levels are:

- ``none``, There is no way to convert.
- ``explicit (E)``, JPype can convert the desired type, but only
  explicitly via the wrapper classes. This means the proper wrapper class
  will access this type as argument.
- ``implicit (I)``, JPype will convert as needed.
- ``exact> (X)``, Like implicit, but when deciding with method overload
  to use, one where all the parameters match "exact" will take precedence
  over "implicit" matches.

============ ========== ========= =========== ========= ========== ========== =========== ========= ========== ========== =========== =========
Python\\Java    byte      short       int       long       float     double     boolean     char      String      Array     Object      Class   
============ ========== ========= =========== ========= ========== ========== =========== ========= ========== ========== =========== =========
    int       I [1]_     I [1]_       X          I        I [11]_    I [11]_    X [10]_                                               
   long       I [1]_     I [1]_     I [1]_       X        I [11]_    I [11]_                                                                   
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
 "Boxed"      I [12]_    I [12]_    I [12]_     I [12]_   I [12]_    I [12]_    I [12]_                                     I/X [8]_ 
============ ========== ========= =========== ========= ========== ========== =========== ========= ========== ========== =========== =========

.. [1] Conversion will occur if the Python value fits in the Java
       native type.

.. [2] Conversion occurs if the Python string or unicode is of
       length 1.

.. [3] The required object must be of a type compatible with
       ``java.lang.String(java.lang.Object, java.util.Comparable)``.

.. [4] Number of dimensions must match, and the types must be
       compatible.

.. [5] Only when the required type is ``java.lang.Object``.

.. [6] Only if the JObject wrapper's specified type is an compatible
       array class.

.. [7] Only if the required type is compatible with the wrappers's
       specified type. The actual type of the Java object is not
       considered.

.. [8] Only if the required type is compatible with the Java Object
       actual type.

.. [9] Only when the required type is ``java.lang.Object`` or
       ``java.lang.Class``.

.. [10] Only the values True and False are implicitly converted to
        booleans.

.. [11] Java defines conversions from integer types to floating point 
        types as implicit conversion.  Java's conversion rules are based
        on the range and can be lossy.
        See (http://stackoverflow.com/questions/11908429/java-allows-implicit-conversion-of-int-to-float-why)

.. [12] Java boxed types are mapped to python primitives, but will 
        produce an implicit conversion even if the python type is an exact 
        match.  This is to allow for resolution between methods 
        that take both a java primitve and a java boxed type.

Converting from Java to Python
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The rules here are much simpler.

Java ``byte, short and int`` are converted to Python ``int``.

Java ``long`` is converted to Python ``long``.

Java ``float and double`` are converted to Python ``float``.

Java ``boolean`` is converted to Python ``int`` of value 1 or 0.

Java ``char`` is converted to Python ``unicode`` of length 1.

Java ``String`` is converted to Python ``unicode``.

Java ``arrays`` are converted to ``JArray``.

Java ``boxed`` types are converted to extensions of python primitives on return.

All other Java objects are converted to ``JavaObjects``.

Java ``Class`` is converted to ``JavaClass``.

Java array ``Class`` is converted to ``JavaArrayClass``.

Boxed types
~~~~~~~~~~~

Both python primitives and Boxed types are immutable.  Thus boxed types are
inherited from the python primitives.  This means that a boxed type regardless
of whether produced as a return or created explicitely are treated as python
types.  They will obey all the conversion rules corresponding
to a python type as implicit matches.  In addition, they will produce an exact 
match with their corresponding java type.  The type conversion for this is 
somewhat looser than java.  While java provides automatic unboxing of a Integer 
to a double primitive, jpype can implicitly convert Integer to a Double boxed.

To box a primitive into a specific type such as to place in on a ``java.util.List``
use ``JObject`` on the desired boxed type.  For example: ::

    from jpype.types import *
    from jpype import java
    ...
    lst = java.util.ArrayList()
    lst.add(JObject(JInt(1)))
    print(type(lst.get(0)))


JProxy
------

The ``JProxy`` allows Python code to "implement" any number of Java interfaces, so
as to receive callbacks through them.

Using ``JProxy`` is simple. The constructor takes 2 arguments. The first is one
or a sequence of string of JClass objects, defining the interfaces to be
"implemented". The second must be a keyword argument, and be either ``dict``
or ``inst``. If ``dict`` is specified, then the 2nd argument must be a
dictionary, with the keys the method names as defined in the interface(s),
and the values callable objects. If ``inst`` an object instance must be
given, with methods defined for the methods declared in the interface(s).
Either way, when Java calls the interface method, the corresponding Python
callable is looked up and called.

Of course, this is not the same as subclassing Java classes in Python.
However, most Java APIs are built so that subclassing is not needed. Good
examples of this are AWT and SWING. Except for relatively advanced features,
it is possible to build complete UIs without creating a single subclass.

For those cases where subclassing is absolutely necessary (i.e. using Java's
SAXP classes), it is generally easy to create an interface and a simple
subclass that delegates the calls to that interface.


Sample code :
~~~~~~~~~~~~~

Assume a Java interface like: ::

  public interface ITestInterface2
  {
          int testMethod();
          String testMethod2();
  }

You can create a proxy *implementing* this interface in 2 ways.
First, with a class: ::

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

Error handling is an important part of any non-trivial program. 
All Java exceptions occurring within java code raise a ``jpype.JException`` which 
derives from python Exception.  These can be caught either using a specific 
java exception or generically as a ``jpype.JException`` or ``java.lang.Throwable``.  
You can then use the ``stacktrace()``, ``str()``, and args to access extended information.

Here is an example: ::

  try :
          # Code that throws a java.lang.RuntimeException
  except java.lang.RuntimeException as ex:
        print("Caught the runtime exception : ", str(ex))
        print(ex.stacktrace())

Multiple java exceptions can be caught together or separately: ::

  try :
        ...
  except (java.lang.ClassCastException, java.lang.NullPointerException) as ex:
        print("Caught multiple exceptions : ", str(ex))
        print(ex.stacktrace())
  except java.lang.RuntimeException as ex:
        print("Caught runtime exception : ", str(ex))
        print(ex.stacktrace())
  except jpype.JException:
        print("Caught base exception : ", str(ex))
        print(ex.stacktrace())
  except Exception as ex:
        print("Caught python exception :", str(ex))

Exceptions can be raised in proxies to throw an exception back to java. 

Exceptions within the jpype core are issued with the most appropriate 
python exception type such as ``TypeError``, ``ValueError``, ``AttributeError``, 
or ``OSError``.

Using ``jpype.JException`` with a class name as a string was supported in previous JPype 
versions but is currently deprecated.

Customizers
-----------

Java wrappers can be customized to better match the expected behavior in python.
Customizers are defined using annotations.  Currently the annotations ``@JImplementionFor``
and ``@JOverride`` can be applied to a regular class to customize an existing class.
``@JImplementationFor`` requires the class name as a string so that it can be applied
to the class before the JVM is started.  ``@JOverride`` can be applied method to 
hide the java implementation allowing a python functionality to be placed into method.
If a java method is overridden it is renamed with an proceeding underscore to 
appear as a private method.  Optional arguments to ``@JOverride`` can be used to 
control the renaminging and force the method override to apply to all classes that 
derive from a base class ("sticky").

Generally speaking, a customizer should be defined before the first instance of a
given class is created so that the class wrapper and all instances will have the 
customization.

Example taken from JPype ``java.util.Collection`` customizer: ::

  @JImplementationFor("java.util.Collection")
  class _JCollection(object):

      # Support of len(obj)
      def __len__(self):
          return self.size()

      def __delitem__(self, i):
          return self.remove(i)

      # addAll does not automatically convert to 
      # a Collection, so we can augment that 
      # behavior here.
      @JOverride(sticky=True)
      def addAll(self, v):
          if isPythonSequence(v):
              r = False
              for i in v:
                  r = self.add(i) or r
              return r
          else:
              return self._addAll(v)

The name of the class does not matter for the purposes of customizer though 
it should probabily be a private class so that it does not get used accidentally.
The customizer code will steal from the prototype class rather than acting as a
base class, thus ensuring that the methods will appear on the most derived 
python class and are not hidden by the java implementations.  The customizer will 
copy methods, callable objects, ``__new__``, class member strings, and properties.


Known limitations
-----------------

This section lists those limitations that are unlikely to change, as they come
from external sources.


Unloading the JVM
~~~~~~~~~~~~~~~~~

The JNI API defines a method called ``destroyJVM()``. However, this method does
not work. That is, Sun's JVMs do not allow unloading. For this reason, after
calling ``shutdownJVM()``, if you attempt calling ``startJVM()`` again you will get
a non-specific exception. There is nothing wrong (that I can see) in JPype.
So if Sun gets around to supporting its own properly, or if you use JPype
with a non-SUN JVM that does (I believe IBM's JVMs support JNI invocation, but
I do not know if their destroyJVM works properly), JPype will be able to take
advantage of it. As the time of writing, the latest stable Sun JVM was
1.4.2_04.


Methods dependent on "current" class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are a few methods in the Java libraries that rely on finding
information on the calling class. So these methods, if called directly from
Python code, will fail because there is no calling Java class, and the JNI
API does not provide methods to simulate one.

At the moment, the methods known to fail are :


java.sql.DriverManager.getConnection(...)
:::::::::::::::::::::::::::::::::::::::::

For some reason, this class verifies that the driver class as loaded in the
"current" classloader is the same as previously registered. Since there is no
"current" classloader, it defaults to the internal classloader, which
typically does not find the driver. To remedy, simply instantiate the driver
yourself and call its ``connect(...)`` method.

Unsupported Python versions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

PyPy 2.7 has issues with the Python meta class programming.  PyPy 3 appears
to work, but does not have very aggressive memory deallocation.  Thus PyPy
3 fails the leak test.


Unsupported Java virtual machines
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The open JVM implementations *Cacao* and *JamVM* are known not to work with
JPype.


Module Reference
----------------

``jpype.imports`` module
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``jpype.imports`` module provides a runtime safe method to import a 
java class into the project scope.  Imports of java classes can only 
occur after the JVM is started.  

Example: ::
     import jpype
     import jpype.imports

     jpype.startJVM(jpype.getDefaultJVMPath())

     #import java classes
     from java.lang import String
     from java.util import ArrayList as jlist
     from java.util import HashMap, TreeMap

Inner classes are loaded into the class scope.
If the class cannot be located when importing an ``ImportError`` is raised.


``jpype.types`` module
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``jpype.types`` module contains all of the required type wrappers for using
JPype.  It can be used to simplify coding without pulling in unnecessary 
symbols.  Currently this contains: ``JArray``, ``JBoolean``, ``JByte``, ``JChar``,
``JClass``, ``JDouble``, ``JException``, ``JFloat``, ``JInt``, ``JInterface``, ``JLong``,
``JObject``, ``JShort``, and ``JString``.

Example: ::

     import jpype
     from jpype.types import *


``jpype.reflect`` module
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This module is deprecated and will be removed.  The functionality for 
getting reflection on java classes is currently supported with the 
``class_`` field in java classes and objects.

Example: ::

     from jype import java
     for method in java.lang.String.class_.getDeclaredMethods():
          print(method)

addClassPath method
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This method manually adds a java class path into the ``getClassPath``
method results.


getClassPath method
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This method gets the class path for java with the correct platform dependent
seperator.  This can be used to define the class path when starting the 
JVM. The class path can be altered either by using os.environ or by using 
``jpype.addClassPath()``.  This is useful when building platform independent
python modules.  Some platforms such as cygwin have a mismatch between 
the java seperator and the python file seperator.

Arguments
:::::::::

env is an optional boolean argument that defaults to true.  If env is 
false than only those paths defined by addClassPath are used.

Return value
::::::::::::

valid path classpath.  Wildcards in the path are expanded to include 
all jars found in the path.

Exceptions
::::::::::
None.


getDefaultJVMPath method
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This method tries to automatically obtain the path to a Java runtime
installation. This path is needed as argument for startJVM method and should
be used in favour of hardcoded paths to make your scripts more portable.
There are several methods under the hood to search for a JVM. If none
of them succeeds, the method will raise a ``JVMNotFoundException``.

Arguments
:::::::::

None

Return value
::::::::::::

valid path to a Java virtual machine library (``jvm.dll``, ``jvm.so``, ``jvm.dylib``)

Exceptions
::::::::::
``JVMNotSupportedException``, if none of the provided methods returned a valid JVM path.


startJVM method
~~~~~~~~~~~~~~~~~

This method MUST be called before any other JPype features can be used. It
will initialize the specified JVM.  Use ``isJVMStarted()`` to verify if it is 
necessary to start in multiple places.

Arguments
:::::::::

-   vmPath - Must be the path to the ``jvm.dll`` (or ``jvm.so``, depending on
    platform)
-   misc arguments - All arguments after the first are optional, and are
    given as it to the JVM. Pretty much any command-line argument you can
    give the JVM can be passed here. A caveat, multi-part arguments (like
    ``-classpath``) do not seem to work, and must e passed in as a ``-D`` option.
    Option ``-classpath a;b;c`` becomes ``-Djava.class.path=a;b;c``


Return value
::::::::::::

None


Exceptions
::::::::::

If the JVM is already started a ``OSError`` is raised.


shutdownJVM method
~~~~~~~~~~~~~~~~~~

For the most part, this method does not have to be called. It will be
automatically executed when the ``jpype`` module is unloaded at Python's exit.


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

``attachThreadToJVM`` is called when a new thread is created in python and 
must be attached to the JVM.  Currently, this method is deprecated as JPype
automatically attached threads when they are encounted.  Automatic 
attachment is a requirement as often third party programs such as sypder
create threads and attempt to call java method which would result in 
a crash.  This can create a resource leak as each thread that is attached
will consume an additional java object.  If this is an issue manually
detach the thread as they are destroyed.

Arguments
:::::::::

None


Return value
::::::::::::

None


isThreadAttachedToJVM method
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This method can be used to determine if a thread is attached to the JVM.
This method is currently broken as the act of checking if a thread is 
attached caused the thread to become attached.


Arguments
:::::::::

None


Return value
::::::::::::

``True`` if the thread is attached.


detachThreadFromJVM method
~~~~~~~~~~~~~~~~~~~~~~~~~~

For the most part, this method does not have to be called. It will be
automatically executed when the jpype module is unloaded at Python's exit.
Programs with a large number of threads calling java methods, can call
this method prior to the termination of the thread to release the
java resources associated with the thread.

Arguments
:::::::::

None


Return value
::::::::::::

None


Exceptions
::::::::::

On failure, a ``RuntimeException`` is raised.


synchronized method
~~~~~~~~~~~~~~~~~~~

synchronized can be used to create a threads safe lock on a java 
object for a limited period of time.  It is used with the python 
``with`` statement to create a block that locks an object.

Example: ::
  from jpype import synchronized

  ... # Get an object from java that requires a thread lock
  with synchronized(obj):
      # Thread-safe access within the block
      obj.modifyObject()

  # No thread-safe access outside the block


Arguments
:::::::::

A Java object to lock on.


Return value
::::::::::::

A monitor object which will release the object at the end of the scope.


Exceptions
::::::::::

On failure, a ``TypeError`` is raised if the object is a null pointer, a primitive,
or is not a python object.


JPackage class
~~~~~~~~~~~~~~

This class allows structured access to Java packages and classes. 
This functionality has been replaced by ``jpype.imports``, but is still
provided to support older code.

Only the root of the package tree need be declared with the ``JPackage``
constructor. Sub-packages will be created on demand.

For example, to import the w3c DOM package: ::

  Document = JPackage('org').w3c.dom.Document

Under some situations such as a missing jar the resulting object
will be a JPackage object rather than the expected java class.  This
results in rather challanging debugging messages.  Thus the 
jpype.imports module is preferred.


Predefined Java packages
::::::::::::::::::::::::

For convenience, the jpype module predefines the following ``JPackage`` :
``java, javax``

They can be used as-is, without needing to resort to the ``JPackage``
class.


Wrapper classes
~~~~~~~~~~~~~~~

The main problem with exposing Java classes and methods to Python, is that
Java allows overloading a method. That is, 2 methods can have the same name
as long as they have different parameters. Python does not allow that. Most
of the time, this is not a problem. Most overloaded methods have very
different parameters and no confusion takes place.

When JPype is unable to decide with overload of a method to call, the user
must resolve the ambiguity. That's where the wrapper classes come in.

Take for example the ``java.io.PrintStream`` class. This class has a variant of
the print and println methods!

So for the following code: ::

  from jpype import *
  startJVM(getDefaultJVMPath(), "-ea")
  java.lang.System.out.println(1)
  shutdownJVM()

JPype will automatically choose the ``println(int)`` method, because the Python
int matches exactly with the Java int, while all the other integral types
are only "implicit" matches. However, if that is not the version you
wanted to call ...

Changing the line thus: ::

  from jpype import *
  startJVM(getDefaultJVMPath(), "-ea")
  java.lang.System.out.println(JByte(1)) # <--- wrap the 1 in a JByte
  shutdownJVM()

tells JPype to choose the byte version.

Note that wrapped object will only match to a method which takes EXACTLY that
type, even if the type is compatible. Using a JByte wrapper to call a method
requiring an int will fail.

One other area where wrappers help is performance. Native types convert quite
fast, but strings, and later tuples, maps, etc ... conversions can be very
costly.

If you're going to make many Java calls with a complex object, wrapping it
once and then using the wrapper will make a huge difference.

Lastly, wrappers allow you to pass in a structure to Java to have it modified.
An implicitly converted tuple will not come back modified, even if the Java
method HAS changed the contents. An explicitly wrapped tuple will be
modified, so that those modifications are visible to the Python program.

The available native wrappers are: ``JChar``, ``JByte``, ``JShort``, ``JInt``,
``JLong``, ``JFloat``, ``JDouble``, ``JBoolean`` and ``JString``.


JObject wrapper
:::::::::::::::

The ``JObject`` wrapper serves a few additional purposes on top of what the other
wrappers do.  ``JObject`` serves as the base class for java classes that derive 
from ``java.lang.Object``.

While the native wrappers help to resolve ambiguities between native types,
it is impossible to create one ``JObject`` wrapper for each Java Class to do the
same thing.

So, the ``JObject`` wrapper accepts two parameters. The first is any convertible
object. The second is the class to convert it to.  Thus ``JObject`` can serve as
a cast operator when used to match overloads.  The second arguments can be the 
name of the class in a string or a ``JClass`` object. If omitted, the second parameter
will be deduced from the first.

Like other wrappers, the method called will only match EXACTLY. A ``JObject``
wrapper of type ``java.lang.Int`` will not work when calling a method requiring a
``java.lang.Number``.



JClass wrapper
::::::::::::::

The ``JClass`` wrapper serves as the meta class for all java class instances and 
as a factory for new java classes.  If called with a string, it will find the 
java class and create a python wrapper.  If called with an existing java class value
instance it will create the corresponding python wrapper.  ``JClass`` has a 
strange relationships to java classes as it is a meta class.  Thus, a java
class wrapper is an instance of a ``JClass``.


JException class
::::::::::::::::::

The ``JException`` wrapper serves as the base class for all java exceptions.
It currently accepts a string to create a java class instance, but this 
functionality is deprecated and will be removed.

Example: ::

     ...
     except Exception as ex:
        if isinstance(ex, jpype.JException):
             print(ex.stacktrace())


JInterface class
::::::::::::::::::

The ``JInterface`` is serves as the base class for any java class that is a pure 
interface without implementation.  It is not possible to create a instance of 
a java interface.  The mro is hacked such that ``JInterface`` does not appear 
in the tree of objects implement an interface.

Example: ::
  
     if issubclass(java.util.function.Function, jpype.JInterface):
          print("is interface")


JArray class
::::::::::::

The ``JArray`` class is the base class used as a factory for all java arrays. 
See the section of java arrays for useage.

One can test if an object is a java arrays using ``isinstance``: ::

      if isinstance(obj, jpype.JArray):
           print("object is a java array")

      if issubclass(cls, jpype.JArray):
           print("class is a java array type.")


