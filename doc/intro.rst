##################
JPype User Guide
##################

This chapter is the narrative front matter of the JPype documentation --
introduction, installation, a first program, use-case walkthroughs, design
philosophy, comparison with other JVM languages and alternative bridges,
and core concepts/best-practices for starting the JVM. It is
direction-neutral (Python-calling-Java framing throughout, as JPype's
primary use case), with no ``_java`` counterpart. For the reverse
direction's equivalent "first steps" material, see :doc:`quickguide_java`.

.. _introduction:

Introduction
************

.. _introduction_jpype_the_python_to_java_bridge:

JPype the Python to Java bridge
===============================
JPype is a Python module that provides direct access to Java libraries from
Python. Unlike Jython, which reimplements Python on the Java Virtual Machine
(JVM), JPype bridges Python and Java at the native level using the Java
Native Interface (JNI). This native approach implements CPython classes for
each Java type and Java support for type management while communicating at
the process level. This approach enables:

- Direct interaction between Python and Java objects.

- Access to the full range of Java libraries and APIs.

- No need to serialize objects when communicating between language.

- Unified primitive types.

- High speed transfers through shared memory between Python and Java
  for large primitive array types.

JPype is intended for Python developers who need to use Java libraries, or
Java developers who want to use Python for scripting, debugging, or
visualization.

.. _introduction_prerequisites:

Prerequisites
-------------
Before using JPype, ensure the following:

1. **Python**: JPype requires Python 3.8 or later. Check your Python version by
   running::

       python --version

2. **Java**: JPype requires a Java Runtime Environment (JRE) or Java Development
   Kit (JDK) version 11 or later. Check your Java version by running::

       java -version

3. **Architecture Compatibility**: Ensure the Python interpreter and JVM have
   matching architectures (e.g., both 64-bit or both 32-bit).

.. _introduction_installation:

Installation
------------
JPype can be installed using either `pip` or `conda`.

.. _introduction_using_pip:

Using pip
~~~~~~~~~
To install JPype via `pip`, run::

    pip install JPype1


.. _introduction_using_conda:

Using conda
~~~~~~~~~~~
To install JPype via `conda`, use::

    conda install -c conda-forge jpype1


.. _introduction_verifying_installation:

Verifying Installation
~~~~~~~~~~~~~~~~~~~~~~
After installation, verify that JPype is installed correctly by running::

    import jpype
    print("JPype installed successfully!")


.. _introduction_your_first_jpype_program:

Your First JPype Program
------------------------
The JVM must be started (with ``jpype.startJVM()``) before any Java class is
touched -- calls made before that point fail. Once started, Java classes are
imported and used directly as if they were Python classes; when the program
ends, the JVM shuts down with it, and that termination is handled
automatically. Save the following in a file named `hello_jpype.py`:

.. code-block:: python

    import jpype
    import jpype.imports

    # Start the JVM
    jpype.startJVM(classpath=["../jar/*","../classes","com.acme-1.0.jar"])

    # Import Java classes
    from java.lang import String

    # Use the Java String class
    java_string = String("Hello from Java!")
    print(java_string.toUpperCase())  # Output: HELLO FROM JAVA!


Run the script using Python::

    python hello_jpype.py

You should see the output::

    HELLO FROM JAVA!

The rest of this guide works through what each of those pieces -- library
access, collections, interface implementation, debugging -- looks like in
practice, starting with the four use cases below.


.. _introduction_jpype_use_cases:

JPype Use Cases
===============

Here are four typical reasons to use JPype.

- Access to a Java library from a Python program (Python oriented)
- Visualization of Java data structures via Matplotlib (Java oriented)
- Interactive Java and Python development including scientific and mathematical
  programming using Python as a Java shell with Spyder or Jupyter notebooks.
- Embedding Python inside a Java application to reach its scientific,
  machine-learning, or scripting ecosystem (Java oriented, reverse bridge)

Let's explore each of these options.


.. _introduction_case_1_access_to_a_java_library:

Case 1: Access to a Java library
--------------------------------

