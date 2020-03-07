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

Here is a sample program to demonstrate how to use JPype:

.. code-block:: python

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

Java synchronization support can be split into two categories. The first is the
``synchronized`` keyword, both as prefix on a method and as a block inside a
method. The second are the different methods available on the Object class
(``notify, notifyAll, wait``).

To support the ``synchronized`` functionality, JPype defines a method called
``synchronized(O)``. O has to be a Java object or Java class. The return value is a
monitor object that will keep the synchronization on as long as the object is
kept alive. Use Python ``with`` statement to control the exact scope.  Do
not hold onto the object indefinitly without a ``with`` statement, the lock
will be not be broken until the monitor is garbage collected.  CPython and
PyPy have difference GC rules.  See :ref:`synchronized <synchronized>` for details of how
to properly synchronize.

The other synchronization methods are available as-is on any ``JObject``.  However, as
general rule one should not use synchronization methods on Java String as
internal string representations may not be complete objects.

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

Follow the regular Python philosophy : **Write it all in Python, then write
only those parts that need it in C.** Except this time, it's write the parts
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
- Inner classes appear as member of the containing class. Thus
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

There are special rules for ``java.lang.Object`` as compared with a
specific Java object.  In Java, primitives are boxed automatically when
passing to a ``java.lang.Object``.

============== ========== ========= =========== ========= ========== ========== =========== ========= ========== =========== ========= ================== =================
Python\\Java    byte      short       int       long       float     double     boolean     char      String      Array       Object    java.lang.Object   java.lang.Class
============== ========== ========= =========== ========= ========== ========== =========== ========= ========== =========== ========= ================== =================
    int         I [1]_     I [1]_       X          I        I [3]_     I [3]_     X [8]_                                                       I [11]_                   
   long         I [1]_     I [1]_     I [1]_       X        I [3]_     I [3]_                                                                  I [11]_                 
   float                                                    I [1]_       X                                                                     I [12]_                 
 sequence                                                                                                                                                         
dictionary                                                                                                                                                        
  string                                                                                     I [2]_       X                                    I                    
  unicode                                                                                    I [2]_       X                                    I                   
   JByte          X                                                                                                                            I [9]_                   
  JShort                     X                                                                                                                 I [9]_                   
   JInt                                 X                                                                                                      I [9]_                   
   JLong                                           X                                                                                           I [9]_                  
  JFloat                                                      X                                                                                I [9]_                  
  JDouble                                                                X                                                                     I [9]_                   
 JBoolean                                                                           X                                                          I [9]_                   
   JChar                                                                                       X                                               I [9]_                   
  JString                                                                                                 X                                    I               
  JArray                                                                                                          I/X [4]_                     I               
  JObject                                                                                                         I/X [6]_    I/X [7]_         I/X [7]_             
  JClass                                                                                                                                       I                  X
 "Boxed"[10]_     I          I          I          I          I          I          I                                                          I             
============== ========== ========= =========== ========= ========== ========== =========== ========= ========== =========== ========= ================== =================

.. [1] Conversion will occur if the Python value fits in the Java
       native type.

.. [2] Conversion occurs if the Python string or unicode is of
       length 1.