Suppose you are a hard core Python programmer.  You can easily use lambdas,
threading, dictionary hacking, monkey patching, been there, done that.  You are
hard at work on your latest project but you just need to pip in the database
driver for your customers database and you can call it a night.  Unfortunately,
it appears that your customers database will not connect to the Python database
API.  The whole thing is custom and the customer isn't going to supply you with
a Python version.  They did send you a Java driver for the database but fat
lot of good that will do for you.

Stumbling through the internet you find a module that says it can natively
load Java packages as Python modules.  Well, it's worth a shot...

So first thing the guide says is that you need to install Java and set up
a ``JAVA_HOME`` environment variable pointing to the JRE.  Then start the
JVM with classpath pointed to customers jar file. The customer sent over
an example in Java so you just have to port it into Python.

.. code-block:: java

  package com.paying.customer;

  import com.paying.customer.DataBase;

  public class MyExample {
     public void main(String[] args) {
       Database db = new Database("our_records");
       try (DatabaseConnection c = db.connect())
       {
          c.runQuery();
          while (c.hasRecords())
          {
            Record record = db.nextRecord();
            ...
          }
       }
    }
  }


It does not look too horrible to translate.  You just need to look past all
those pointless type declarations and meaningless braces.  Once you do, you
can glue this into Python and get back to what you really love, like performing
dictionary comprehensions on multiple keys.

You glance over the JPype quick start guide.  It has a few useful patterns...
set the class path, start the JVM, remove all the type declarations, and you are done.

.. code-block:: python

   # Boiler plate stuff to start the module
   import jpype
   import jpype.imports
   from jpype.types import *

   # Launch the JVM
   jpype.startJVM(classpath=['jars/database.jar'])

   # import the Java modules
   from com.paying.customer import DataBase

   # Copy in the patterns from the guide to replace the example code
   db = Database("our_records")
   with db.connect() as c:
       c.runQuery()
       while c.hasRecords():
           record = db.nextRecord()
           ...

Launch it in the interactive window.  You can get back to programming in Python
once you get a good night sleep.




.. _introduction_case_2_visualization_of_java_structures:

Case 2: Visualization of Java structures
----------------------------------------

Suppose you are a hard core Java programmer.  Weakly typed languages are for
wimps, if it isn't garbage collected it is garbage.  Unfortunately your latest
project has suffered a nasty data structure problem in one of the threads.  You
managed to capture the data structure in a serialized form but if you could just
make graph and call a few functions this would be so much easier.  But  the
interactive Java shell that you are using doesn't really have much in the way of
visualization and you don't have time to write a whole graphing applet just to
display this dataset.

So poking around on the internet you find that Python has exactly the
visualization that you need for the problem, but it only runs in CPython.  So
in order to visualize the structure, you need to get it into Python, extract
the data structures and, send it to the plotting routine.

You install conda, follow the install instructions to connect to conda-forge,
pull JPype1, and launch the first Python interactive environment that appear to
produce a plot.

You get the shell open and paste in the boilerplate start commands, and load
in your serialized object.

.. code-block:: python

   import jpype
   import jpype.imports

   jpype.startJVM(classpath = ['jars/*', 'test/classes'])

   from java.nio.file import Files, Paths
   from java.io import ObjectInputStream

   with Files.newInputStream(Paths.get("myobject.ser")) as stream:
       ois = ObjectInputStream(stream)
       obj = ois.readObject()

   print(obj)  # prints org.bigstuff.MyObject@7382f612

It appears that the structure is loaded.  The problematic structure requires you
call the getData method with the correct index.

.. code-block:: python

   d = obj.getData(1)

   > TypeError: No matching overloads found for org.bigstuff.MyObject.getData(int),
   > options are:
          public double[] org.bigstuff.MyObject.getData(double)
          public double[] org.bigstuff.MyObject.getData(int)

Looks like you are going to have to pick the right overload as it can't
figure out which overload to use.  Darn weakly typed language, how to get
the right type in so that you can plot the right data.  It says that
you can use the casting operators.

.. code-block:: python

   from jpype.types import *
   d = obj.getData(JInt(1))
   print(type(d))  # prints double[]

Great. Now you just need to figure out how to convert from a Java array into
something our visualization code can deal with.  As nothing indicates that
you need to convert the array, you just copy out of the visualization tool
example and watch what happens.

.. code-block:: python

   import matplotlib.pyplot as plt
   plt.plot(d)
   plt.show()

A graph appears on the screen.  Meaning that NumPy has not issue dealing with
Java arrays.  It looks like ever 4th element in the array is zero.
It must be the PR the new guy put in.  And off you go back to the wonderful
world of Java back to the safety of curly braces and semicolons.


.. _introduction_case_3_interactive_java:

Case 3: Interactive Java
------------------------

Suppose you are a laboratory intern running experiments at Hawkins National
Laboratory.  (For the purpose of this exercise we will ignore the fact that
Hawkins was shut down in 1984 and Java was created in 1995).  You have the test
subject strapped in and you just need to start the experiment.  So you pull up
Jupyter notebook your boss gave you and run through the cells.  You need to
add some heart wave monitor to the list of graphed results.

The relevant section of the API for the Experiment appears to be

.. code-block:: java

  package gov.hnl.experiment;

  public interface Monitor {
     public void onMeasurement(Measurement measurement);
  }

  public interface Measurement {
     public double getTime();
     public double getHeartRate();
     public double getBrainActivity();
     public double getDrugFlowRate();
     public boolean isNoseBleeding();
  }

  public class Experiment {
     public void addCondition(Instant t, Condition c);
     public void addMoniter(Monitor m);
     public void run();
  }

The notebook already has all the test conditions for the experiment set up
and the JVM is started, so you just need to implement the monitor.

Based on the previous examples, you start by defining a monitor class

.. code-block:: python

  from jpype import JImplements, JOverride
  from gov.hnl.experiment import Monitor

  @JImplements(Monitor)
  class HeartMonitor:
      def __init__(self):
          self.readings = []
      @JOverride
      def onMeasurement(self, measurement):
          self.readings.append([measurement.getTime(), measurement.getHeartRate()])
      def getResults(self):
          return np.array(self.readings)

There is a bit to unpack here.  You have implemented a Java class from within Python.
The Java implementation is simply an ordinary Python class which has be
decorated with ``@JImplements`` and ``@JOverride``.  When you forgot to place
the ``@JOverride``, it gave you the response::

  NotImplementedError: Interface 'gov.hnl.experiment.Monitor' requires
  method 'onMeasurement' to be implemented.

But once you added the ``@JOverride``, it worked properly.  The subject appears
to be getting impatient so you hurry up and set up a short run to make sure it
is working.

.. code-block:: python

  hm = HeartMonitor()
  experiment.addMonitor(hm)
  experiment.run()
  readings = hm.getResults()
  plt.plot(readings[:,0], readings[:,1])
  plt.show()

To your surprise, it says unable to find method addMonitor with an error message::

  AttributeError: 'gov.hnl.experiment.Experiment' object has no attribute 'addMonitor'


You open the cell and type ``experiment.add<TAB>``.  The line completes with
``experiment.addMoniter``.  Whoops, looks like there is typo in the interface.
You make a quick correction and see a nice plot of the last 30 seconds pop up
in a window.  Job well done, so you set the runtime back to one hour.  Looks
like you still have time to make the intern woodlands hike and forest picnic.
Though you wonder if maybe next year you should sign up for another laboratory.
Maybe next year, you will try to sign up for those orbital lasers the President
was talking about back in March.  That sounds like real fun.

(This advanced demonstration utilized the concept of :ref:`Proxies
<Proxies>` and :ref:`Code completion
<miscellaneous_topics_code_completion>`)


.. _introduction_case_4_embedding_python:

Case 4: Embedding Python in Java
---------------------------------

Suppose you are a Java developer maintaining a long-running enterprise
service. Product wants a "rules engine" so that analysts can tweak scoring
logic without waiting for a Java release train, and separately, the data
science team keeps handing you ``.pkl`` files and asking why the fraud model
they trained in scikit-learn can't just run inside the service. Rewriting
either of these in Java is possible but nobody wants to maintain it, and
shelling out to a Python subprocess for every request is a latency and
deployment headache you'd rather not own.