.. [3] Java defines conversions from integer types to floating point
       types as implicit conversion. Java's conversion rules are based
       on the range and can be lossy.
       See (http://stackoverflow.com/questions/11908429/java-allows-implicit-conversion-of-int-to-float-why)

.. [4] Number of dimensions must match, and the types must be
       compatible.

.. [6] Only if the specified type is an compatible array class.

.. [7] Exact is the object class is an exact match, otherwise
       implicit.

.. [8] Only the values True and False are implicitly converted to
       booleans.

.. [9] Primitives are boxed as per Java rules.

.. [10] Java boxed types are mapped to python primitives, but will
        produce an implicit conversion even if the python type is an exact
        match. This is to allow for resolution between methods
        that take both a java primitve and a java boxed type.

.. [11] Boxed to ``java.lang.Long`` as there is no difference
        between long and int in Python3,

.. [12] Boxed to ``java.lang.Double``

Converting from Java to Python
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The rules here are much simpler.

- Java ``byte, short and int`` are converted to Python ``int``.

- Java ``long`` is converted to Python ``long``.

- Java ``float and double`` are converted to Python ``float``.

- Java ``boolean`` is converted to Python ``int`` of value 1 or 0.

- Java ``char`` is converted to Python ``unicode`` of length 1.

- All Java objects are converted to ``JObject``.

- Java ``Throwable`` is converted to ``JException`` derived from ``JObject``.

- Java ``String`` is converted to ``JString`` derived from ``JObject``.

- Java **arrays** are converted to ``JArray`` derived from ``JObject``.

- Java **boxed** types are converted to ``JObject`` with extensions of python primitives on return.

Casting
~~~~~~~

The main problem with exposing Java classes and methods to Python, is that
Java allows overloading a method. That is, multiple methods can have the same
name as long as they have different parameters. Python does not allow that. Most
of the time, this is not a problem. Most overloaded methods have very
different parameters and no confusion takes place.

When JPype is unable to decide with overload of a method to call, the user
must resolve the ambiguity. That's where the wrapper classes come in.

Take for example the ``java.io.PrintStream`` class. This class has a variant of
the print and println methods!

So for the following code:

.. code-block:: python

  from jpype import *
  startJVM(getDefaultJVMPath(), "-ea")
  java.lang.System.out.println(1)
  shutdownJVM()

JPype will automatically choose the ``println(int)`` method, because the Python
int matches exactly with the Java int, while all the other integral types
are only "implicit" matches. However, if that is not the version you
wanted to call ...

Changing the line thus:

.. code-block:: python

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
costly.  If you're going to make many Java calls with a complex object,
wrapping it once and then using the wrapper will make a huge difference.

Casting using the Java types is also usedful when placing objects in generic
containers such as Java List or Map. Both primitive and boxed type Java
object derive from the corresponding Python type, so they will work with any
Python call.


Boxed types
~~~~~~~~~~~

Both python primitives and Boxed types are immutable. Thus boxed types are
inherited from the python primitives. This means that a boxed type regardless
of whether produced as a return or created explicitely are treated as python
types. They will obey all the conversion rules corresponding
to a python type as implicit matches. In addition, they will produce an exact
match with their corresponding java type. The type conversion for this is
somewhat looser than java. While java provides automatic unboxing of a Integer
to a double primitive, jpype can implicitly convert Integer to a Double boxed.

To box a primitive into a specific type such as to place in on a ``java.util.List``
use ``JObject`` on the desired boxed type. For example:

.. code-block:: python

    from jpype.types import *
    from jpype import java
    # ...
    lst = java.util.ArrayList()
    lst.add(JObject(JInt(1)))
    print(type(lst.get(0)))

Implementing interfaces
-----------------------

At times it is necessary to implement an interface in python especially to use
classes that require java lambdas.  To implement an interface contruct a
python class and decorate it with annotations ``@JImplements`` and ``@JOverride``.

.. code-block:: python

  from jpype import JImplements, JOverride
  from java.lang.util import DoubleUnaryOperator
  # ...
  @JImplements(DoubleUnaryOperator)
  class MyImpl(object):
      @JOverride
      def applyAsDouble(self, value):
          return 123+value

The java interface may specified by a java wrapper or using a string naming the
class.  Multiple interfaces can be implemented by a single class by giving a
list of interfaces.   Alternatively, the interface can be implemented using
JProxy.

In a future release, Python callables will be able to automatically match to
interfaces that have the Java annotation ``@FunctionalInterface``.


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

Assume a Java interface like:

.. code-block:: java

  public interface ITestInterface2
  {
          int testMethod();
          String testMethod2();
  }

You can create a proxy *implementing* this interface in 2 ways.
First, with a class:

.. code-block:: python

  class C :
          def testMethod(self) :
                  return 42

          def testMethod2(self) :
                  return "Bar"

  c = C()
  proxy = JProxy("ITestInterface2", inst=c)

or you can do it with a dictionary

.. code-block:: python

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
derives from python Exception. These can be caught either using a specific
java exception or generically as a ``jpype.JException`` or ``java.lang.Throwable``.
You can then use the ``stacktrace()``, ``str()``, and args to access extended information.

Here is an example:

.. code-block:: python

  try :
          # Code that throws a java.lang.RuntimeException
  except java.lang.RuntimeException as ex:
        print("Caught the runtime exception : ", str(ex))
        print(ex.stacktrace())

Multiple java exceptions can be caught together or separately:

.. code-block:: python

  try:
        #  ...
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
Customizers are defined using annotations. Currently the annotations ``@JImplementionFor``
and ``@JOverride`` can be applied to a regular class to customize an existing class.
``@JImplementationFor`` requires the class name as a string so that it can be applied
to the class before the JVM is started. ``@JOverride`` can be applied method to
hide the java implementation allowing a python functionality to be placed into method.
If a java method is overridden it is renamed with an proceeding underscore to
appear as a private method. Optional arguments to ``@JOverride`` can be used to
control the renaminging and force the method override to apply to all classes that
derive from a base class ("sticky").

Generally speaking, a customizer should be defined before the first instance of a
given class is created so that the class wrapper and all instances will have the
customization.

Example taken from JPype ``java.util.Collection`` customizer:

.. code-block:: python

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
python class and are not hidden by the java implementations. The customizer will
copy methods, callable objects, ``__new__``, class member strings, and properties.

In addition, one can add a custom converter method which is called whenever a specified
Python type is passed to a particular Java type.  To specify a conversion method
add ``@JConversion`` to an ordinary Python function with the name of Java class
to be converted to and one keyword of ``exact``, ``instanceof`` or ``attribute`` specified.
The keyword control how strictly the conversion will be applied.  ``exact`` is 
restricted to Python objects whose type exactly matched the specified type.
``instanceof`` accepts anything that matches isinstance to the specified type.
``attribute`` checks for a particular attribute being on supplied type and thus
allows a duck typing conversion.

User supplied conversions are tested after all internal conversions have been exhausted
and are always consider to be an implicit conversion.

.. code-block:: python
        @_jcustomizer.JConversion("java.util.Collection", instanceof=Sequence)
        def _JSequenceConvert(jcls, obj):
            return _jclass.JClass('java.util.Arrays').asList(obj)



Collections
-----------

JPype uses customizers to augment Java collection classes to operate like Python collections.
Enhanced objects include ``java.util.List``, ``java.util.Set``, ``java.util.Map``, and 
``java.util.Iterator``.  These classes generally comply with the Python API except in
cases where there is a significant name conflict and thus no special treatment is required
when handling these Java types.

Java List classes such as ArrayList and LinkedList can be used in Python for loops and
list comprehensions directly.  A Java list can be converted to a Python list or the 
reverse by simply called the requested type as a copy constructor.

.. code-block:: python
     pylist = ['apple', 'orange', 'pears']

     # Copy the Python list to Java.
     jlist = JClass('java.util.ArrayList')(pylist)

     # Copy the Java list back to Python.
     pylist2 = list(jlist)

 Note that the individual list elements are still Java objects when converted to Python
 and thus a list comprehension would be required to force Python types if required.
 Converting to Java will attempt to convert each argument individually to Java.  If there
 is no conversion it will produce a ``TypeError``.  The conversion can be forced by
 casting to the appropraite Java type with a list comprehension or by defining a 
 new conversion customizer.

 Similarly Java maps are interchangable with Python dict.  By applying these customizers
 to the Java types, we avoid requiring special handing methods.


Known limitations
-----------------

This section lists those limitations that are unlikely to change, as they come
from external sources.

Restarting the JVM
~~~~~~~~~~~~~~~~~~

JPype caches many resources to the JVM. Those resource are still allocated
after the JVM is shutdown as there are still Python objects that point to
those resources.  If the JVM is restarted, those stale Python objects will be
in a broken state and the new JVM instance will obtain the references to these
resulting in a memory leak. Thus it is not possible to start the JVM after it
has been shutdown with the current implementation.

Running multiple JVM
~~~~~~~~~~~~~~~~~~~~

JPype uses the Python global import module dictionary, a global Python to
Java class map, and global JNI typemanager map.  These resources are all
tied to the JVM that is started or attached. Thus operating more than one
JVM does not appear to be possible under the current implementation.
Difficulties that would need to be overcome to remove this limitation include:

- All available JVM implementations support on one JVM instance per
  process. Thus a communication layer would have to proxy JNI
  class from JPype to another process.
- Which JVM would a static class method call. Thus the class types
  would need to be JVM specific (ie. ``JClass('org.MyObject', jvm=JVM1)``)
- How would can a wrapper for two different JVM coexist in the
  ``jpype._jclass`` module with the same name if different class
  is required for each JVM.
- How would the user specify which JVM a class resource is created in
  when importing a module.
- How would objects in one JVM be passed to another.
- How can boxed and String types hold which JVM they will box to on type
  conversion.

Thus it appears prohibitive to support multiple JVMs in the JPype
class model.

Working with Multiprocessing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Because only one JVM can be started per process, JPype cannot be used
with processes created with fork.  Forks copy all memory including the
JVM.  The copied JVM usually will not function properly thus
JPype cannot support multiprocessing using fork.

To use multiprocessing with JPype, processes must be created with
"spawn".  As the multiprocessing context is usually selected at
the start and the default for unix is fork, this requires the creating
the appropraite spawn context.  To launch multiprocessing properly
the following receipe can be used.

.. code-block:: python

   import multiprocessing as mp

   ctx = mp.get_context("spawn")
   process = ctx.Process(...)
   queue = ctx.Queue()
   ...

Also when using multiprocessing, Java objects cannot be sent
through the default Queue methods as it used pickle without
any Java support.  This can be overcome by wrapping Queue to first
encode to a byte stream using the JPickle package.  By wrapping
a Queue with the Java pickler any serializable Java object can
be transferred between processes.

In addition, a standard Queue will not produce an error if is unable to
pickle a Java object.  This can cause deadlocks when using
multiprocessing IPC thus wrapping any Queue is required.


Errors reported by Python fault handler
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The JVM takes over the standard fault handlers resulting in unusual
behavior if Python handlers are installed.  As part of normal operations
the JVM will trigger a segmentation fault when starting and when
interrupting threads.  Pythons faulthandler can intercept these operations
thus reporting extraneous fault messages or preventing normal JVM
operations if Python handles it.  When operating with JPype, Python
faulthandler module should be disabled.


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


Unsupported Python versions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

CPython 2 support was removed starting in 2020.


Unsupported Java virtual machines
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The open JVM implementations *Cacao* and *JamVM* are known not to work with
JPype.

Cygwin
~~~~~~

Cygwin is currently usable in JPype, but has a number of issues for
which there is no current solution.

Cygwin does not appear to pass environment variables to the JVM properly
resulting in unusual behavior with certain windows calls. The path
separator for Cygwin does not match that of the Java dll, thus specification
of class paths must account for this. Subject to these issues JPype is usable.

PyPy
~~~~

The GC routine in PyPy 3 does not play well with Java. It runs when it thinks
that Python is running out of resources. Thus a code that allocates a lot
of Java memory and deletes the Python objects will still be holding the
Java memory until Python is garbage collected. This means that out of
memory failures can be issued during heavy operation.

Advanced Topics
---------------

Using JPype for debugging Java code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

One common use of JPype is not to develop programs in Python, but rather to
function as a Read-Eval-Print Loop for Java. When operating Java though
Python as a method of developing or debugging Java there are a few tricks
that can be used to simplify the job, beyond being able to probe and plot the
Java data structures interactively. These methods include:

1) Attaching a debugger to the Java JVM being run under JPype.
2) Attaching debugging information to a Java exception.
3) Serializing the state of a Java process to be evaluated at a later point.