This is the mirror image of Cases 1-3: instead of Python reaching into Java,
Java reaches into Python. JPype's reverse bridge embeds a real CPython
interpreter inside your JVM process, so Python runs in the same address
space as your service, with no subprocess, no socket, and no serialization
between calls.

You start the interpreter once, near the top of your application's
lifecycle, alongside the JVM your service already runs in.

.. code-block:: java

   import org.jpype.MainInterpreter;
   import org.jpype.Script;
   import python.lang.PyObject;

   MainInterpreter interpreter = MainInterpreter.getInstance();
   if (!interpreter.isStarted())
       interpreter.start(new String[0]);

   Script rules = new Script(interpreter);
   rules.exec(
           "def score(transaction):\n" +
           "    risk = transaction['amount'] * 0.01\n" +
           "    if transaction['country'] != transaction['home_country']:\n" +
           "        risk += 5.0\n" +
           "    return risk\n");

The analysts' rule now lives in a ``.py`` file the service reloads on
change, rather than a Java class that needs a full build and deploy. Calling
it from Java looks like an ordinary method call, just routed through the
interpreter:

.. code-block:: java

   import python.lang.PyCallable;
   import python.lang.PyDict;

   PyDict transaction = rules.dict();
   transaction.putAny("amount", 4200.0);
   transaction.putAny("country", "RO");
   transaction.putAny("home_country", "US");

   PyCallable score = (PyCallable) rules.eval("score");
   PyObject risk = score.call(transaction);
   System.out.println(risk);  // 47.0

Loading the fraud model is the same story, just with ``joblib`` or
``pickle`` standing in for hand-written rules:

.. code-block:: java

   rules.exec(
           "import joblib\n" +
           "model = joblib.load('fraud_model.pkl')\n");

   PyObject prediction = rules.eval("model.predict([features])");

Because the interpreter lives inside the JVM's process, the transaction
dictionary above didn't need to be serialized to JSON, written to a socket,
or shipped to a sidecar process -- it's a real Python object your Java code
built directly through ``python.lang.PyDict``. The tradeoff, spelled out in
:doc:`limitations_java`, is that this interpreter can't be restarted and a
Python-side crash takes the JVM down with it -- the same coupling that gives
JPype's forward bridge its speed applies here too.

(This use case is explored in depth starting from :doc:`quickguide_java`.)


.. _introduction_the_jpype_philosophy:

The JPype Philosophy
=====================

JPype's design follows a handful of concrete principles:

1. **Make Java appear Pythonic** -- Java methods are mapped to Python
   methods, and Java collections are customized to behave like Python
   collections.

2. **Make Python appear like Java** -- Python classes can implement Java
   interfaces, and Java objects can be manipulated using Python's
   object-oriented features, so Java developers face a shallow learning
   curve.

3. **Expose all of Java to Python** -- threading, reflection, and other
   advanced APIs are reachable, not just a curated subset.

4. **Keep the design simple** -- for example, all Java array types
   originate from a single ``JArray`` factory rather than a family of
   type-specific constructors.

5. **Favor clarity over performance** -- JPype optimizes critical paths,
   but avoids premature optimization that would complicate the codebase
   for marginal gains elsewhere.

6. **Introduce familiar methods** -- new API surface follows existing
   conventions on both sides. Python's ``memoryview`` is used to access
   Java-backed memory; Java's ``Stream.of`` inspired ``JArray.of`` for
   converting NumPy arrays to Java arrays.

7. **Provide obvious solutions for both audiences** -- Python programmers
   can use list comprehensions with Java collections; Java programmers can
   use familiar methods like ``contains`` or ``hashCode``.

**Balancing two worlds**

Mixing Python's dynamic typing with Java's static typing requires mapping
concepts between the two languages:

- **Types** -- JPype provides type factories (``JClass``, ``JArray``) and a
  casting operator (``@``) to bridge Python's weak typing with Java's
  strict type declarations.
- **Inheritance** -- Java's single inheritance plus interfaces is mapped to
  Python classes via the ``@JImplements`` decorator.
- **Collections** -- Java's ``List``/``Map``/``Set`` are customized to
  behave like their Python counterparts.
- **Error handling** -- Java exceptions are mapped to Python exceptions, so
  a single ``try``/``except`` handles both.


.. _introduction_languages_other_than_java:

Languages Other Than Java
=========================

Although JPype is primarily designed to bridge Python with Java, its
capabilities extend in principle to other JVM-based languages such as
Kotlin, Scala, Groovy, and Clojure, since they compile to the same bytecode
JPype already reads. Each language introduces its own features and
paradigms that may need extra care when integrating with Python.


.. _introduction_supported_jvmbased_languages:

Supported JVM-Based Languages
-----------------------------

Because JPype operates at the JVM/JNI level rather than parsing Java source,
any class on the classpath is reachable the same way regardless of which
JVM language compiled it -- Kotlin, Scala, Groovy, Clojure, and others all
produce ordinary ``.class`` files. In practice this means:

- **Null safety, functional constructs, dynamic typing, Lisp-style
  syntax**, and other language-specific features exist at the source level
  in the original language; once compiled, they surface to JPype as
  whatever the bytecode actually declares (an ``Object``, a boxed type, an
  interface method), not as a JPype-specific concept.
- Each language's own standard library (Kotlin's collections, Scala's
  ``ArrayBuffer``, etc.) needs its runtime jar on the classpath, same as
  any other third-party Java dependency.
- None of these languages are part of JPype's own test suite -- this
  repository tests against plain Java only. Treat cross-language interop
  as "should work by the same mechanism as Java," not as a supported,
  verified configuration.

If you want to add real coverage for a specific JVM language, the natural
place is a dedicated test bench under JPype's test directory pulling in
that language's runtime jar, exercising its features, and documenting any
gotchas found along the way.


.. _introduction_alternatives:

Alternatives
============
JPype is not the only Python module of its kind that acts as a bridge to Java.
Depending on your programming requirements, one of the alternatives may be a
better fit. Specifically, JPype is designed for clarity and high levels of
integration between the Python and Java virtual machines. As such, it makes use
of JNI and inherits all the benefits and limitations that JNI imposes. With
JPype, both virtual machines run in the same process, sharing the same memory
space and threads. JPype can intermingle Python and Java threads and exchange
memory quickly. However, the JVM cannot be restarted within the same process,
and if Python crashes, Java will also terminate since they share the same
process.

Below is a comparison of JPype with other Python-to-Java bridging technologies.
These alternatives may suit different use cases depending on the level of
integration, performance requirements, or ease of use.

.. _introduction_py4j:

Py4J
---------------------------
`Py4J <https://py4j.org/>`_ is a Python library that enables communication
with a JVM through a remote tunnel. Unlike JPype, which embeds the JVM directly
into the Python process using JNI, Py4J operates the JVM as a separate process,
allowing Python and Java to run independently. This separation introduces
several unique advantages:

1. **Cross-Architecture Compatibility**: Py4J allows Python and Java to run on
different architectures or platforms. For example, you can run Python on a
64-bit architecture while connecting to a 32-bit JVM, or even run Python and
Java on entirely different machines. This flexibility is particularly useful
for distributed systems or environments where the Python and Java components
have different hardware or software requirements.

2. **Restartable Java Sessions**: Because Py4J operates the JVM as a separate
process, it is possible to stop and restart the JVM without restarting the
Python process. This is a feature frequently requested by JPype users but is
not feasible with JPype due to its use of JNI, which tightly couples the Python
and Java memory spaces. Py4J's ability to restart the JVM makes it suitable for
applications requiring dynamic lifecycle management of the Java environment.

3. **Memory Isolation**: Since Python and Java run in separate processes, Py4J
provides complete memory isolation between the two environments. This ensures
that a crash in the JVM does not affect the Python process and vice versa. Such
isolation can be critical for applications requiring high reliability and fault
tolerance.

4. **RPC-Style Communication**: Py4J operates more like a remote procedure call
(RPC) framework, where Python sends commands to the JVM and receives responses.
While this approach is less integrated than JPype's direct JNI-based
interaction, it is intended for applications where tight coupling between
Python and Java is not required.

Despite these advantages, Py4J has some limitations compared to JPype:

- **Performance**: The remote communication introduces a transfer penalty when
  moving data between Python and Java, making Py4J less suitable for
  applications requiring high-performance data exchange.

- **Integration**: Py4J does not provide the seamless integration of Java
  objects into Python syntax that JPype offers. For example, Java collections
  and arrays do not behave like native Python objects.

Py4J is a good choice for applications requiring cross-architecture
compatibility, restartable JVM sessions, or memory isolation between Python and
Java. However, for applications needing tight integration and high-performance
data exchange, JPype may be a better fit.


.. _introduction_jep:

Jep
-------
`Jep <https://github.com/ninia/jep>`_ stands for Java embedded Python. It is
designed to allow Java to access Python as a sub-interpreter. The syntax for
accessing Java resources from within the embedded Python is similar to JPype,
with support for imports.  However, Jep has limitations due to Python's
sub-interpreter model, which restricts the use of many Python modules.
Additionally, Jep's documentation is sparse, making it difficult to assess its
full capabilities without experimentation. Jep is best suited for applications
where Java needs to embed Python for scripting purposes.

.. _introduction_pyjnius:

PyJnius
-------
`PyJnius <https://github.com/kivy/pyjnius>`_ is another Python-to-Java bridge.
Its syntax is somewhat similar to JPype, allowing classes to be loaded and
accessed with Java-native syntax. PyJnius supports customization of Java
classes to make them appear more Pythonic. However, PyJnius lacks support for
primitive arrays, requiring Python lists to be converted manually whenever an
array is passed as an argument or return value. This limitation makes PyJnius
less suitable for scientific computing or applications requiring efficient
array manipulation. PyJnius is actively developed and is particularly focused
on Android development, making it a strong choice for mobile applications
requiring Python-Java integration.

.. _introduction_jython:

Jython
------
`Jython <https://www.jython.org/>`_ is a reimplementation of Python in Java. It
allows Python code to run directly on the JVM, providing seamless access to
Java libraries. Jython, while limited to Python 2, played a significant role in
bridging Python and Java in earlier development eras. It may still be useful for
legacy systems or environments where Python 2 compatibility is required. Its
development has largely stalled, and it lacks support for popular Python
libraries like NumPy and pandas, making it unsuitable for modern applications.

.. _introduction_javabridge:

Javabridge
-----------
`Javabridge <https://github.com/CellProfiler/python-javabridge/>`_  provides
direct low-level JNI control from Python. Its integration
level is low, offering only the JNI API to Python rather than attempting to
wrap Java in a Python-friendly interface. While Javabridge can be useful for
advanced users familiar with JNI, it requires significant expertise to use
effectively. Javabridge is best suited for applications needing fine-grained
control over JNI interactions.

.. _introduction_jcc:

JCC
---
`JCC <https://lucene.apache.org/pylucene/jcc/>`_ is a C++ code generator that
produces a C++ object interface wrapping a Java library via JNI. JCC also
generates C++ wrappers conforming to Python's C type system, making instances
of Java classes directly available to a Python interpreter. JCC is actively
maintained as part of PyLucene and is useful for exposing specific Java
libraries to Python rather than providing general Java access. It is best
suited for applications requiring tight integration with libraries like
Apache Lucene.

.. _introduction_about_this_guide:

About this Guide
================

The JPype User Guide is designed for two primary audiences:

1. **Python Programmers**: Those who are proficient in Python and wish to
use Java libraries or integrate Java functionality into their Python
projects.

2. **Java Programmers**: Those who are experienced in Java and want
to use Python as a development tool for Java, particularly for tasks like
visualization, debugging, or scripting.


This guide aims to bridge the gap between these two languages by comparing and
contrasting their differences, providing examples that illustrate how to
translate concepts from one language to the other. It assumes that readers are
proficient in at least one of the two languages. If you lack a strong
background in either Python or Java, you may need to consult tutorials or
introductory materials for the respective language before proceeding.