We will briefly discuss each of these methods.


Attaching a Debugger
::::::::::::::::::::

Interacting with Java through a shell is great, but sometimes it is necessary
to drop down to a debugger. To make this happen we need to start the JVM
with options to support remote debugging.

.. code-block:: python

    jpype.startJVM("-Xint", "-Xdebug", "-Xnoagent",
      "-Xrunjdwp:transport=dt_socket,server=y,address=12999,suspend=n")

Then add a marker in your program when it is time to attach the debugger
in the form of a pause statement.

.. code-block:: python

    input("pause to attach debugger")
    myobj.callProblematicMethod()

When Python reaches that point during execution, switch to a Java IDE such as
Netbeans and select Debug : Attach Debugger. That brings up a window (see
example below).  After attaching (and setting desired break points) go back to
Python and hit enter to continue.  Netbeans should come to the foreground when
a breakpoint is hit.

.. image:: attach_debugger.png


Attach data to an Exception
:::::::::::::::::::::::::::

Sometimes getting to the level of a debugger is challenging especially if the
code is large and error occurs rarely. In this case it is often benefitial to
simply attach data to an exception. To do this, we need to write a small
utility class. Java exceptions are not strictly speaking expandable, but
they can be chained. Thus, it we create a dummy exception holding a
``java.util.Map`` and attach it to as the cause of the exception, it will be
passed back down the call stack until it reaches Python. We can then use
``getCause()`` to retrieve the map containing the relevant data.


Capturing the state
:::::::::::::::::::

If the program is not running in an interactive shell or the program run time
is long, we may not want to deal with the problem during execution. In this
case, we can serialize the state of the relevant classes and variables. To
use this option, we simply make sure all of the classes in Java that we are
using are Serializable, then add a condition that detects the faulty algorithm
state. When the fault occurs, we create a ``java.util.HashMap`` and populate
it with the values we wish to be able to examine from within Python. We then
use Java serialization to write this state file to disk. We then execute the
program and collect the resulting state files.

We can then return later with an interactive Python shell, and launch JPype
with a classpath for the jars and possibly a connection to debugger.
We load the state file into memory and we can then probe or execute the
methods that lead up to the fault.