This chapter, like the rest of the User Guide, is written from the
Python-calling-Java direction (Cases 1-3 above). JPype also supports the
reverse direction -- a Java application embedding Python, as in Case 4 -- but
that direction has its own paired set of chapters (:doc:`quickguide_java`,
:doc:`jvm_java`, :doc:`types_java`, :doc:`limitations_java`, and others named
with a ``_java`` suffix throughout this documentation) rather than being
folded into the material below.

Key Features of the Guide
-------------------------

- **No JNI Knowledge Required**: JPype abstracts away the complexities of the
  Java Native Interface (JNI). Users do not need to understand JNI concepts or
  its naming conventions to use JPype effectively. In fact, relying on JNI
  knowledge may lead to incorrect assumptions about the JPype API. Where JNI
  imposes limitations, the guide explains the consequences in practical
  programming terms.

- **Python 3 Compatibility**: JPype supports only Python 3. All examples in
  this guide use Python 3 syntax and assume familiarity with Python's new-style
  object model. If you're using an older version of Python, you will need to
  upgrade to Python 3 to use JPype.

- **Java Naming Conventions**: JPype adheres to Java's naming conventions for
  methods and fields to ensure consistency and avoid potential name collisions.
  While this may differ from Python's conventions, it is a deliberate choice to
  maintain compatibility with Java libraries and APIs.

By following this guide, you'll learn how to use JPype to combine Python and
Java in your own projects.


.. _introduction_getting_jpype_started:

Getting JPype started
---------------------

This document holds numerous JPype examples.  For the purposes of clarity
the module is assumed to have been started with the following command

.. code-block:: python

  # Import the module
  import jpype

  # Allow Java modules to be imported
  import jpype.imports

  # Import all standard Java types into the global scope
  from jpype.types import *

  # Import each of the decorators into the global scope
  from jpype import JImplements, JOverride, JImplementationFor

  # Start JVM with Java types on return
  jpype.startJVM()

  # Import default Java packages
  import java.lang
  import java.util

This is not the only style used by JPype users.  Some people feel it is
best to limit the number for symbols in the global scope and instead
start with a minimalistic approach.

.. code-block:: python

  import jpype as jp                 # Import the module
  jp.startJVM()                      # Start the module

Either style is usable and we do not wish to force any particular style on the
user.  But as the extra ``jp.`` tends to just clutter up the space and implies
that JPype should always be used as a namespace due to namespace conflicts, we
have favored the global import style.  JPype only exposes 40 symbols total
including a few deprecated functions and classes. The 13 most commonly used
Java types are wrapped in a special module ``jpype.types`` which can be used to
import all for the needed factories and types with a single command without
worrying about importing potentially problematic symbols.

We will detail the starting process more later in the guide.  See
:ref:`Starting the JVM <startJVM>` (in :doc:`jvm_py`).


.. _introduction_jpype_concepts:

JPype Concepts
==============

At its heart, JPype is about providing a bridge to use Java within Python.
Depending on your perspective, this can either be a means of accessing Java
libraries from within Python or a way to use Java with Python syntax for
interactivity and visualization. JPype aims to provide access to the entirety
of the Java language from Python, mapping Java concepts to their closest Python
equivalents wherever possible.

Python and Java share many common concepts, such as types, classes, objects,
functions, methods, and members. However, there are significant differences
between the two languages. For example, Python lacks features like casting,
type declarations, and method overloading, which are central to Java's strongly
typed paradigm. JPype introduces these concepts into Python syntax while
striving to maintain Pythonic usability.

This section breaks down JPype's core concepts into nine distinct categories.
These categories define how Java elements are mapped into Python and how they
can be used effectively.


.. _introduction_core_concepts:

Core Concepts
-------------

1. **Type Factories**:

   - Type factories allow you to declare specific Java types in Python. These
     factories produce wrapper classes for Java types.

   - Examples include `JClass` for Java classes and `JArray` for Java arrays.

   - Factories also exist for implementing Java classes from within Python
     using proxies (e.g., `JProxy`).

2. **Meta Classes**:

   - Meta classes describe properties of Java classes, such as whether a class
     is an interface.

   - Example: `JInterface` can be used to check if a Java class is an interface.

3. **Base Classes**:

   - JPype provides base classes for common Java types, such as `Object`,
     `String`, and `Exception`.

   - These classes can be used for convenience, such as catching all Java
     exceptions with `JException`.

   - Example: `java.lang.Throwable` can be caught using `JException`.

4. **Wrapper Classes**:

   - Wrapper classes correspond to individual Java classes and are dynamically
     created by JPype. These wrappers encapsulate Java objects and provide a
     Pythonic interface for interacting with them. Depending on the context,
     a wrapper may contain a Java reference, such as a class instance,
     primitive array, or boxed type, or a Java proxy that dynamically
     implements a Java interface.

   - They allow access to static variables, static methods, constructors, and
     casting.

   - Example: `java.lang.Object`, `java.lang.String`.

5. **Object Instances**:

   - These are Java objects created or accessed within Python. They behave like
     Python objects, with Java fields mapped to Python attributes and Java
     methods mapped to Python methods.

   - Example: A Java `String` object can be accessed and manipulated like a
     Python string.

6. **Primitive Types**:

   - JPype maps Java's primitive types (e.g., `boolean`, `int`, `float`) into
     Python classes.

   - Example: `JInt`, `JFloat`, `JBoolean`.

7. **Decorators**:

   - JPype provides decorators to augment Python classes and methods with
     Java-specific functionality.

   - Examples include `@JImplements` for implementing Java interfaces and
     `@JOverride` for overriding Java methods.

8. **Mapping Java Syntax to Python**:

   - JPype maps Java syntax to Python wherever possible. For example:

     - Java's `try`, `throw`, and `catch` are mapped to Python's `try`, `raise`,
       and `except`.

     - Java's `synchronized` keyword is mapped to Python's `with` statement
       using `jpype.synchronized`.

9. **JVM Control Functions**:

   - JPype provides functions for controlling the JVM, such as starting and
     shutting it down.

   - Examples: `jpype.startJVM()` and `jpype.shutdownJVM()`.


.. _introduction_additional_details:

Additional Details
------------------

- **Name Mangling**:

  - JPype handles naming conflicts between Java and Python by appending an
    underscore (`_`) to conflicting names.

  - Example: A Java method named `with` will appear as `with_` in Python.

  - For details see :ref:`Name mangling <name_mangling>` (in :doc:`types_py`).

- **Lifetime Management**:

  - Java objects remain alive as long as their corresponding Python handles
    exist. Once the Python handle is disposed, the Java object is eligible for
    garbage collection.

Understanding these core concepts is enough to use JPype effectively for
most integration work.


.. _introduction_best_practices:

Best Practices on JVM Startup
-----------------------------

1. **Start the JVM early**, before any code that needs Java classes runs.
   Delayed or conditional ``startJVM()`` calls are a common source of
   confusing "class not found" errors from code that assumed the JVM was
   already up.

2. **Configure the classpath explicitly**, via the ``classpath`` argument
   to ``startJVM()`` or ``addClassPath()`` beforehand, rather than relying
   on defaults -- this avoids ambiguity about which JAR versions get
   loaded.

3. **Disable automatic string conversion** (``convertStrings=False``) for
   large-scale data transfers or performance-sensitive code. Automatic
   conversion is a legacy default kept for backward compatibility, not a
   recommendation.

4. **The JVM cannot be restarted** once shut down in the same process.
   Design applications to start it once and keep it running for the
   program's lifetime.

.. _optimize_data_transfers:

5. **Optimize data transfers** between Python and Java, since frequent
   back-and-forth calls create bottlenecks in data-heavy code:

   - NumPy arrays map directly to Java primitive arrays without copying.
   - Java's ``nio`` buffers give shared memory for large datasets or
     memory-mapped files, avoiding repeated conversions.
   - Cache Java objects used repeatedly from Python rather than
     re-resolving them each call.
   - Arrays and collections being transferred must be rectangular and
     type-compatible -- jagged arrays or mismatched types fail or degrade
     performance.

6. **Catch Java exceptions explicitly** with ``jpype.JException`` or a
   specific exception class; ``stacktrace()`` gives the Java-side detail
   when debugging.

7. **Document classpath and startup configuration** in the codebase --
   it's the first thing another developer needs to reproduce your setup.

