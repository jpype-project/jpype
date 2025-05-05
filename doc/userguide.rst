##################
JPype User Guide
##################

.. toctree::
   :maxdepth: 2

.. _introduction:

Introduction
************

.. _introduction_jpype_the_python_to_java_bridge:

JPype the Python to Java bridge
===============================
JPype is a Python module that provides seamless
access to Java libraries from Python. Unlike Jython, which reimplements Python
on the Java Virtual Machine (JVM), JPype bridges Python and Java at the native
level using the Java Native Interface (JNI). This native approach implements CPython
classes for each Java type and Java support for type management while communicating
at the process level.  This approach enables:

- Direct interaction between Python and Java objects.

- Access to the full range of Java libraries and APIs.

- No need to serialize objects when communicating between language.

- Unified primitive types.

- High speed transfers through shared memory between Python and Java 
  for large primitive array types.

JPype is intended for Python developers who need to leverage Java libraries or
Java developers who want to use Python for scripting, debugging, or
visualization.

.. _introduction_why_use_jpype?:

Why Use JPype?
--------------
JPype makes it easy to integrate Python and Java, enabling developers to:

1. Access Java libraries directly from Python code.
2. Debug Java data structures interactively using Python tools.
3. Use Python's flexibility for scientific computing while leveraging Java's
   robustness for enterprise applications.

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
Follow these steps to write and run your first JPype program:


.. _introduction_step_1_start_the_jvm:

Step 1: Start the JVM
~~~~~~~~~~~~~~~~~~~~~
JPype requires the JVM to be started before interacting with Java. All calls
to the Java prior to the start of the JVM will fail. Use the
`jpype.startJVM()` function to start the JVM::

    import jpype

    # Start the JVM
    jpype.startJVM(classpath=[])

.. _introduction_step_2_access_java_classes:

Step 2: Access Java Classes
~~~~~~~~~~~~~~~~~~~~~~~~~~~
JPype allows you to import and use Java classes directly. For example, import
Java's `java.lang.String` class::

    from java.lang import String

.. _introduction_step_3_use_java_objects:

Step 3: Use Java Objects
~~~~~~~~~~~~~~~~~~~~~~~~
Create and manipulate Java objects just like Python objects::

    java_string = String("Hello from Java!")
    print(java_string.toUpperCase())  # Output: HELLO FROM JAVA!

.. _introduction_step_4_shut_down_the_jvm:

Step 4: Shut Down the JVM
~~~~~~~~~~~~~~~~~~~~~~~~~
Once the program is complete the JVM will exit when Python does.  All termination
code is handled automatically.


.. _introduction_complete_example:

Complete Example
~~~~~~~~~~~~~~~~
Save the following code in a file named `hello_jpype.py`:

.. code-block:: python

    import jpype
    import jpype.imports

    # Start the JVM
    jpype.startJVM(classpath=["../jar/*","../classes","com.amce-1.0.jar"])

    # Import Java classes
    from java.lang import String

    # Use the Java String class
    java_string = String("Hello from Java!")
    print(java_string.toUpperCase())  # Output: HELLO FROM JAVA!


Run the script using Python::

    python hello_jpype.py

You should see the output::

    HELLO FROM JAVA!


.. _introduction_next_steps:

Next Steps
----------
Once you've successfully set up JPype, explore the following topics:

1. **Accessing Java Libraries**: Learn how to use JPype to interact with third-
   party Java libraries.
2. **Working with Java Collections**: Discover how JPype integrates Java
   collections with Python's `collections` module.
3. **Implementing Java Interfaces in Python**: Use JPype's proxy functionality
   to implement Java interfaces in Python.
4. **Debugging Java Code**: Use JPype as an interactive shell for debugging
   Java programs.


.. _introduction_summary_of_jpype:

Summary of JPype
----------------
JPype bridges Python and Java, enabling seamless integration between the two
languages. With JPype, you can access Java libraries, implement Java
interfaces, and debug Java code—all from the comfort of Python. Happy coding!



.. _introduction_jpype_use_cases:

JPype Use Cases
===============

Here are three typical reasons to use JPype.

- Access to a Java library from a Python program (Python oriented)
- Visualization of Java data structures via Matplotlib (Java oriented)
- Interactive Java and Python development including scientific and mathematical
  programming using Python as a Java shell with Spyder or Jupyter notebooks.

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

  import com.paying.customer.DataBase

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
  plt.plot(readings[:,0], readings[:,1)
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

(This advanced demonstration utilized the concept of Proxies_ and
`Code completion`_)


.. _introduction_the_jpype_philosophy:

The JPype Philosophy
=====================

JPype is designed to provide seamless integration between Python and Java,
allowing developers to use Java libraries and features as naturally as possible
within Python. To achieve this, JPype adheres to several core design
principles:

1. **Make Java appear Pythonic**:

   - JPype strives to make Java concepts feel familiar to Python programmers.
     This involves adapting Java syntax and behaviors to align with Python's
     conventions wherever possible.
   - For example, Java methods are mapped to Python methods, and Java
     collections are customized to behave like Python collections.

2. **Make Python appear like Java**:

   - JPype ensures that Java developers can work with Python without a steep
     learning curve. This includes presenting Python constructs in a way that
     resembles Java syntax and behavior.
   - For instance, Python classes can implement Java interfaces, and Java
     objects can be manipulated using Python's object-oriented features.

3. **Expose all of Java to Python**:

   - JPype aims to provide access to the entirety of the Java ecosystem,
     including libraries, packages, and features. The goal is to act as a
     bridge, enabling unrestricted interaction between the two languages.
   - Whether it's Java threading, reflection, or advanced APIs, JPype ensures
     that Python developers can leverage Java's full capabilities.

4. **Keep the design simple**:

   - Mixing two languages is inherently complex, so JPype minimizes additional
     complexity by maintaining a simple and consistent design.
   - For example, all Java array types originate from the `JArray` factory,
     ensuring a unified approach to handling arrays.

5. **Favor clarity over performance**:

   - While JPype optimizes critical paths for performance, clarity is
     prioritized to ensure long-term maintainability and usability.
   - For example, JPype avoids premature optimization that could complicate the
     codebase or introduce unnecessary constraints.

6. **Introduce familiar methods**:

   - When new methods are added, JPype ensures they align with established
     conventions in both Python and Java.
   - For example, Python's ``memoryview`` is used to access Java-backed memory,
     while Java's ``Stream.of`` inspired the ``JArray.of`` method for converting
     NumPy arrays to Java arrays.

7. **Provide obvious solutions for both Python and Java programmers**:

   - JPype recognizes that "obviousness" varies between Python and Java
     developers. Therefore, it provides solutions that feel natural to both
     audiences.

   - For example, Python programmers can use list comprehensions with Java
     collections, while Java programmers can use familiar methods like
     ``contains`` or ``hashCode``.


**Balancing Two Worlds**

JPype bridges two distinct programming paradigms: Python's dynamic and flexible
nature versus Java's strongly-typed and structured approach. This balance
requires careful mapping of concepts between the two languages:

- **Types**:

  - Python's weak typing allows variables to change types dynamically, while
    Java's strong typing enforces strict type declarations. JPype accommodates
    this difference by providing type factories (``JClass``, ``JArray``) and casting
    operators (``@``).

- **Inheritance**:

  - Java supports single inheritance with interfaces, while Python allows
    multiple inheritance. JPype maps Java interfaces to Python classes using
    decorators (``@JImplements``) to ensure compatibility.

- **Collections**:

  - Java collections (``List``, ``Map``, ``Set``) are customized to behave like
    Python collections, enabling intuitive interaction for Python developers.

- **Error Handling**:

  - Java exceptions are mapped to Python exceptions, allowing developers to
    handle errors seamlessly across both languages.


**Philosophy in Practice**

JPype's design philosophy ensures a small footprint while offering high levels
of integration between Python and Java. Developers can use JPype to:

- Access Java libraries for tasks that Python lacks native support for (e.g.,
  advanced threading, enterprise APIs).
- Use Python's interactive and visualization capabilities to debug or analyze
  Java data structures.
- Combine Python's flexibility with Java's robustness for scientific computing,
  machine learning, and enterprise applications.

By adhering to these principles, JPype provides a powerful yet accessible tool
for bridging the Python and Java ecosystems.


.. _introduction_languages_other_than_java:

Languages Other Than Java
=========================

Although JPype is primarily designed to bridge Python with Java, its
capabilities extend to other JVM-based languages such as Kotlin, Scala, Groovy,
and Clojure. These languages share the same underlying Java Virtual Machine
(JVM) infrastructure, allowing JPype to interact with them seamlessly. However,
each language introduces unique features and paradigms that may require
additional considerations when integrating with Python.


.. _introduction_supported_jvmbased_languages:

Supported JVM-Based Languages
-----------------------------

1. **Kotlin**:

   - Kotlin is a modern JVM-based language that emphasizes conciseness and
     safety. JPype can interact with Kotlin libraries and classes just as it
     does with Java.

   - Kotlin's null safety and extension functions are fully compatible with
     JPype, though developers may need to handle Kotlin's nullable types
     explicitly when working in Python.

   - Example: Using Kotlin's ``List`` class in Python via JPype.

.. code-block:: python

      from kotlin.collections import List
      my_list = List.of("apple", "orange", "banana")
      print(my_list.size())  # Access Kotlin methods


2. **Scala**:

   - Scala combines object-oriented and functional programming paradigms,
     making it a popular choice for big data and distributed systems.

   - JPype can interact with Scala libraries, including those built on
     frameworks like Akka or Spark.

   - Scala's collections and functional constructs (e.g., `map`, `flatMap`) can
     be accessed directly from Python, though some functional idioms may
     require adaptation.

.. code-block:: python

      from scala.collection.mutable import ArrayBuffer
      buffer = ArrayBuffer()
      buffer.append(1)
      buffer.append(2)
      print(buffer.mkString(", "))  # Outputs: "1, 2"


3. **Groovy**:

   - Groovy is a dynamic language for the JVM, often used for scripting and
     lightweight application development.

   - JPype can interact with Groovy scripts and libraries, enabling Python
     developers to leverage Groovy's concise syntax and dynamic capabilities.

   - Groovy's dynamic typing aligns well with Python, but accessing from
     within JPype may cause difficulties.

.. code-block:: python

      from groovy.util import Eval
      result = Eval.me("3 + 5")
      print(result)  # Outputs: 8


4. **Clojure**:

   - Clojure is a functional programming language that runs on the JVM. Its
     emphasis on immutability and concurrency makes it ideal for certain types
     of applications.

   - JPype can interact with Clojure libraries, though developers may need to
     adapt to Clojure's Lisp-like syntax and functional paradigms.

.. code-block:: python

      from clojure.lang import PersistentVector
      vector = PersistentVector.create([1, 2, 3])
      print(vector.nth(1))  # Access elements using Clojure methods


.. _introduction_using_jpype_with_other_jvm_languages:

Using JPype with Other JVM Languages
------------------------------------

JPype can be used with JVM-based languages, but the following considerations
apply:

1. **Language-Specific Features**:

   - Each language introduces unique features (e.g., Kotlin's null safety,
     Scala's functional constructs, Groovy's dynamic typing). These may require
     adaptation when working with Python.

2. **Interoperability**:

   - JPype relies on the JVM's native interoperability mechanisms, ensuring
     seamless interaction with JVM-based languages. However, developers should
     be aware of differences in naming conventions, type systems, and runtime
     behavior.

3. **Testing and Integration**:

   - To fully support a JVM-based language, developers should set up a test
     bench to exercise its features, write language-specific quick-start
     guides, and ensure compatibility with JPype's existing API.


.. _introduction_expanding_jpype_for_other_jvm_languages:

Expanding JPype for Other JVM Languages
---------------------------------------

If you wish to extend JPype's capabilities for a specific JVM-based language,
the following steps are recommended:

1. **Create a Test Bench**:

   - Set up a test environment for your language under JPype's test directory.
     Use Ivy or Maven to pull in the required JAR files and exercise the
     language's unique features.

2. **Write a Language-Specific Guide**:

   - Document how your language interacts with JPype, highlighting differences
     from Java and providing examples for common use cases.

3. **Set Up a Test Harness**:

   - Build a test harness to verify compatibility for each language feature.
     Place the setup script (e.g., `test_kotlin`, `test_scala`) alongside
     JPype's existing tests.

.. _introduction_conclusion_on_languages:

Conclusion on Languages
-----------------------

JPype's ability to interact with JVM-based languages opens up exciting
possibilities for Python developers. Whether you're working with Kotlin's
modern syntax, Scala's functional paradigms, Groovy's dynamic scripting, or
Clojure's immutability, JPype provides a powerful bridge to leverage the
strengths of these languages within Python. By following the steps outlined
above, you can ensure smooth integration and expand JPype's capabilities for
your specific needs.


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
PyJnius <https://github.com/kivy/pyjnius>_ is another Python-to-Java bridge.
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
Jython <https://www.jython.org/>_ is a reimplementation of Python in Java. It
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
suited for applications requiring tight integration with libraries like Apache
Lucene.  It is best suited for applications requiring tight integration with
specific Java libraries.

.. _introduction_about_this_guide:

About this Guide
================

The JPype User Guide is designed for two primary audiences:

1. **Python Programmers**: Those who are proficient in Python and wish to
leverage Java libraries or integrate Java functionality into their Python
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

By following this guide, you’ll learn how to use JPype to seamlessly integrate
Python and Java, unlocking the strengths of both languages in your projects.


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
`Starting the JVM`_.


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
     created by JPype. These wrappers encapsulate Java objects and provide a Pythonic
     interface for interacting with them. Depending on the context, a wrapper may 
     contain a Java reference, such as a class instance, primitive array, or boxed type
     or a Java proxy which implements dynamically implements a Java interface.

   - Wrappers are designed to make Java objects behave like native Python objects, 
     enabling seamless integration between Python and Java. These wrappers provide
     a Pythonic interface to Java objects, making them behave like native Python
     objects while retaining their Java functionality.

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

  - For details see `Name Mangling`_.

- **Lifetime Management**:

  - Java objects remain alive as long as their corresponding Python handles
    exist. Once the Python handle is disposed, the Java object is eligible for
    garbage collection.

By understanding these core concepts, you can effectively use JPype to
integrate Python and Java, leveraging the strengths of both languages.


.. _introduction_best_practices:

Best Practices on JVM Startup
-----------------------------

Starting the Java Virtual Machine (JVM) correctly is critical for ensuring the
smooth operation of JPype-based applications. A well-configured JVM startup
process minimizes runtime issues, optimizes performance, and ensures
compatibility with the required Java libraries. This section provides a
detailed explanation of best practices to guide developers in setting up the
JVM effectively.

1. Start the JVM Early
   The JVM should always be started early in the application lifecycle. By
   initializing the JVM at the beginning of your program, you can avoid issues
   related to delayed imports or incomplete initialization. This approach
   ensures that all Java classes and libraries required by your application
   are properly loaded and accessible throughout the program's execution.

2. Configure the Classpath Explicitly
   Classpath configuration is another essential consideration. The
   ``classpath`` specifies the location of Java classes and JAR files that the
   JVM needs to load. For optimal results, explicitly define the ``classpath``
   when starting the JVM. This can be done using the ``classpath`` argument in
   the ``startJVM()`` function or dynamically through the ``addClassPath()``
   method prior to JVM startup. Explicit configuration prevents errors caused
   by missing dependencies and ensures that the correct versions of libraries
   are loaded.

3. Disable Automatic String Conversion
   When dealing with large-scale data transfers or computationally intensive
   operations, it is advisable to disable automatic string conversion by
   setting the ``convertStrings`` argument to ``False``. This prevents
   unnecessary overhead caused by automatic conversion of Java strings to
   Python strings, allowing developers to retain control over string handling
   and improve performance. While enabling automatic string conversion may
   seem convenient, it is considered a legacy option and should be avoided in
   modern applications.

4. Avoid Restarting the JVM
   It is important to note that the JVM cannot be restarted once it has been
   shut down. Therefore, design your application to start the JVM once and
   keep it running for the program's lifetime. Attempting to restart the JVM
   will result in errors due to lingering references and resource conflicts.
   This limitation underscores the importance of careful planning when
   initializing the JVM.

.. _optimize_data_transfers:

5. Optimize Data Transfers
   When Python and Java need to exchange large amounts of data, such as arrays
   or complex structures, the efficiency of these transfers can significantly
   impact application performance. Without optimization, frequent back-and-
   forth calls between Python and Java can create bottlenecks, especially in
   computationally intensive applications like scientific computing or machine
   learning.

   To ensure smooth data exchange, consider the following strategies:

   1. **Use NumPy Arrays**: NumPy arrays integrate seamlessly with JPype and
      allow fast, memory-efficient data transfers to Java. For example, a
      NumPy array can be mapped directly to a Java primitive array, enabling
      high-speed operations without unnecessary copying.

   2. **Leverage Java Buffers**: Java's `nio` buffers provide a mechanism for
      shared memory between Python and Java. These buffers are particularly
      useful for large datasets or memory-mapped files, as they eliminate the
      overhead of repeated conversions and allow both languages to operate on
      the same memory space.

   3. **Cache Java Objects**: If a Java object is used repeatedly in Python,
      consider caching it to reduce the frequency of cross-language calls.
      This avoids redundant conversions and improves overall runtime
      efficiency.

   4. **Validate Data Structures**: Ensure that arrays or collections being
      transferred are rectangular and compatible with the expected Java types.
      For example, jagged arrays or incompatible data types can lead to errors
      or performance degradation.

   By implementing these strategies, you can optimize the interaction between
   Python and Java, ensuring that your application performs efficiently even
   when handling large-scale data or computationally intensive tasks.

6. Handle Exceptions Properly
   Exception handling is another key aspect of JVM startup. Always catch Java
   exceptions using ``jpype.JException`` or specific Java exception classes to
   ensure robust error handling. When debugging issues, the ``stacktrace()``
   method can provide detailed information about Java exceptions, helping
   developers identify and resolve problems effectively.

7. Document Your Setup
   Finally, document the JVM startup process and configuration settings
   clearly within your codebase. This practice not only aids in debugging but
   also ensures that other developers working on the project can understand
   and replicate the setup. By adhering to these best practices, you can
   maximize the reliability, performance, and maintainability of your
   JPype-based applications.

By adhering to these best practices, you can maximize the performance,
reliability, and maintainability of your JPype-based applications.


.. _jpype_types:

JPype Types
***********

Both Java and Python have a concept of a type.  Every variable refers to an
object which has a defined type.  A type defines the data that the variable is
currently holding and how that variable can be used.  In this chapter we will
learn how Java and Python types relate to one another, how to create import
types from Java, and how to use types to create Java objects.


.. _jpype_types_stay_strong_in_a_weak_language:

Stay strong in a weak language
==============================

Before we get into the details of the types that JPype provides, we first need
to contrast some of the fundamental language differences between Java and
Python.  Python is inherently a weakly typed language.  Any variable can take
any type and the type of a particular variable can change over the
lifetime of a program.  Types themselves can be mutable as you can patch an
existing type to add new behaviors.  Python methods can in principle take any
type of object as an argument, however if the interface is limited it will produce
a TypeError to indicate a particular argument requires a specific type.  Python
objects and classes are open.  Each class and object is basically a dictionary
storing a set of key-value pairs.  Types implemented in native C are often more
closed and thus can't have their method dictionaries or data members altered
arbitrarily.  But subject to a few restrictions based implementation, it is
pretty much the wild west.

In contrast, Java is a strongly typed language.  Each variable can only take
a value of the specified class or a class that derives from the specified
class.  Each Java method takes only a specific number and type of arguments.
The type and number are all checked at compile type to ensure there is
little possibility of error.  As each method requires a specific number and type
of arguments, a method can be overloaded by having two different
implementations which take a different list of types sharing the same method
name. A primitive variable can never hold an object and it can only be converted
to or from other primitive types unless it is specifically cast to that type.
Java objects and classes are completely closed.  The methods and fields for a
particular class and object are defined entirely at compile time.  Though it is
possible create classes with a dictionary allowing expansion, this is not the
Java norm and no standard mechanism exists.

Thus we need to introduce a few Java terms to the Python vocabulary.  These are
"conversion" and "cast".


.. _jpype_types_java_conversions:

Java conversions
----------------

A conversion is a permitted change from an object of one type to another.
Conversions have three different degrees.  These are: exact, derived, implicit,
and explicit.

Exact conversions are those in which the type of an object is identical.  In
Java each class has only one definition thus there is no need for an exact
conversion.  But when dealing with Python we have objects that are effectively
identical for which exact conversion rules apply.  For example, a Java string
and a Python string both bind equally well to a method which requires a string,
thus this is an exact conversion for the purposes of bind types.

The next level of conversion is derived.  A derived class is one which is a
descends from a required type.  It is better that implicit but worse than
exact.  If all of the types in a method match are exact or derived then it will
override a method in which one argument is implicit.

The next level of conversion is implicit.  An implicit conversion is one that
Java would perform automatically.  Java defines a number of other conversions
such as converting a primitive to a boxed type or from a boxed type back to a
primitive as implicit conversions.  Python conversions defined by the user are
also considered to be implicit.

Of course not every cast is safe to perform.  For example, converting an object
whose type is currently viewed as a base type to a derived type is not
performed automatically nor is converting from one boxed type to another.  For
those operations the conversion must be explicitly requested, hence these are
explicit conversions.   In Java, a cast is requested by placing the type name
in parentheses in front of the object to be cast.  Python does not directly
support Java casting syntax. To request an explicit conversion an object must
be "cast" using a cast operator @.   Overloaded methods with an explicit
argument will not be matched.  After applying an explicit cast, the match
quality can improve to exact or derived depending on the cast type.

Not every conversion is possible between Java types.  Types that cannot be
converted are considerer to be conversion type "none".

Details on how method overloads are resolved are given in `Method Resolution`_.
Details on the standard conversions provided by JPype are given in the section
`Type Matching`_.

.. _cast:

Java casting
------------

To access a casting operation we use the casting ``JObject`` wrapper.
For example, ``JObject(object, Type)`` would produce a copy with specificed type.
The first argument is the object to convert and
the second is the type to cast to.  The second argument should always be a Java
type specified using a class wrapper, a Java class instance, or a string.
Casting will also add a hidden class argument to the resulting object such that
it is treated as the cast type for the duration of that variable lifespan.
Therefore, a variable create by casting is stuck as that type and cannot revert
back to its original for the purposes of method resolution.

The object construction and casting are sometimes a bit blurry.  For example,
when one casts a sequence to a Java list, we will end up constructing a new
Java list that contains the elements of the original Python sequence.  In
general JPype constructors only provide access the Java constructor methods
that are defined in the Java documentation.  Casting on the other hand is
entirely the domain of whatever JPype has defined including user defined casts.

As ``JObject`` syntax is long and does not look much like Java syntax, the
Python matmul operator is overloaded on JPype types such that one can use the
``@`` operator to cast to a specific Java type.   In Java, one would write
``(Type)object`` to cast the variable ``object`` to ``Type``.  In Python, this
would be written as ``Type@object``.   This can also be applied to array types
``JLong[:]@[1,2,3]``, collection types ``Iterable@[1,2,3]`` or Java functors
``DoubleUnaryOperator@(lambda x:x*2)``.  The result of the casting operator
will be a Java object with the desired type or raise a ``TypeError`` if the
cast or conversion is not possible.   For Python objects, the Java object will
generally be a copy as it is not possible to reflect changes in an array back
to Python.  If one needs to retrieve the resulting changes keep a copy of the
converted array before passing it.  For an existing Java object, casting
changes the resolution type for the object.  This can be very useful when
trying to call a specific method overload.   For example, if we have a Java
``a=String("hello")`` and there were an overload of the method ``foo`` between
``String`` and ``Object`` we would need to select the overload with
``foo(java.lang.Object@a)``.

.. _JObject:

Casting is performed through the Python class ``JObject``.  JObject is called
with two arguments which are the object to be cast and the type to cast too.
The cast first consults the conversion table to decide if the cast it permitted
and produces a ``TypeError`` if the conversion is not possible.

``JObject`` also serves as a abstract base class for testing if an object
instance belongs to Java.  All objects that belong to Java will return
true when tested with ``isinstance``.  Like Python's sequence, JObject is an
abstract base class.  No classes actual derive from ``JObject``.

.. _null:

Of particular interest is the concept of Java ``null``.  In Java, null is a
typeless entity which can be placed wherever an object is taken to
indicate that the object is not available.  The equivalent concept in Python is
``None``.  Thus all methods that accept any object type that permit a null will
accept None as an augment with implicit conversion.  However, sometime it is
necessary to pass an explicit type to the method resolution.  To achieve this
in JPype use ``Type@None`` which will create a null pointer with the
desired type.  To test if something is null we have to compare the handle to
None.  This unfortunately trips up some code quality checkers.  The idiom in
Python is ``obj is None``, but as this only matches things that Python
considers identical, we must instead use ``obj==None``.

Casting ``None`` is use to specify types when calling between overloads
with variadic arguments such as ``foo(Object a)`` and ``foo(Object... many)``.
If we want to call ``foo(None)`` is is ambiguous whether we intend to call the
first with a null object or the second with a null array.  We can resolve the
ambiguity with ``foo(java.lang.Object@None)`` or
``foo(java.lang.Object[:]@None)``

Type enforcement appears in three different places within JPype.  These are
whenever a Java method is called, whenever a Java field is set, and whenever
Python returns a value back to Java.


Primitive Types
===============

Unlike Python, Java makes a distinction between objects and primitive data types.
Primitives represent the minimum data that can be manipulated by a computer. These
stand in contrast to objects, which have the ability to contain any combination of
data types and objects within themselves, and can be inherited from.

Java primitives come in three categories:

- **Logical**: `boolean` (true/false values).
- **Textual**: `char` (single Unicode character).
- **Numerical**: Fixed-point or floating-point numbers of varying sizes.

JPype maps Java primitives to Python classes. To avoid naming conflicts with
Python, JPype prefixes each primitive type with `J` (e.g., `JBoolean`, `JInt`).

.. _jpype_types_primitive_types_jboolean:

JBoolean
--------

Represents a logical value (`True` or `False`). In JPype, `True` and `False` are
exact matches for `JBoolean`. Methods returning a `JBoolean` will always return a
Python `bool`.

.. code-block:: python

   # Example usage
   java_boolean = JBoolean(True)
   print(java_boolean)  # Output: True

.. _jpype_types_primitive_types_jchar:

JChar
-----

Represents a single character. Java `char` types are 16-bit Unicode characters,
but some Unicode characters require more than 16 bits. JPype maps `JChar` to
Python strings of length 1. While `JChar` supports numerical operations, modifying
characters numerically can corrupt their encoding.

.. code-block:: python

   # Example usage
   java_char = JChar('A')
   print(java_char)  # Output: 'A'

.. _jpype_types_primitive_types_jbyte,_jshort,_jint,_jlong:

JByte, JShort, JInt, JLong
--------------------------

These types represent signed integers of varying sizes:

- **JByte**: 8 bits
- **JShort**: 16 bits
- **JInt**: 32 bits
- **JLong**: 64 bits

JPype maps these types to Python's `int`. Methods returning integer primitives will
return Python `int` values. Methods accepting integer primitives will accept Python
integers or any object that can be converted into the appropriate range.

.. code-block:: python

   # Example usage
   java_int = JInt(42)
   print(java_int)  # Output: 42


.. _jpype_types_jfloat_jdouble:

JFloat, JDouble
---------------

These types represent floating-point numbers:

- **JFloat**: 32-bit precision
- **JDouble**: 64-bit precision

JPype maps these types to Python's `float`. Numbers exceeding the range of `JFloat`
or `JDouble` will result in positive or negative infinity. Range checks are
performed when converting Python types, and an `OverflowError` will be raised if
the value is out of bounds.

.. code-block:: python

   # Example usage
   java_double = JDouble(3.14)
   print(java_double)  # Output: 3.14


.. _jpype_types_objects__classes:

Objects & Classes
=================

In contrast to primitive data type, objects can hold any combination of
primitives or objects.  Thus they represent structured data.  Objects can also
hold methods which operate on that data.  Objects can inherit from one another.

However unlike Python, Java objects must have a fixed structure which defines
its type.  These are referred to the object's class.  Here is a point of
confusion.  Java has two different class concepts: the class definition and the
class instance.  When you import a class or refer to a method using the class
name you are accessing the class definition.  When you call ``getClass`` on an
object it returns a class instance.  The class instance is a object whose
structure can be used to access the data and methods that define the class
through reflection.  The class instance cannot directly access the fields or
method within a class but instead provides its own interface for querying the
class.  For the purposes of this document a "class" will refer to the class
definition which corresponds to the Python concept of a class. Wherever the
Java reflection class is being referred to we will use the term "class
instance".  The term "type" is synonymous with a "class" in Java, though often
the term "type" is only used when inclusively discussing the type of primitives
and objects, while the term "class" generally refers to just the types
associated with objects.

All objects in Java inherit from the same base class ``java.lang.Object``, but
Java does not support multiple inheritance.  Thus each class can only inherit
from a single parent.  Multiple inheritance, mix-ins, and diamond pattern are
not possible in Java.  Instead Java uses the concept of an interface.  Any Java
class can inherit as many interfaces as it wants, but these interfaces may not
contain any data elements.  As they do not contain data elements there can
be no ambiguity as to what data a particular lookup.

.. _JInterface:

The meta class ``JInterface`` is used to check if a class type is an interface
using ``isinstance``.  Classes that are pure interfaces cannot be instantiated,
thus, there is not such thing as an abstract instance.  Therefore, every
Java object should have Objects cannot actual be pure interfaces.  To
represent this in Python every interface inherits ``java.lang.Object`` methods
even through it does not have ``java.lang.Object`` as a parent.  This ensures
that anonymous classes and lambdas have full object behavior.

.. _jpype_types_classes:

Classes
-------

In JPype, Java classes are instances of the Python ``type`` and function like
any ordinary Python class.  However unlike Python types, Java classes are
closed and cannot be extended.  To enforce extension restrictions, all Java
classes are created from a special private meta class called
``_jpype._JClass``.  This gatekeeper ensures that the attributes of classes
cannot be changed accidentally nor extended.  The type tree of Java is fixed
and closed.

All Java classes have the following functionality.

Class constructor
  The class constructor is accessed by using the Python call syntax ``()``.
  This special method invokes a dispatch whenever the class is called
  as a function.  If an matching constructor is found a new Java instance
  is created and a Python handle to that instance is returned.  In the case
  of primitive types, the constructor creates a Java value with the exact
  type requested.

Get attribute
  The Python ``.`` operator gets an attribute from a class with a specified
  name.  If no method or field exists a ``AttributeError`` will be raised.
  For public static methods, the getattr will produce a Python descriptor which
  can be called to invoke the static method.  For public static fields, a Python
  descriptor will be produced that allows the field to be get or set depending
  on whether the field is final or not.  Public instance methods and instance
  fields will produce a function that can be applied to a Java object to
  execute that method or access the field.  Function accessors are
  non-virtual and thus they can provide access to behaviors that have been
  hidden by a derived class.

Set attribute
  In general, JPype only allows the setting of public non-final fields.  If you
  attempt to set any attribute on an object that does not correspond to a
  settable field it will produce an ``AttributeError``.  There is one exception
  to this rule.  Sometime it is necessary to attach addition private meta data to
  classes and objects.  Attributes that begin with an underbar are consider to be
  Python private attributes.  Private attributes handled by the default Python
  attribute handler allowing these attributes to be attached to to attach data to
  the Python handle.  This data is invisible to Java and it is retained only on
  the Python instance.  If an object with Python meta data is passed to Java
  and Java returns the object, the new Python handle will not contain any of the
  attached data as this data was lost when the object was passed to Java.

``class_`` Attribute
  For Java classes there is a special attribute called ``class``.  This
  is a keyword in Python so `name mangling`_ applies.  This is a class instance
  of type ``java.lang.Class``.  It can be used to access fields and methods.

Inner classes
  For methods and fields, public inner classes appear as attributes of
  the class.  These are regular types that can be used to construct objects,
  create array, or cast.

String
  The Java method ``toString`` is mapped into the Python function ``str(obj)``.

Equality
  The Java method ``equals()`` has been mapped to Python ``==`` with special
  augmentations for null pointers.  Java ``==`` is not exposed directly
  as it would lead to numerous errors.  In principle, Java ``==`` should map
  to the Python concept of ``is`` but it is not currently possible to overload
  Python in such a way to achieve the desired effect.

Hash
  The Java method ``hashCode`` is mapped to Python ``hash(obj)`` function.
  There are special augmentations for strings and nulls.  Strings will return
  the same hash code as returned by Python so that Java strings and Python
  strings produce the same dictionary lookups.  Null pointers produce the
  same hash value as None.

  Java defines ``hashCode`` on many objects including mutable ones.  Often
  the ``hashCode`` for a mutable object changes when the object is changed.
  Only use immutable Java object (String, Instant, Boxed types) as
  dictionary keys or risk undefined behavior.

Java objects are instances of Java classes and have all of the methods defined
in the Java class including static members.  However, the get attribute method
converts public instance members and fields into descriptors which act on
the object.

Now that we have defined the basics of Java objects and classes, we will
define a few special classes that operate a bit differently.

.. _jpype_types_array_classes:

Array Classes
-------------

In Java, all arrays are objects, but they cannot define any methods beyond a
limited set of Java array operations. These operations have been mapped into
Python to their closest Python equivalent.

`JArray` is an abstract base class for all Java array classes. Thus, you can
test if something is an array class using ``issubclass``, and check if a Java
object is an array using ``isinstance``.

Creating Array Types
~~~~~~~~~~~~~~~~~~~~

In principle, you can create an array class using ``JClass``, but the signature
required would need to use the proper name as required for the Java method
``java.lang.Class.forName``. Instead, JPype provides two specialized methods to
create array types: arrays may be produced through the factory ``JArray`` or
through the index operator ``[]`` on any `JClass` instance.

.. _JArray:

The signature for `JArray` is ``JArray(type, [dims=1])``. The `type` argument
accepts any Java type, including primitives, and constructs a new array class.
This class can be used to create new instances, cast, or serve as the input to
the array factory. The resulting object has a constructor method that takes
either:

- A number, which specifies the desired size of the array.
- A sequence, which provides the elements of the array. If the members of the
  initializer sequence are not Java objects, each will be converted. If any
  element cannot be converted, a ``TypeError`` will be raised.

As a shortcut, the ``[]`` operator can be used to specify an array type or
create a new instance of an array with a specified length. You can also create
multidimensional arrays or arrays with unspecified dimensions after a specific
point. This applies to both primitive and object types. Because of the number
of options, we will walk through each use case.

To create a one-dimensional array type, append ``[:]`` to any Java class or
primitive type. For example:

- ``JInt[:]`` creates a Java array type for integers.
- ``java.lang.Object[:]`` creates a Java array type for objects.
- ``java.util.List[:]`` creates a Java array type for lists.

Once the array type is created, you can use it to construct arrays, cast Python
sequences to Java arrays, or define multidimensional arrays.

.. code-block:: python

   # Example: Creating array types
   int_array_type = JInt[:]
   object_array_type = java.lang.Object[:]

   # Creating arrays
   int_array = int_array_type([1, 2, 3])
   object_array = object_array_type([None, "Hello", 42])

   print(int_array)  # Output: [1, 2, 3]
   print(object_array)  # Output: [null, Hello, 42]

Multidimensional Arrays
~~~~~~~~~~~~~~~~~~~~~~~

JPype supports the creation of multi-dimensional arrays by appending additional
dimensions using ``[:]``. For example:

- ``JInt[:,:]`` creates a two-dimensional array type for integers.
- ``java.lang.Object[:,:]`` creates a two-dimensional array type for objects.
- ``JDouble[:,:,:]`` creates a three-dimensional array type for double-precision
  floating-point numbers.

When creating multi-dimensional arrays, you can initialize them using nested
Python lists. JPype automatically converts nested lists into the appropriate
Java array structure.

.. code-block:: python

   # Example: Creating multidimensional arrays
   int_2d_array_type = JInt[:, :]
   int_2d_array = int_2d_array_type([[1, 2], [3, 4]])

   print(int_2d_array[0][1])  # Output: 2

   # Creating a 3D array
   double_3d_array_type = JDouble[:, :, :]
   double_3d_array = double_3d_array_type([[[1.1, 2.2], [3.3, 4.4]], [[5.5, 6.6], [7.7, 8.8]]])

   print(double_3d_array[1][0][1])  # Output: 6.6

Jagged Arrays
~~~~~~~~~~~~~

Java supports jagged arrays, which are arrays of arrays with varying lengths.
To create jagged arrays in JPype, replace the final dimension with `[:]`. For
example:

- `JInt[5, :]` creates a jagged array of integers with 5 rows.
- `java.lang.Object[3, :]` creates a jagged array of objects with 3 rows.

Jagged arrays can be initialized using nested Python lists with varying lengths.

.. code-block:: python

   # Example: Creating jagged arrays
   jagged_int_array_type = JInt[3, :]
   jagged_int_array = jagged_int_array_type([[1, 2], [3, 4, 5], [6]])

   print(jagged_int_array[1][2])  # Output: 5

Use of Java Arrays
~~~~~~~~~~~~~~~~~~

Java arrays provide several Python methods:

- **Get Item**:
  Arrays are collections of elements. Array elements can be accessed using the
  Python ``[]`` operator. For multidimensional arrays, JPype uses Java-style
  access with a series of index operations, such as ``jarray[4][2]``.

- **Get Slice**:
  Arrays can be accessed using slices, like Python lists. The slice operator is
  ``[start:stop:step]``. Note that array slices are views of the original array,
  so any alteration to the slice will affect the original array. Use the `clone`
  method to create a copy of the slice if needed.

- **Set Item**:
  Array items can be set using the Python ``[]=`` operator.

- **Set Slice**:
  Multiple array items can be set using a slice assigned with a sequence. The
  sequence must have the same length as the slice. If the items being transferred
  are a buffer, a faster buffer transfer assignment will be used.

- **Buffer Transfer**:
  Buffer transfers from Java arrays work for primitive types. Use Python's
  ``memoryview(jarray)`` function to create a buffer for transferring data.
  Memory views of Java arrays are not writable.

- **Iteration (For Each)**:
  Java arrays can be used in Python `for` loops and lopp comprehensions.

- **Clone**:
  Java arrays can be duplicated using the `clone()` method.

- **Length**:
  Arrays in Java have a defined, immutable length. Use Python's ``len(array)``
  function to get the array length.

Character specialization
~~~~~~~~~~~~~~~~~~~~~~~~

- The Java class `JChar[]` has additional customizations to work better with
  string types.
- Java arrays do not support additional mathematical operations at this time.
- Creating a Java array is required for pass-by-reference syntax when using Java
  methods that modify array contents.

.. code-block:: python

   orig = [1, 2, 3]
   obj = jpype.JInt[:](orig)
   a.modifies(obj)   # Modifies the array by multiplying all elements by 2
   orig[:] = obj     # Copies all the values back from Java to Python


.. _jpype_types_buffer_classes:

Buffer classes
--------------

In addition to array types, JPype also supports Java ``nio`` buffer types.
Buffers in Java come in two flavors.  Array backed buffers have no special
access.  Direct buffers are can converted to Python buffers with both
read and write capabilities.

Each primitive type in Java has its own buffer type named based on the
primitive type.  ``java.nio.ByteBuffer`` has the greatest control allowing
any type to be read and written to it.  Buffers in Java function are like
memory mapped files and have a concept of a read and write pointer which
is used to traverse the array.  They also have direct index access to their
specified primitive type.

Java buffer provide an additional Python method:

Buffer transfer
  Buffer transfers from a Java buffer works for a direct buffer.  Array backed
  buffers will raise a ``BufferError``.  Use the Python ``memoryview(jarray)``
  function to create a buffer that can be used to transfer any portion of a Java
  buffer out.  Memory views of Java buffers are readable and writable.

Buffers do not currently support element-wise access.


.. _jpype_types_boxed_classes:

Boxed Classes
-------------

Often one wants to be able to place a Java primitive into a method of
fields that only takes an object.  The process of creating an object from a
primitive is referred to as creating a "boxed" object.  The resulting object is
an immutable object which stores just that one primitive.

Java boxed types in JPype are wrapped with classes that inherit from Python
``int`` and ``float`` types as both are immutable in Python.  This means that
a boxed type regardless of whether produced as a return or created explicitly
are treated as Python types. They will obey all the conversion rules
corresponding to a Python type as implicit matches.

In addition, they produce an exact match with their corresponding Java
type. The type conversion for this is somewhat looser than Java.  While Java
provides automatic unboxing of a Integer to a double primitive, JPype can
implicitly convert Integer to a Double boxed.

To box a primitive into a specific type such as to place it into a
``java.util.List`` use ``JObject`` on the desired boxed type or call
the constructor for the desired boxed type directly.  For example:

.. code-block:: python

     lst = java.util.ArrayList()
     lst.add(JObject(JInt(1)))      # Create a Java integer and box it
     lst.add(java.lang.Integer(1))  # Explicitly create the desired boxed object

JPype boxed classes have some additional functionality.  As they inherit from
a mathematical type in Python they can be used in mathematical operations.
But unlike Python numerical types they can take an addition state corresponding
to being equal to a null pointer.  The Python methods are not aware of this
new state and will treat the boxed type as a zero if the value is a null.

To test for null, cast the boxed type to a Python type explicitly and the
result will be checked.  Casting null pointer will raise a ``TypeError``.

.. code-block:: python

     b = JObject(None, java.lang.Integer)
     a = b+0      # This succeeds and a gets the value of zero
     a = int(b)+0 # This fails and raises a TypeError

Boxed objects have the following additional functionality over a normal object.

Convert to index
  Integer boxed types can be used as Python indices for arrays and other
  indexing tasks. This method checks that the value of the boxed
  type is not null.

Convert to int
  Integer and floating point boxed types can be cast into a Python integer
  using the ``int()`` method.  The resulting object is always of type ``int``.
  Casting a null pointer will raise a ``TypeError``.

Convert to float
  Integer and floating point boxed types can be cast into a Python float
  using the ``float()`` method.  The resulting object is always of type
  ``float``.  Casting a null pointer will raise a ``TypeError``.

Comparison
  Integer and floating point types implement the Python rich comparison API.
  Comparisons for null pointers only succeed for ``==`` and ``!=`` operations.
  Non-null boxed types act like ordinary numbers for the purposes of
  comparison.


.. _jpype_types_number_class:

Number Class
------------

The Java class ``java.lang.Number`` is a special type in Java. All numerical
Java primitives and Python number types can convert implicitly into a
Java Number.

========================== ========================
Input                      Result
========================== ========================
None                       java.lang.Number(null)
Python int, float          java.lang.Number
Java byte,   NumPy int8    java.lang.Byte
Java short,  NumPy int16   java.lang.Short
Java int,    NumPy int32   java.lang.Integer
Java long,   NumPy int64   java.lang.Long
Java float,  NumPy float32 java.lang.Float
Java double, NumPy float64 java.lang.Double
========================== ========================

Additional user defined conversion are also applied.  The primitive types
boolean and char and their corresponding boxed types are not considered to
numbers in Java.

.. _java.lang.Object:

Object Class
------------

Although all classes inherit from Object, the object class itself has special
properties that are not inherited.  All Java primitives will implicitly convert
to their box type when placed in an Object.  In addition, a number of Python
types implicitly convert to a Java object.  To convert to a different object
type, explicitly cast the Python object prior to placing in a Java object.

Here a table of the conversions:

================ =======================
Input            Result
================ =======================
None             java.lang.Object(null)
Python str       java.lang.String
Python bool      java.lang.Boolean
Python int       java.lang.Number
Python float     java.lang.Number
================ =======================

In addition it inherits the conversions from ``java.lang.Number``.
Additional user defined conversion are also applied.

.. _java.lang.String:

String Class
------------

The String class in Java is a special representation often pointing either to
a dynamically created string or to a constant pool item defined in the class.
All Java strings are immutable just like Python strings and thus these are
considered to be equivalent classes.

Because Java strings are in fact just pointers to blob of bytes they are
actually slightly less than a full object in some JVM implementation.  This is
a violation of the Object Orients (OO) principle, never take something away by
inheritance.  Unfortunately, Java is a frequent violator of that rule, so
this is just one of those exceptions you have to trip over.  Therefore, certain
operations such as using a string as a threading control with ``notify`` or ``wait``
may lead to unexpected results.  If you are thinking about using a Java string
in synchronized statement then remember it is not a real object.

Java strings have a number of additional functions beyond a normal
object.

Length
  Java strings have a length measured in the number of characters required
  to represent the string.  Extended Unicode characters
  count for double for the purpose of counting characters.  The string length
  can be determined using the Python ``len(str)`` function.

Indexing
  Java strings can be used as a sequence of characters in Python and thus
  each character can be accessed as using the Python indexing operator ``[]``.

Hash
  Java strings use a special hash function which matches the Python hash code.
  This ensures that they will always match the same dictionary keys as
  the corresponding string in Python.  The Python hash can be determined using
  the Python ``hash(str)`` function.  Null pointers are not currently handled.
  To get the actually Java hash, use ``s.hashCode()``

Contains
  Java strings implement the concept of ``in`` when using the Java method
  ``contains``.  The Java implementation is sufficiently similar that it will
  work fairly well on strings.
  For example, ``"I" in java.lang.String("team")`` would be equal to False.

  Testing other types using the ``in`` operator
  will likely raise a ``TypeError`` if Java is unable to convert the other item
  into something that can be compared with a string.

Concatenation
  Java strings can be appended to create a new string which contains the
  concatenation of the two strings.  This is mapped to the Python operator
  ``+``.

Comparison
  Java strings are compared using the Java method ``compareTo``.  This
  method does not currently handle null and will raise an exception.

For each
  Java strings are treated as sequences of characters and can be used with a
  for-loop construct and with list comprehension.  To iterate through all of the
  characters, use the Python construct ``for c in str:``.

Unfortunately, Java strings do not yet implement the complete list of
requirements to act as Python sequences for the purposes of
``collections.abc.Sequence``.

.. _JString:

The somewhat outdated JString factory is a Python class that pretends to be a
Java string type.  It has the marginal advantage that it can be imported before
the JVM is actually started.  Once the JVM is started, its class representation
is pointed to ``java.lang.String`` and can be used to construct a new string
object or to test if an object is actually a Java string using ``isinstance``.
It does not implement any of the other string methods and just serves as
convenience class.  The more capable ``java.lang.String`` can be imported
in place of JString, but only after the JVM is started.

String objects may optionally convert to Python strings when returned
from Java methods, though this option is a performance issue and can lead to
other difficulties.  This setting is selected when the JVM is started.
See `String Conversions`_ for details.

Java strings will cache the Python conversion so we only pay the conversion
cost once per string.


.. _jpype_types_exception_classes:

Exception Classes
-----------------

Both Python and Java treat exception classes differently from other objects.
Only these types may be caught as part of a try block.  Therefore, the
exceptions have a special wrapper.  Most of the mechanics of exceptions happen
under the surface.  The one difference between Python and Java is the behavior
when the argument is queried.  Java arguments can either be the string value, the exception
itself, or the internal construction key depending on how the exception came
into existence.  Therefore, the arguments to a Java exception should never be
used as their values are not guaranteed.

Java exception can report their stacktrace to Python in two different ways.  If
printed through the Python stack trace routine, Java exceptions are split
between the Python code that raised and a phantom Java ``cause`` which contains the
Java exception in Python order.  If the debugging information for the Java
source is enabled, Python may even print the Java source code lines
where the error occurred.  If you prefer Java style stack traces then print the
result from the ``stacktrace()`` method.  Unhandled exception that terminate
the program will print the Python style stack trace information.

.. _JException:

The base class ``JException`` is a special type located in ``jpype.types`` that
can be imported prior to the start of the JVM.  This serves as the equivalent
of ``java.lang.Throwable`` and contains no additional methods.  It is currently
being phased out in favor of catching the Java type directly.

Using ``jpype.JException`` with a class name as a string was supported in
previous JPype versions but is currently deprecated.  For further information
on dealing with exception, see the `Exception Handling`_ section.  To create a
Java exception use JClass or any of the other importing methods.


.. _jpype_types_anonymous_classes:

Anonymous Classes
-----------------

Sometimes Java will produce an anonymous class which does to have any actual
class representation.  These classes are generated when a method implements
a class directly as part of its body and they serve as a closure with access
to some of the variables that were used to create it.

For the purpose of JPype these classes are treated as their parents.  But this
is somewhat problematic when the parent is simply an interface and not an actual
object type.


.. _jpype_types_lambdas:

Lambdas
-------

The companion of anonymous classes are lambda classes.  These are generated
dynamically and their parent is always an interface.  Lambdas are always
Single Abstract Method (SAM) type interfaces.  They can implement additional
methods in the form of default methods but those are generally not accessible
within JPype.

.. _jpype_types_inner_classes:

Inner Classes
-------------

For the most part, inner classes can be used like normal classes, with the
following differences:

- Inner classes in Java natively use $ to separate the outer class from the
  inner class. For example, inner class Foo defined inside class Bar is called
  Bar.Foo in Java, but its real native name is Bar$Foo.
- Inner classes appear as member of the containing class. Thus to access them
  import the outer class and call them as members.
- Non-static inner classes cannot be instantiated from Python code.  Instances
  received from Java code can be used without problem.

.. _jpype_types_buffer_transfers:

Buffer Transfers
----------------
Java arrays provide efficient buffer transfers for primitive types using Python's
`memoryview`. This allows seamless integration with libraries like NumPy for
numerical operations. For strategies to optimize data exchange, 
see :ref:`Optimize Data Transfers <optimize_data_transfers>`.

.. code-block:: python

   # Example: Buffer transfer
   import numpy as np

   int_array = JInt[:](5)
   int_array[:] = [1, 2, 3, 4, 5]  # Transfer data to Java array

   buffer = memoryview(int_array)
   np_array = np.array(buffer)     # Convert to NumPy array

   print(np_array)  # Output: [1, 2, 3, 4, 5]


.. _import:

Importing Java classes
======================

As Java classes are remote from Python and can neither be created nor extended within
Python, they must be imported.  JPype provides three different methods for
creating classes.

The highest level API is the use of the import system.
To import a Java class, one must first import the optional module
``jpype.imports`` which has the effect of binding the Java package system
to the Python module lookup.  Once this is completed package or class can
be imported using the standard Python import system.  The import system
offers a very rich error reporting system.  All failed imports produce
an ``ImportError`` with diagnostics as to what went wrong.  Errors include
unable to find the class, unable to find a required dependency, and incorrect
Java version.

One important caveat when dealing with importing Java modules.  Python always
imports local directories as modules before calling the Java importer.  So any
directory named ``java``, ``com``, or ``org`` will hide corresponding Java
package.  We recommend against naming directories as ``java`` or top level
domain.

.. _JPackage:

The older method of importing a class is with the ``JPackage`` factory.
This factory automatically loads classes as attributes as requested.
If a class cannot be found it will produce an ``AttributeError``.  The
symbols ``java`` and ``javax`` in the ``jpype`` module are both ``JPackage``
instances.  Only public classes appear on ``JPackage`` but protected and even
private classes can be accessed by name.  Though most private classes
don't have any methods or fields that can be accessed.

.. _JClass:

The last mechanism for looking up a class is through the use of the ``JClass``
factory.  This is a low level API allowing the loading of any class available
using the forName mechanism in Java.  The JClass method can take up to three
arguments corresponding to arguments of the forName method and can be used
with alternative class loaders.  The majority of the JPype test bench uses
JClass so that the tests are only evaluating the desired functionality and not
the import system.  But this does not imply that JClass is the preferred
mechanic for importing classes.  The first argument can be a string or
a Java class instance.  There are two keyword arguments ``loader`` and
``initialize``.  The loader can point to an alternative ClassLoader which
is handy when loading custom classes through mechanisms such as over the
web.  A False ``initialize`` argument loads a class without
loading dependencies nor populating static fields.  This option is likely
not useful for ordinary users.  It was provided when calling forName was problematic
due to `caller sensitive`_ issues.


.. _name_mangling:

Name mangling
=============

When providing Java package, classes, methods, and fields to Python,
there are occasionally naming conflicts.  For example, if one has a method
called ``with`` then it would conflict with the Python keyword ``with``.
Wherever this occurs, JPype renames the offending symbol with a trailing
under bar.  Java symbols with a leading or trailing under bars are consider to
be privates and may not appear in the JPype wrapper entirely with the exception
of package names.

The following Python words will trigger name mangling of a Java name:

=========== =========== ============= =========== ==========
``False``   ``None``    ``True``      ``and``     ``as``
``async``   ``await``   ``def``       ``del``     ``elif``
``except``  ``exec``    ``from``      ``global``  ``in``
``is``      ``lambda``  ``nonlocal``  ``not``     ``or``
``pass``    ``print``   ``raise``     ``with``    ``yield``
=========== =========== ============= =========== ==========


.. _methods:
.. _jpype_types_method_resolution:


Method Resolution
=================

Because Java supports method overloading and Python does not, JPype wraps Java
methods as a "method dispatch". The dispatch is a collection of all of the
methods from the class and all of its parents which share the same name. The
job of the dispatch is to choose the method to call. Enforcement of the strong
typing of Java must be performed at runtime within Python. Each time a method
is invoked, JPype must match against the list of all possible methods that the
class implements and choose the best possible overload. For this reason, the
methods that appear in a JPype class will not be the actual Java methods, but
rather a "dispatch" whose job is deciding which method should be called based
on the type of the provided arguments. If no method is found that matches the
provided arguments, the method dispatch will produce a ``TypeError``. This is
the exact same outcome that Python uses when enforcing type safety within a
function. If a type doesn't match, a ``TypeError`` will be produced.

Dispatch Example
----------------

When JPype is unable to decide which overload of a method to call, the user
must resolve the ambiguity. This is where casting comes in. Take for example
the ``java.io.PrintStream`` class. This class has a variant of the print and
println methods! So for the following code:

.. code-block:: python

   java.lang.System.out.println(1)

JPype will automatically choose the ``println(long)`` method, because the
Python ``int`` matches exactly with the Java ``long``, while all the other
numerical types are only "implicit" matches. However, if that is not the
version you wanted to call, you must cast it. In this case, we will use a
primitive type to construct the correct type. Changing the line thus:

.. code-block:: python

   java.lang.System.out.println(JByte(1))  # <--- wrap the 1 in a JByte

This tells JPype to choose the byte version. When dealing with Java types,
JPype follows the standard Java matching rules. Types can implicitly grow to
larger types but will not shrink without an explicit cast.

Caching Optimization for Method Resolution
------------------------------------------

JPype optimizes method resolution by caching the results of previous matches.
If the same method is called repeatedly with the same argument types (e.g.,
inside a loop or list comprehension), JPype reuses the cached resolution,
avoiding the overhead of re-evaluating all overloads. This greatly improves
performance for repetitive calls.

For example, consider the following code:

.. code-block:: python

   fruits = ["apple", "orange", "banana"]
   jlist = java.util.ArrayList()
   [jlist.add(fruit) for fruit in fruits]  # Cached resolution for each iteration

In this case, JPype caches the resolution for ``add(str)`` to ``add(String)``
method after the first call, and subsequent calls reuse the cached result. This
optimization is particularly beneficial in loops and list comprehensions.  A
call to ``add(int)`` would trigger a new resolution.  The next call to ``add(str)``
will once again trigger a resolution request.

**Note**: For an in-depth discussion on how this caching mechanism improves
loop performance, particularly in list comprehensions, see the
:ref:`Performance <miscellaneous_topics_performance>` section.

Interactions of Custom Converters and Caching
---------------------------------------------

It is unwise to define very broad conversions as it can interact poorly with 
caching. Suppose that one defined a convertion from all Python strings to the
Java class for date under some condition. Or perhaps an even broader convserion
was defined such as all Python classes that inherit from object.

If such overly broad conversions are applied to a function
for which both date and string were acceptable it were prefer the date
conversion when method resolution starts.  As the type for the cache was string
it would attempt the out of order resolution of date first.  If the 
condition yield a fail it will fall back to normal method resolution, but
an overly broad conversion specialization may end up being dispatched to the 
previously defined conversion.

Under normal operation of JPype the type conversions are narrowly defined such
that the cache will always yield the proper resolution.  But user defined
conversions may cause unexpected results.  In such a case, a cast operation to
the Java type would be required to resolve the ambiguity.



.. _jpype_types_type_matching:

Type Matching
=============

This section provides tables documenting the JPype conversion rules.
JPype defines different levels of "match" between Python objects and Java
types. These levels are:

- **none**, There is no way to convert.
- **explicit (E)**, JPype can convert the desired type, but only
  explicitly via casting.  Explicit conversions are only execute automatically
  in the case of a return from a proxy.
- **implicit (I)**, JPype will convert as needed.
- **exact (X)**, Like implicit, but when deciding with method overload
  to use, one where all the parameters match "exact" will take precedence
  over "implicit" matches.

See the previous section on `Java Conversions`_ for details.

There are special conversion rules for ``java.lang.Object`` and ``java.lang.Number``.
(`Object Class`_ and `Number Class`_)

============== ========== ========= =========== ========= ========== ========== =========== ========= ========== =========== ========= ================== =================
Python\\Java    byte      short       int       long       float     double     boolean     char      String      Array       Object    java.lang.Object   java.lang.Class
============== ========== ========= =========== ========= ========== ========== =========== ========= ========== =========== ========= ================== =================
    int         I [1]_     I [1]_       X          I        I [3]_     I [3]_     X [8]_                                                       I [11]_
   long         I [1]_     I [1]_     I [1]_       X        I [3]_     I [3]_                                                                  I [11]_
   float                                                    I [1]_       X                                                                     I [11]_
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

.. [4] Number of dimensions must match and the types must be
       compatible.

.. [6] Only if the specified type is a compatible array class.

.. [7] The object class is an exact match, otherwise
       implicit.

.. [8] Only the values `True` and `False` are implicitly converted to
       booleans.

.. [9] Primitives are boxed as per Java rules.

.. [10] Java boxed types are mapped to Python primitives, but will
        produce an implicit conversion even if the Python type is an exact
        match. This is to allow for resolution between methods
        that take both a Java primitve and a Java boxed type.

.. [11] Boxed to ``java.lang.Number``


Exception Handling
==================

Error handling is an important part of any non-trivial program. All Java
exceptions occurring within Java code raise a `jpype.JException`, which derives
from Python's `Exception`. These can be caught either using a specific Java
exception or generically as a `jpype.JException` or `java.lang.Throwable`. You
can then use the `stacktrace()`, `str()`, and `args` to access extended
information.

.. _jpype_types_catching_a_specific_java_exception:

Catching a Specific Java Exception
----------------------------------

The following example demonstrates catching a specific Java exception:

.. code-block:: python

    try:
        # Code that throws a java.lang.RuntimeException
    except java.lang.RuntimeException as ex:
        print("Caught the runtime exception:", str(ex))
        print(ex.stacktrace())

.. _jpype_types_catching_multiple_java_exceptions:

Catching Multiple Java Exceptions
---------------------------------

Multiple Java exceptions can be caught together or separately:

.. code-block:: python

    try:
        # Code that may throw various exceptions
    except (java.lang.ClassCastException, java.lang.NullPointerException) as ex:
        print("Caught multiple exceptions:", str(ex))
        print(ex.stacktrace())
    except java.lang.RuntimeException as ex:
        print("Caught runtime exception:", str(ex))
        print(ex.stacktrace())
    except jpype.JException as ex:
        print("Caught base exception:", str(ex))
        print(ex.stacktrace())
    except Exception as ex:
        print("Caught Python exception:", str(ex))

.. _jpype_types_raising_exceptions_from_python_to_java:

Raising Exceptions from Python to Java
--------------------------------------

Exceptions can be raised in proxies to throw an exception back to Java.
Exceptions within the JPype core are issued with the most appropriate Python
exception type, such as `TypeError`, `ValueError`, `AttributeError`, or
`OSError`.

.. _jpype_types_raising_exceptions_in_proxies:

Raising Exceptions in Proxies
-----------------------------

JPype allows Python proxies to raise exceptions that are propagated back to
Java. This is particularly useful when implementing Java interfaces in Python
and handling invalid inputs or unexpected conditions.

When an exception is raised in Python, it is wrapped in a `RuntimeException` in
Java. If the exception propagates back to Python, it is unpacked to return the
original Python exception.

.. _jpype_types_example:

Example
~~~~~~~

The following example demonstrates raising a Python exception from a proxy:

.. code-block:: python

    import jpype
    import jpype.imports

    jpype.startJVM()

    from java.util.function import Function

    @jpype.JImplements(Function)
    class MyFunction:
        @jpype.JOverride
        def apply(self, value):
            if value is None:
                raise ValueError("Invalid input: None is not allowed")
            return value.upper()

    try:
        func = MyFunction()
        result = func.apply(None)  # This will raise a ValueError
    except ValueError as ex:
        print("Caught Python exception:", str(ex))


.. _jpype_types_exception_aliasing:

Exception Aliasing
------------------

Certain exceptions in Java have a direct correspondence with existing Python
exceptions. Rather than forcing JPype to translate these exceptions or
requiring the user to handle Java exception types throughout the code, these
exceptions are "derived" from their Python counterparts. This allows the user
to catch them using standard Python exception types.

+---------------------------------------+------------------+
| Java Exception                        | Python Exception |
+---------------------------------------+------------------+
| `java.lang.IndexOutOfBoundsException` | `IndexError`     |
| `java.lang.NullPointerException`      | `ValueError`     |
+---------------------------------------+------------------+


.. _jpype_types_aliasing_example:

Aliasing Example
~~~~~~~~~~~~~~~~

The following example demonstrates catching an aliased exception:

.. code-block:: python

    try:
        # Code that throws a java.lang.IndexOutOfBoundsException
    except IndexError as ex:
        print("Caught IndexError:", str(ex))

By deriving these exceptions from Python, the user is free to catch the
exception either as a Java exception or as the more general Python exception.
Remember that Python exceptions are evaluated in order from most specific to
least.


.. _controlling_the_jvm:

Controlling the JVM
*******************

In this chapter, we will discuss how to control the JVM from within Python.
For the most part, the JVM is invisible to Python.  The only user controls
needed are to start up and shutdown the JVM.

.. _startJVM:

Starting the JVM
================

JPype requires the Java Virtual Machine (JVM) to be started before interacting
with Java. This section explains how to start the JVM, configure its options,
and troubleshoot common issues.

.. _controlling_the_jvm_key_requirements:

Key Requirements
----------------
Before starting the JVM, ensure the following prerequisites are met:

1. **Java Installation**: A Java Runtime Environment (JRE) or Java Development
   Kit (JDK) must be installed. JPype supports Java versions 11 and later.

2. **Architecture Match**: The architecture of the Python interpreter (e.g.,
   64-bit or 32-bit) must match the architecture of the installed JVM.

3. **Classpath Configuration**: Specify the paths to Java classes or JAR files
   required by your application.

4. **Environment Variable**: Ensure the `JAVA_HOME` environment variable is set
   to the directory containing the Java installation.


How to Start the JVM
--------------------
To start the JVM, use the ``jpype.startJVM()`` function. This function
initializes the JVM with the specified options. The key arguments are:

- **``classpath``**: A list of paths to JAR files or directories containing
  Java classes.
- **``convertStrings``**: A boolean flag controlling whether Java strings are
  automatically converted to Python strings.
- **``ignoreUnrecognized``**: A flag that suppresses errors for unrecognized
  JVM options.
- **Additional JVM options**: Any valid JVM arguments (e.g., ``-Xmx`` for
  memory allocation).


Example: Starting the JVM
~~~~~~~~~~~~~~~~~~~~~~~~~
Here is a typical example of starting the JVM:

.. code-block:: python

    import jpype

    # Start the JVM with classpath and options
    jpype.startJVM(
        classpath=['lib/*', 'classes'],
        jvmOptions=["-ea"]  # Enable assertions
    )



Classpath Configuration
-----------------------
JPype supports two methods for specifying the classpath:

1. **``classpath`` Argument**: Pass a list of paths directly to the
   ``startJVM()`` function. Wildcards (``*``) are supported for JAR files in a
   directory.

.. code-block:: python

    jpype.startJVM(classpath=['lib/*', 'classes'])

2. **``addClassPath()`` Function**: Use ``jpype.addClassPath()`` to add paths
   dynamically before starting the JVM.

.. code-block:: python

    jpype.addClassPath('lib/*')
    jpype.addClassPath('classes')
    jpype.startJVM()

To debug classpath issues, print the effective classpath after starting the
JVM:

.. code-block:: python

    print(java.lang.System.getProperty('java.class.path'))

.. _controlling_the_jvm_handling_jar_files_compiled_for_newer_java_versions:

Handling JAR Files Compiled for Newer Java Versions
---------------------------------------------------
If a JAR file is compiled for a newer version of Java than the JVM being used,
JPype will fail to load the classes from the JAR file, and the JVM will throw
an ``UnsupportedClassVersionError``. This occurs because the JVM cannot
interpret class files compiled for a newer version.


.. _controlling_the_jvm_behavior:

Behavior
~~~~~~~~
When attempting to load a JAR file compiled for a newer version of Java, the
JVM will throw an error similar to the following::

    java.lang.UnsupportedClassVersionError: <class_name> has been compiled by a
    more recent version of the Java Runtime (class file version X), this
    version of the Java Runtime only recognizes class file versions up to Y.

For example:

- Java 11 corresponds to class file version 55.
- Java 17 corresponds to class file version 61.

If the JAR file contains class files compiled with a newer version than the
JVM supports, the JVM cannot interpret them.

.. _controlling_the_jvm_starting_the_jvm_handling_jar_files_compiled_for_newer_java_versions_steps_to_resolve:

Steps to Resolve
~~~~~~~~~~~~~~~~
1. **Upgrade the JVM**:
   Ensure the JVM version matches or exceeds the version used to compile the
   JAR file. Use the following command to check the JVM version::

       java -version

2. **Recompile the JAR**:
   If you have access to the source code, recompile the JAR with an older
   version of Java using the ``--release`` flag. For example::

       javac --release 11 -d output_directory source_files

   This ensures compatibility with Java 11.

3. **Check the Class File Version**:
   Use the ``javap`` command to verify the class file version of the JAR::

       javap -verbose <class_name>

   Look for the ``major version`` field in the output.


.. _controlling_the_jvm_best_practices:

Best Practices
~~~~~~~~~~~~~~
- Always ensure the JVM version matches the requirements of the JAR files
  being loaded.
- If possible, use JAR files compiled for long-term support (LTS) versions of
  Java, such as Java 11 or Java 17, to maximize compatibility.

.. _controlling_the_jvm_automatic_jvm_path_detection:

Automatic JVM Path Detection
----------------------------
JPype automatically detects the path to the JVM shared library using the
``JAVA_HOME`` environment variable. If ``JAVA_HOME`` is not set, JPype searches
common directories based on the platform. You can retrieve the detected path
using:

.. code-block:: python

    print(jpype.getDefaultJVMPath())

If the automatic detection fails, specify the JVM path manually as the first
argument to ``startJVM()``:

.. code-block:: python

    jpype.startJVM('/path/to/libjvm.so', classpath=['lib/*'])


.. _controlling_the_jvm_handling_nonascii_characters_in_the_jvm_path:

Handling Non-ASCII Characters in the JVM Path
----------------------------------------------
JPype has been revised to handle JVM paths containing non-ASCII characters. Due
to restrictions in Java, JPype must make a copy of the JVM shared library when
the path includes non-ASCII characters. This ensures compatibility with the
Java Virtual Machine.

**Windows-Specific Behavior**:
On Windows, the copied JVM shared library cannot be deleted after use due to
file locking restrictions imposed by the operating system. As a result, the
temporary file will remain on disk after the JVM is shut down.

**Implications**:

- The copied JVM shared library will occupy disk space until manually removed.

- This behavior is specific to Windows and does not affect Linux or macOS.

**Best Practices**:

- Avoid using non-ASCII characters in the JVM path when running JPype on
  Windows to prevent unnecessary file duplication.

- If non-ASCII characters are unavoidable, ensure sufficient disk space is
  available for temporary files.

**Troubleshooting**:
To locate the copied JVM shared library, check the directory where the JVM path
is specified. The copied file will have the same name as the original shared
library but may include additional identifiers.

**Example**:
If the original JVM path is:
C:\Program Files\Java\jdk-11.0.7\bin\server\jvm.dll

And it contains non-ASCII characters, JPype will create a copy in a temporary directory.

This behavior is necessary to ensure compatibility with Java's handling of
non-ASCII paths.


.. _controlling_the_jvm_additional_flags_for_startjvm:

Additional Flags for `startJVM()`
---------------------------------
JPype provides several optional flags for `startJVM()` to customize the JVM
startup process:

1. **`jvmOptions`**: A list of JVM options for memory, debugging, or garbage
   collection tuning.
   Example: ``jpype.startJVM(jvmOptions=["-Xmx512m", "-XX:+UseG1GC"])``

2. **`ignoreUnrecognized`**: Suppresses errors for unrecognized JVM options.
   Example: ``jpype.startJVM(ignoreUnrecognized=True)``

3. **`convertStrings`**: Controls automatic conversion of Java strings to
   Python strings.
   Example: ``jpype.startJVM(convertStrings=False)``

4. **`classpath`**: Specifies paths to JAR files and Java classes.
   Example: ``jpype.startJVM(classpath=["lib/*", "classes"])``

5. **`jvmPath`**: Specifies the path to the JVM shared library.
   Example: ``jpype.startJVM(jvmPath="/path/to/libjvm.so")``

6. **`attachThread`**: Automatically attaches Python threads to the JVM.
   Example: ``jpype.startJVM(attachThread=True)``

7. **`disableGC`**: Disables JPype's garbage collection hooks.
   Example: ``jpype.startJVM(disableGC=True)``

8. **`stackTrace`**: Enables detailed stack traces for Java exceptions.
   Example: ``jpype.startJVM(stackTrace=True)``

9. **`initializers`**: A list of Python functions executed during JVM startup.
   Example: ``jpype.startJVM(initializers=[setup])``

10. **`modulePath`**: Specifies the module path for Java modular applications.
    Example: ``jpype.startJVM(modulePath=["modules/*"])``

.. _string_conversions:

String Conversions
------------------
The ``convertStrings`` argument controls whether Java strings are automatically
converted to Python strings. By default, this behavior is disabled
(``convertStrings=False``) to preserve Java string methods and avoid
unnecessary conversions.


If enabled (``convertStrings=True``), Java strings are returned as Python
strings, but this can impact performance and chaining of Java string methods.
This option is consisted a legacy option as it will result in unncessary
calls to ``str()`` every time a String is passed from Java.

Best practice: Set ``convertStrings=False`` unless your application explicitly
requires automatic conversion.


.. _controlling_the_jvm_checking_jvm_state:

Checking JVM State
------------------
Use the following functions to check the status of the JVM:

- **``jpype.isJVMStarted()``**: Returns ``True`` if the JVM is running.
- **``jpype.getJVMVersion()``**: Retrieves the version of the running JVM.

Example:

.. code-block:: python

    if not jpype.isJVMStarted():
        print("JVM is not running!")
    else:
        print("JVM version:", jpype.getJVMVersion())

.. _controlling_the_jvm_common_issues_and_troubleshooting:

Common Issues and Troubleshooting
---------------------------------
1. **Classpath Errors**: Ensure that all required JAR files and directories are
   included in the classpath. Use ``java.lang.System.getProperty('java.class.path')``
   to verify the effective classpath.

2. **Architecture Mismatch**: Ensure the Python interpreter and JVM have
   matching architectures (e.g., both 64-bit or both 32-bit). Running a 64-bit
   Python interpreter with a 32-bit JVM will cause startup failures.

3. **Environment Variable Issues**: Verify that the ``JAVA_HOME`` environment
   variable is set correctly. If necessary, set it manually:
   - **Windows**: ``set JAVA_HOME=C:\Program Files\Java\jdk-<version>``
   - **Linux/Mac**: ``export JAVA_HOME=/usr/lib/jvm/java-<version>``

4. **Unrecognized JVM Options**: If you encounter errors for unrecognized JVM
   options, use the ``ignoreUnrecognized=True`` flag to suppress them.

5. **Memory Allocation Errors**: Ensure sufficient memory is allocated to the
   JVM using the ``-Xmx`` option.

6. **Debugging Startup Failures**: Enable stack traces for additional
   diagnostics:

.. code-block:: python

    import _jpype
    _jpype.enableStacktraces(True)


.. _controlling_the_jvm_best_practices_for_jvm_starting:

Best Practices for JVM starting
-------------------------------
- **Start Early**: Start the JVM at the beginning of your program to avoid
  issues with imports and initialization.
- **Specify Classpath Explicitly**: Use the ``classpath`` argument to ensure
  all required JAR files and directories are loaded.
- **Disable String Conversion**: Set ``convertStrings=False`` for better
  control and performance.
- **Avoid Restarting the JVM**: JPype does not support restarting the JVM after
  it has been shut down. Design your application to start the JVM once and keep
  it running for the program's lifetime.
- **Monitor Resource Usage**: If your application uses large Java objects,
  monitor memory usage to avoid out-of-memory errors.

.. _controlling_the_jvm_starting_the_jvm_summary:

Summary of JVM starting
-----------------------
Starting the JVM is a critical step in using JPype to integrate Python with
Java. By following the guidelines in this section, you can ensure a smooth
startup process, avoid common pitfalls, and configure the JVM to meet your
application's needs. Proper classpath configuration, architecture matching, and
memory allocation are key to successful integration. Debugging tools and best
practices are available to help troubleshoot issues and optimize performance.


.. _shutdownJVM:

Shutting Down the JVM
======================

At the end of your program, you may want to shut down the JVM to terminate the
Java environment explicitly. While this is possible, it is generally not
recommended unless absolutely necessary. JPype automatically shuts down the JVM
when the Python process terminates, ensuring a clean exit without manual
intervention.

.. _controlling_the_jvm_risks_of_shutting_down_the_jvm:

Risks of Shutting Down the JVM
------------------------------

Shutting down the JVM manually can lead to serious risks and instability,
especially if there are lingering Java references or shared resources. Once the
JVM is shut down, all Java objects become invalid, and any attempt to access
them will result in errors. This includes:

- **Lingering Java References**: Any Java objects held by Python will become
  invalid after the JVM is shut down. Accessing these objects will raise
  exceptions and could result in undefined behavior.

- **Shared Resources**: Shared resources such as buffers (e.g., memory mapped
  from Java to NumPy) will become unstable. Accessing these buffers after the
  JVM is shut down may cause crashes or memory corruption.

- **Proxies and Threads**: If Java threads or proxies are active when the JVM is
  shut down, they will be terminated abruptly, potentially leaving the system in
  an inconsistent state.

- **Non-Daemon Threads**: All threads must be attached as daemon threads before
  shutting down the JVM. Non-daemon threads will block the shutdown process,
  causing it to hang indefinitely. Python threads that interact with Java are
  automatically attached as daemon threads by JPype, but any custom threads
  created in Java must also be marked as daemon.

For most applications, it is safer to allow the JVM to shut down automatically
when the Python process exits. This ensures that all resources are cleaned up
properly and avoids the risks associated with manual shutdown.

.. _controlling_the_jvm_how_jpype_shuts_down_the_jvm:

How JPype Shuts Down the JVM
----------------------------

JPype performs the following steps during JVM shutdown to ensure proper cleanup:

1. **Request JVM Shutdown**: JPype requests the JVM to shut down gracefully.
2. **Wait for Non-Daemon Threads**: The JVM waits for all non-daemon threads to
   terminate. If you have active Java threads, ensure they are properly
   terminated or marked as daemon before shutting down the JVM.
3. **Execute Shutdown Hooks**: The JVM executes any registered shutdown hooks.
   These hooks can be used to clean up resources before the JVM terminates.
4. **Release JPype Reference Queue**: JPype shuts down its internal reference
   queue, which is responsible for dereferencing Python resources tied to Java
   objects.
5. **Release JPype Type Manager**: JPype releases its type manager, which
   handles mappings between Python and Java types.
6. **Unload JVM Shared Library**: The JVM shared library is unloaded, freeing
   memory used by the JVM.
7. **Finalize Python Resources**: JPype cleans up any remaining Python handles
   tied to Java objects, ensuring that no invalid references remain.

Once the JVM is shut down, all Java objects are considered dead and cannot be
reactivated. Any attempt to access their data field will raise an exception.

.. _controlling_the_jvm_managing_threads_during_jvm_shutdown:

Managing Threads During JVM Shutdown
------------------------------------

The JVM requires all threads to be attached as daemon threads during shutdown.
Daemon threads are background threads that do not prevent the JVM from
terminating. Non-daemon threads, on the other hand, will block the shutdown
process, causing it to hang indefinitely until those threads terminate.

JPype automatically attaches Python threads that interact with Java as daemon
threads. However, if you create custom threads in Java, you must explicitly mark
them as daemon threads to ensure they do not block the JVM shutdown.

To mark a Java thread as a daemon, use the following pattern:

.. code-block:: python

    import java.lang.Thread

    # Create a Java thread
    thread = java.lang.Thread()

    # Mark the thread as daemon
    thread.setDaemon(True)

    # Start the thread
    thread.start()

If you need to check whether a thread is a daemon, use the `isDaemon()` method:

.. code-block:: python

    print(f"Thread is daemon: {thread.isDaemon()}")

Ensure that all non-daemon threads are properly terminated or marked as daemon
before shutting down the JVM. Failure to do so may cause the shutdown process to
hang indefinitely.

.. _controlling_the_jvm_how_to_shut_down_the_jvm:

How to Shut Down the JVM
------------------------

If you must shut down the JVM manually, you can use the `jpype.shutdownJVM()`
function. This should only be called from the main Python thread. Calling it
from any other thread will raise an exception.

.. code-block:: python

    import jpype

    # Shut down the JVM
    jpype.shutdownJVM()


Numerous examples found on the internet explicity state that `shutdownJVM` is a
good practice.  These examples are legecy from early developement.  At the time
shutdownJVM brutally closed the JVM and bypassed all for the JVM shutdown routines
thus causing the program to skip over errors in the JPype module resulting
from mishandled race conditions.   While it is still acceptable to shutdown the
JVM and may be desireable to do so if a module needs a particular order to shutdown
cleanly, the use of an explicit shutdown is discouraged.


.. _controlling_the_jvm_debugging_jvm_shutdown:

Debugging JVM Shutdown
----------------------

If the JVM shutdown process hangs or fails, it is often due to lingering threads
or resources that were not properly terminated. Use the following techniques to
debug shutdown issues:

1. **Check Active Threads**: Before shutting down the JVM, check for active
   non-daemon threads that may be preventing the shutdown. You can use the
   following Java code to list all active threads:

   .. code-block:: python

       import java.lang.Thread

       # Get all active threads
       threads = java.lang.Thread.getAllStackTraces().keySet()
       for thread in threads:
           print(f"Thread: {thread.getName()}, Daemon: {thread.isDaemon()}")

   Ensure that all non-daemon threads are terminated or marked as daemon before
   calling `jpype.shutdownJVM()`.

2. **Inspect Shutdown Hooks**: If you have attached shutdown hooks, verify that
   they complete quickly and do not hang. Long-running shutdown hooks can delay
   or block JVM termination.

3. **Monitor Resource Usage**: If shared resources such as buffers are in use,
   ensure that they are properly released before shutting down the JVM. For
   example, copy buffer contents to a Python object to preserve data.

4. **Enable Debugging Logs**: JPype can provide additional diagnostics during
   the shutdown process. Use the following command to enable debugging logs:

   .. code-block:: python

       import _jpype
       _jpype.enableStacktraces(True)

   This will print detailed stack traces for exceptions that occur during the
   shutdown process.

5. **Handle Hanging Threads**: If the JVM shutdown hangs due to threads that
   cannot terminate, you can forcefully terminate the Python process using
   `os._exit()` or `java.lang.Runtime.exit()`. **However, note that calling
   `exit` will bypass normal `atexit` routines in both Python and Java.** This
   means that any cleanup tasks, such as writing logs (e.g., Jacoco coverage
   reports) or flushing buffers, will not be executed. Use this approach only
   as a last resort when all other debugging techniques fail.

.. _controlling_the_jvm_best_practices_for_jvm_shutdown:

Best Practices for JVM Shutdown
-------------------------------

- **Avoid Manual Shutdown**: Whenever possible, allow the JVM to shut down
  automatically when the Python process exits. This avoids the risks of lingering
  references and shared resource instability.

- **Terminate Threads Properly**: Ensure all non-daemon Java threads are
  terminated or marked as daemon before shutting down the JVM. Failure to do so
  may cause the shutdown process to hang indefinitely.

- **Handle Buffers Carefully**: If you are using shared buffers (e.g., Java
  direct buffers with NumPy), avoid accessing them after the JVM is shut down.
  If you need to preserve data, copy the buffer contents to a Python object
  before shutting down the JVM.

- **Use Shutdown Hooks**: Attach shutdown hooks only when necessary to clean up
  resources. Ensure that the hooks complete quickly to avoid delaying JVM
  termination.

- **Avoid Forceful Termination**: Avoid using `os._exit()` or `java.lang.Runtime.exit()`
  unless absolutely necessary. These methods prevent normal cleanup routines from
  executing, which can result in missing logs, incomplete resource cleanup, or
  other unintended consequences.


.. _controlling_the_jvm_summary_of_jvm_shutdown:

Summary of JVM Shutdown
------------------------

JPype's shutdown process is designed to ensure that resources are cleaned up
properly and the JVM terminates gracefully. While shutting down the JVM manually
is possible, it introduces risks that can lead to instability and crashes. For
most applications, the JVM should be allowed to shut down automatically when the
Python process exits. If manual shutdown is required, take precautions to ensure
that all Java references and shared resources are properly cleaned up before
shutting down the JVM. Avoid forceful termination unless absolutely necessary,
as it bypasses critical cleanup routines in both Python and Java.



.. _customization:

Customization
*************

JPype supports customization to enhance the integration between Java and Python.
This allows users to modify Java classes and type conversions to better suit
their needs, making Java APIs more Pythonic or enabling seamless interaction
with Python data structures.

There are two primary types of customizations available:

1. **Class Customizers**: Add Python methods and properties to Java classes to
   make them behave like native Python classes.
2. **Type Conversion Customizers**: Define implicit conversions between Python
   types and Java types for seamless interoperability.

.. _customization_class_customizers:

Class Customizers
=================

Customizers are applied to JPype wrapper classes to enhance their Pythonic
interface. By adding Python methods and properties to Java classes, customizers
make Java objects behave like native Python objects. These customizations are
applied to wrappers, whether they encapsulate a proxy or a Java reference.

Java wrappers can be customized to better match the expected behavior in Python.
Customizers are defined using decorators. Applying the annotations
``@JImplementationFor`` and ``@JOverride`` to a regular Python class will
transfer methods and properties to a Java class.

``@JImplementationFor`` requires the class name as a string, a Java class
wrapper, or a Java class instance. Only a string can be used prior to starting
the JVM. ``@JOverride``, when applied to a Python method, will hide the Java
implementation, allowing the Python method to replace the Java implementation.
When a Java method is overridden, it is renamed with a preceding underscore to
appear as a private method. Optional arguments to ``@JOverride`` can be used to
control the renaming and force the method override to apply to all classes that
derive from a base class ("sticky").

Generally speaking, a customizer should be defined before the first instance of
a given class is created so that the class wrapper and all instances will have
the customization.

.. _customization_example_customizing_javautilmap:

Example: Customizing ``java.util.Map``
--------------------------------------

The following example demonstrates how to customize the ``java.util.Map`` class
to behave like a Python dictionary:

.. code-block:: python

   @_jcustomizer.JImplementationFor('java.util.Map')
   class _JMap:
       def __jclass_init__(self):
           Mapping.register(self)

       def __len__(self):
           return self.size()

       def __iter__(self):
           return self.keySet().iterator()

       def __delitem__(self, i):
           return self.remove(i)

The name of the class does not matter for the purposes of the customizer,
though it should be a private class so that it does not get used accidentally.
The customizer code will steal from the prototype class rather than acting as a
base class, ensuring that the methods will appear on the most derived Python
class and are not hidden by the Java implementations.

The customizer copies methods, callable objects, ``__new__``, class member
strings, and properties.


.. _customization_type_conversion_customizers:

Type Conversion Customizers
===========================

JPype allows users to define custom conversion methods that are called whenever
a specified Python type is passed to a particular Java type. To specify a
conversion method, add ``@JConversion`` to an ordinary Python function with the
name of the Java class to be converted to and one keyword of ``exact`` or
``instanceof``. The keyword controls how strictly the conversion will be
applied:

- ``exact``: Restricted to Python objects whose type exactly matches the
  specified type.
- ``instanceof``: Accepts anything that matches ``isinstance`` to the specified
  type or protocol.

In some cases, the existing protocol definition will be overly broad. Adding
the keyword argument ``excludes`` with a type or tuple of types can be used to
prevent the conversion from being applied. Exclusions always apply first.

User-supplied conversions are tested after all internal conversions have been
exhausted and are always considered to be an implicit conversion.


.. _customization_example_converting_python_sequences_to_java_collections:

Example: Converting Python Sequences to Java Collections
--------------------------------------------------------

The following example demonstrates how to convert Python sequences into Java
collections:

.. code-block:: python

   @JConversion("java.util.Collection", instanceof=Sequence,
                             excludes=str)
   def _JSequenceConvert(jcls, obj):
       return _jclass.JClass('java.util.Arrays').asList(obj)

JPype supplies customizers for certain Python classes by default. These include:

========================= ==============================
Python class              Implicit Java Class
========================= ==============================
pathlib.Path              java.io.File
pathlib.Path              java.nio.file.Path
datetime.datetime         java.time.Instant
collections.abc.Sequence  java.util.Collection
collections.abc.Mapping   java.util.Map
========================= ==============================


.. _customization_jpype_beans_module:

JPype Beans Module
==================

.. _customization_overview_of_jpype_beans:

Overview of JPype Beans
-----------------------

The `jpype.beans` module is an optional feature that converts Java Bean-style
getter and setter methods into Python properties. This customization is
particularly useful for interactive programming or when working with Java
classes that follow the Bean pattern.

However, this behavior is not enabled by default because it can lead to
confusion about whether a class is exposing a variable or a property added by
JPype. Additionally, it violates Python's principle of *"There should be one--
and preferably only one --obvious way to do it."* and the C++ principle of
*"You only pay for what you use."*

If you find this feature useful, you can enable it explicitly by importing the
`jpype.beans` module.

.. _customization_enabling_beans_as_properties:

Enabling Beans as Properties
----------------------------

To enable the `jpype.beans` module, simply import it into your Python program:

.. code-block:: python

  import jpype.beans

Once enabled, the module applies globally to all Java classes that have already
been loaded, as well as any classes loaded afterward. This behavior cannot be
undone after the module is imported.

.. _customization_how_it_jpype_beans_works:

How It JPype beans Works
------------------------

The `jpype.beans` module scans Java classes for methods that follow the Bean
naming conventions:

- **Getter methods**: Methods prefixed with `get` (e.g., `getName`) are treated
  as property accessors.
- **Setter methods**: Methods prefixed with `set` (e.g., `setName`) are treated
  as property mutators.

For example, a Java class with the following methods:

.. code-block:: java

  public class Person {
      private String name;

      public String getName() {
          return name;
      }

      public void setName(String name) {
          this.name = name;
      }
  }

Will automatically expose the `name` field as a Python property:

.. code-block:: python

  import jpype
  import jpype.beans

  jpype.startJVM()

  Person = jpype.JClass("Person")
  person = Person()
  person.name = "Alice"  # Calls setName("Alice")
  print(person.name)     # Calls getName(), Output: Alice

.. _customization_implementation_details_of_jpype_beans:

Implementation Details of JPype beans
-------------------------------------

The module works by:

1. Identifying getter and setter methods in Java classes using the
   `_isBeanAccessor()` and `_isBeanMutator()` methods.
2. Creating Python properties for these methods.
3. Adding the properties to the class dynamically.

The customization applies retroactively to all classes currently loaded and
globally to all future classes.

.. _customization_limitations_of_jpype_beans:

Limitations of JPype beans
--------------------------

1. **Global Behavior**: Once enabled, the customization applies to all Java
   classes globally. It cannot be undone.
2. **Confusion with Existing Members**: If a Java class already has a Python
   member with the same name as a property, the property will not be added to
   avoid conflicts.
3. **Ambiguity**: This feature can make it unclear whether a field is a true
   Java variable or a property added by JPype.

.. _customization_best_practices_for_jpype_beans:

Best Practices for JPype beans
------------------------------

- Use this module only when working with Java classes that heavily rely on the
  Bean pattern.
- Avoid enabling this module in large projects unless absolutely necessary, as
  the global behavior may lead to unintended consequences.
- Document its usage clearly in your codebase to avoid confusion for other
  developers.

.. _customization_summary_of_jpype_beans:

Summary of JPype beans
----------------------

The `jpype.beans` module provides a convenient way to work with Java Bean-style
classes in Python by exposing getter and setter methods as Python properties.
While useful in certain scenarios, it is an optional feature that must be
explicitly enabled and should be used with caution due to its global and
irreversible behavior.

.. _customization_resolving_method_name_conflicts_with_customizers:

Resolving Method Name Conflicts with Customizers
================================================

.. _customization_overview_of_conflict_resolution:

Overview of conflict resolution
-------------------------------

When working with Java classes in Python, conflicts can arise between public
fields and methods that share the same name. JPype provides tools to resolve
these conflicts using customizers, allowing you to rename fields or methods
dynamically and expose them in a Pythonic way.

This section demonstrates how to use a customizer to resolve such conflicts by
renaming fields or methods and exposing them as Python properties.

.. _customization_example_renaming_conflicting_fields_and_methods:

Example: Renaming Conflicting Fields and Methods
------------------------------------------------

Consider a Java class with a field and a method that share the same name.
Without customization, JPype will expose the method, and the field will be
hidden. To resolve this, you can use a customizer to rename the conflicting
field or method and expose it as a Python property.

Here’s an example:

.. code-block:: python

    def asProperty(field):
        def get(E):
            return field.get(E)
        def set(E, V):
            field.set(E, V)
        return property(get, set)

    @jpype.JImplementationFor("java.lang.Object")  # Use your base class.
    class MyCustomizer(object):

        # This is applied to every class that derives from the type
        def __jclass_init__(cls):
            # Traverse the fields
            for field in cls.class_.getDeclaredFields():
                name = str(field.getName())
                tp = type(cls.__dict__.get(str(field.getName()), None))

                # Watch for private methods
                if tp is type(None):
                    continue

                # Resolve conflicts between public fields and methods
                if tp is jpype.JMethod:
                    cls._customize("%s_" % name, asProperty(field))

.. _customization_how_it_conflict_resolution_works:

How It Conflict Resolution Works
--------------------------------

1. **Field Traversal**: The customizer iterates over all declared fields in the
   class using `getDeclaredFields()`.
2. **Conflict Detection**: For each field, it checks whether a public method
   with the same name exists.
3. **Renaming**: If a conflict is detected, the field is renamed by appending
   an underscore (`_`) to its name.
4. **Property Creation**: The renamed field is exposed as a Python property
   using the `property()` function.

.. _customization_example_usage_of_conflict_resolution:

Example Usage of Conflict Resolution
------------------------------------

Suppose you have a Java class `A` with a field `mean` and a method `mean`.
Without customization, the field would be inaccessible. Using the customizer
above, you can expose the field as `mean_`:

.. code-block:: python

    A = jpype.JClass("A")
    a = A()
    print(a.mean_)  # Access the renamed field
    a.mean_ = 2      # Modify the field
    print(a.mean_)   # Verify the updated value

.. _customization_notes_on_global_customizers:

Notes on Global Customizers
---------------------------

- The customizer is applied globally to all classes that derive from the
  specified base class (`java.lang.Object` in this example). You can replace
  the base class with a more specific class to limit the scope of the
  customization.
- This approach is particularly useful for resolving conflicts in large Java
  libraries or frameworks where method and field names overlap frequently.

.. _customization_best_practices_regarding_name_resolution_customizers:

Best Practices Regarding Name Resolution Customizers
----------------------------------------------------

- Use meaningful naming conventions when renaming fields or methods to avoid
  confusion.
- Document customizations clearly in your codebase to help other developers
  understand the changes.
- Test the customizer thoroughly to ensure it behaves as expected across all
  relevant classes.

.. _customization_summary_of_naming_conflict_resolution:

Summary of Naming Conflict Resolution
-------------------------------------

This example demonstrates how to use JPype customizers to resolve conflicts
between fields and methods in Java classes. By renaming conflicting fields or
methods and exposing them as Python properties, you can create a more Pythonic
interface for interacting with Java classes.


.. _customization_best_practices_for_class_customization:

Best Practices For Class Customization
======================================

To ensure effective use of customizations, follow these best practices:

1. **Define Customizers Early**: Always define customizers before the first
   instance of the class is created to ensure proper initialization.

2. **Test Customizations Thoroughly**: Verify that the customized behavior
   works as expected, especially for complex or heavily-used classes.

3. **Avoid Conflicts**: Ensure that customizers do not introduce conflicting
   methods or properties, especially when customizing multiple interfaces.

4. **Monitor Performance**: Be mindful of performance implications when adding
   extensive customizations.

5. **Document Customizations**: Clearly document the purpose and behavior of
   customizations to assist other developers working on the codebase.

By leveraging class and type conversion customizers, JPype users can create
seamless integrations between Python and Java, making Java APIs feel native to
Python programmers.


.. _collections:

Collections
***********

JPype uses customizers to augment Java collection classes to operate like Python
collections. Enhanced objects include ``java.util.List``, ``java.util.Set``,
``java.util.Map``, and ``java.util.Iterator``. These classes generally comply
with the Python API except in cases where there is a significant name conflict.
This section details the integration of Java collections with Python constructs.

.. _collections_specialized_collection_wrappers:

Specialized Collection Wrappers
===============================

JPype customizes Java collection classes to behave like Python collections,
making them intuitive for Python developers. This includes support for iteration,
indexing, and key-value access. Below are the key behaviors of specific Java
collection types.

.. _collections_iterable:

Iterable
--------

Java classes that implement ``java.util.Iterable`` are customized to support
Python's iteration constructs. This allows seamless use in Python `for` loops
and list comprehensions. For example, a Java ``ArrayList`` can be iterated
directly:

.. code-block:: python

    from java.util import ArrayList

    jlist = ArrayList()
    jlist.add("apple")
    jlist.add("orange")
    jlist.add("banana")

    for item in jlist:
        print(item)

This integration ensures that Java collections behave like Python sequences,
providing a natural experience for Python developers.

.. _collections_iterators:

Iterators
---------

Java classes that implement ``java.util.Iterator`` act as Python iterators.
This means they can be used in Python `for` loops and list comprehensions
without requiring additional conversion. For example:

.. code-block:: python

    from java.util import Vector

    jvector = Vector()
    jvector.add("apple")
    jvector.add("orange")

    iterator = jvector.iterator()
    for item in iterator:
        print(item)

.. _collections_collection:

Collection
----------

Java classes that inherit from ``java.util.Collection`` integrate seamlessly
with Python's collection constructs. They support operations such as length
retrieval, iteration, and implicit conversion of Python sequences into Java
collections. For example:

.. code-block:: python

    from java.util import ArrayList

    pylist = ["apple", "orange", "banana"]
    jlist = ArrayList(pylist)  # Convert Python list to Java collection

    print(len(jlist))  # Output: 3
    for item in jlist:
        print(item)

Methods that accept Java collections can automatically convert Python sequences
if all elements are compatible with Java types. Otherwise, a ``TypeError`` is
raised.

.. _java.util.List:

Lists
-----

Java `List` classes, such as ``ArrayList`` and ``LinkedList``, can be used in
Python `for` loops and list comprehensions. They also support indexing and
deletion, making them behave like Python lists. For example:

.. code-block:: python

    from java.util import ArrayList

    jlist = ArrayList()
    jlist.add("apple")
    jlist.add("orange")
    jlist.add("banana")

    print(jlist[0])  # Output: apple
    del jlist[1]     # Remove "orange"
    print(jlist)     # Output: [apple, banana]

Java lists can also be converted to Python lists and vice versa using the copy
constructor. For example:

.. code-block:: python

    pylist = ["apple", "orange", "banana"]
    jlist = ArrayList(pylist)  # Convert Python list to Java list
    pylist2 = list(jlist)      # Convert Java list back to Python list

Note that individual elements remain Java objects when converted to Python.
Converting to Java will attempt to convert each argument
individually to Java.  If there is no conversion it will produce a
``TypeError``.  The conversion can be forced by casting to the appropriate
Java type with a list comprehension or by defining a new conversion
customizer.

Lists also have iterable, length, item deletion, and indexing.  Note that
indexing of ``java.util.LinkedList`` is supported but can have a large
performance penalty for large lists.  Use of iteration is much for efficient.


.. _java.util.Map:

Maps
----

Java classes that implement ``java.util.Map`` behave like Python dictionaries.
They support key-value access, iteration, and deletion. 
Here is a summary of their capabilities:

=========================== ================================
Action                       Python
=========================== ================================
Place a value in the map     ``jmap[key]=value``
Delete an entry              ``del jmap[key]``
Get the length               ``len(jmap)``
Lookup the value             ``v=jmap[key]``
Get the entries              ``jmap.items()``
Fetch the keys               ``jmap.key()``
Check for a key              ``key in jmap``
=========================== ================================

Example using Java HashMap with Pythonic interface:

.. code-block:: python

    from java.util import HashMap

    jmap = HashMap()
    jmap.put("key1", "value1")
    jmap.put("key2", "value2")

    print(jmap["key1"])  # Output: value1
    del jmap["key2"]     # Remove "key2"
    print(len(jmap))     # Output: 1

Maps also support iteration over keys and values:

.. code-block:: python

    for key, value in jmap.items():
        print(f"{key}: {value}")

Methods that accept Java maps can implicitly convert Python dictionaries if
all keys and values are compatible with Java types. Otherwise, a ``TypeError``
is raised.

.. _collections_map_entries:

Map Entries
-----------

Java map entries unpack into key-value pairs, allowing easy iteration in Python
loops. For example:

.. code-block:: python

    for key, value in jmap.items():
        print(f"{key}: {value}")

.. _collections_sets:

Sets
----

Java classes that implement ``java.util.Set`` behave like Python sets. They
support operations such as item deletion, iteration, and length retrieval. For
example:

.. code-block:: python

    from java.util import HashSet

    jset = HashSet()
    jset.add("apple")
    jset.add("orange")

    print(len(jset))  # Output: 2
    jset.remove("orange")
    print(jset)       # Output: [apple]

.. _collections_enumeration:

Enumeration
-----------

Java classes that implement ``java.util.Enumeration`` act as Python iterators.
This allows them to be used in Python `for` loops and list comprehensions. For
example:

.. code-block:: python

    from java.util import Vector

    jvector = Vector()
    jvector.add("apple")
    jvector.add("orange")

    enumeration = jvector.elements()
    for item in enumeration:
        print(item)


.. _collections_integrating_pythonic_constructs_with_java_collections:

Integrating Pythonic Constructs with Java Collections
======================================================

JPype enables Python developers to interact with Java collections and streams
while leveraging Python's idiomatic constructs, such as list comprehensions and
generator expressions. This section explores how Pythonic constructs and Java
methods can be used interchangeably or combined for efficient manipulation of
data structures.



.. _collections_using_pythonic_constructs_with_java_collections:

Using Pythonic Constructs with Java Collections
------------------------------------------------
JPype enables Python developers to interact with Java collections while
leveraging Python's idiomatic constructs, such as list comprehensions and
generator expressions. For example:

.. code-block:: python

    from java.util import ArrayList

    jlist = ArrayList()
    jlist.add("apple")
    jlist.add("orange")
    jlist.add("banana")

    filtered = [item.upper() for item in jlist if item.startswith("a")]
    print(filtered)  # Output: ['APPLE']

Combining Pythonic constructs with Java methods allows developers to use the
best tools for the task, whether they prefer Python's simplicity or Java's
robustness.


.. _collections_using_java_streams_for_functional_operations:

Using Java Streams for Functional Operations
---------------------------------------------

Java's `Stream` API provides powerful functional programming constructs, such
as `filter`, `map`, and `reduce`. JPype allows Python developers to use these
methods with Java collections, enabling them to leverage Java's robust
libraries.



.. _collections_example_filtering_and_mapping_with_java_streams:

Example: Filtering and Mapping with Java Streams
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from java.util.stream import Collectors

   # Use Java Stream API for filtering and mapping
   filtered = jlist.stream().filter(lambda s: s.startswith("a")).map(
       lambda s: s.upper()).collect(Collectors.toList())
   print(filtered)  # Output: [APPLE]

Advantages of Java Streams:
- Integration with Java's enterprise libraries.
- Parallel processing capabilities (e.g., `.parallelStream()`).
- Type-safe operations with Java's generics.



.. _collections_comparison_pythonic_constructs_vs_java_methods:

Comparison: Pythonic Constructs vs Java Methods
------------------------------------------------

+---------------------------+---------------------------------------+
| **Feature**               | **Pythonic Constructs**               |
|                           | **(List Comprehensions)**             |
+---------------------------+---------------------------------------+
| Syntax                    | Concise and readable                  |
+---------------------------+---------------------------------------+
| Performance               | Python interpreter overhead           |
+---------------------------+---------------------------------------+
| Parallel Processing       | Requires external libraries           |
|                           | (e.g., `multiprocessing`)             |
+---------------------------+---------------------------------------+
| Type Safety               | Dynamic typing                        |
+---------------------------+---------------------------------------+
| Ease of Use               | Familiar to Python developers         |
+---------------------------+---------------------------------------+


.. _collections_combining_pythonic_constructs_and_java_methods:

Combining Pythonic Constructs and Java Methods
----------------------------------------------

JPype allows developers to mix Pythonic constructs and Java methods for maximum
flexibility. For example, you can use Java streams for complex operations and
Pythonic constructs for post-processing.


.. _collections_example_combining_streams_and_list_comprehensions:

Example: Combining Streams and List Comprehensions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   # Use Java Stream API for filtering
   filtered_stream = jlist.stream().filter(lambda s: s.startswith("a")).collect(
       Collectors.toList())

   # Use Pythonic list comprehension for further processing
   final_result = [item.lower() for item in filtered_stream]
   print(final_result)  # Output: ['apple']


.. _collections_when_to_use_each_approach:

When to Use Each Approach
-------------------------

+-------------------------------------------+---------------------------+
| **Scenario**                              | **Recommended Approach**  |
+-------------------------------------------+---------------------------+
| Simple filtering or mapping               | Pythonic constructs       |
|                                           | (list comprehensions)     |
+-------------------------------------------+---------------------------+
| Complex operations (e.g., grouping,       | Java Streams              |
| reducing)                                 |                           |
+-------------------------------------------+---------------------------+
| Integration with Java enterprise          | Java Streams              |
| libraries                                 |                           |
+-------------------------------------------+---------------------------+
| Quick prototyping or debugging            | Pythonic constructs       |
+-------------------------------------------+---------------------------+
| Parallel processing                       | Java Streams              |
|                                           | (`parallelStream`)        |
+-------------------------------------------+---------------------------+



.. _collections_best_practices_for_collection_processing:

Best Practices for Collection Processing
----------------------------------------

1. **Choose the Right Tool for the Job**:
   - Use Pythonic constructs for simplicity and readability.
   - Use Java streams for performance-critical or enterprise applications.

2. **Leverage JPype's Seamlessness**:
   - Combine Pythonic constructs and Java methods to get the best of both worlds.

3. **Optimize for Performance**:
   - Avoid frequent back-and-forth calls between Python and Java. Cache results when possible.



.. _collections_conclusion_on_collection_processing:

Conclusion on Collection Processing
-----------------------------------

JPype enables Python developers to work with Java collections using both
Pythonic constructs and Java methods. Whether you prefer Python's simplicity or
Java's robustness, JPype provides the flexibility to choose the paradigm that
best fits your workflow.


.. _serialization_with_jpickler:

Serialization with JPickler
***************************

JPype provides the **JPickler** utility for serializing (`pickling`) Java objects
into Python-compatible byte streams. This is particularly useful for saving Java
objects to disk, transferring them between systems, or debugging their state.



.. _serialization_with_jpickler_why_use_jpickler:

Why Use JPickler?
=================

When working with Java objects in Python, serialization is often required for:

1. **Persistence**: Saving Java objects to files for later use.
2. **Data Exchange**: Transferring Java objects between Python applications or
   systems.
3. **Debugging**: Capturing the state of Java objects during execution for
   offline analysis.

However, Python's default `pickle` module does not support Java objects.
JPickler bridges this gap by encoding Java objects into a format compatible
with Python's serialization tools.



.. _serialization_with_jpickler_how_jpickler_works:

How JPickler Works
------------------

JPickler uses Java's `Serializable` interface to serialize Java objects into a
byte stream that can be stored or transferred. It also provides a companion
utility, **JUnpickler**, for deserializing these byte streams back into Java
objects.



.. _serialization_with_jpickler_example_1_basic_serialization:

Example 1: Basic Serialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following example demonstrates how to serialize and deserialize Java
objects using JPickler and JUnpickler.

.. code-block:: python

    import jpype
    import jpype.imports
    from jpype.pickle import JPickler, JUnpickler

    # Start the JVM
    jpype.startJVM()

    # Create a Java object
    java_list = jpype.java.util.ArrayList()
    java_list.add("Hello")
    java_list.add("World")

    # Serialize the Java object to a file
    with open("serialized_java_list.pkl", "wb") as f:
        JPickler(f).dump(java_list)

    print("Java object serialized successfully!")

    # Deserialize the Java object from the file
    with open("serialized_java_list.pkl", "rb") as f:
        deserialized_list = JUnpickler(f).load()

    print("Deserialized Java object:", deserialized_list)
    # Output: [Hello, World]



.. _serialization_with_jpickler_example_2_serializing_complex_java_objects:

Example 2: Serializing Complex Java Objects
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

JPickler can handle any Java object that implements `java.io.Serializable`.
Here's an example with a custom Java class:

.. code-block:: java

    // Save this as MySerializableClass.java and compile it
    import java.io.Serializable;

    public class MySerializableClass implements Serializable {
        private String name;
        private int value;

        public MySerializableClass(String name, int value) {
            this.name = name;
            this.value = value;
        }

        @Override
        public String toString() {
            return "MySerializableClass{name='" + name + "', value=" + value + "}";
        }
    }

.. code-block:: python

    import jpype
    import jpype.imports
    from jpype.pickle import JPickler, JUnpickler

    # Start the JVM
    jpype.startJVM(classpath=["."])

    # Create an instance of the custom Java class
    MySerializableClass = jpype.JClass("MySerializableClass")
    java_object = MySerializableClass("TestObject", 42)

    # Serialize the Java object to a file
    with open("serialized_java_object.pkl", "wb") as f:
        JPickler(f).dump(java_object)

    print("Custom Java object serialized successfully!")

    # Deserialize the Java object from the file
    with open("serialized_java_object.pkl", "rb") as f:
        deserialized_object = JUnpickler(f).load()

    print("Deserialized Java object:", deserialized_object)
    # Output: MySerializableClass{name='TestObject', value=42}



.. _serialization_with_jpickler_best_practices_with_jpicker:

Best Practices with JPicker
===========================

1. **Ensure Objects Are Serializable**:

   - Only Java objects that implement `java.io.Serializable` can be serialized.
     Ensure your custom Java classes implement this interface.

2. **Validate Serialization**:

   - Test serialization and deserialization to ensure data integrity.

3. **Handle Non-Serializable Fields**:

   - If a Java object contains non-serializable fields, mark them as `transient`
     to exclude them during serialization.

4. **Avoid Reference Loops**:

   - Break reference loops between Python and Java objects to prevent memory
     leaks.


.. _serialization_with_jpickler_limitations_of_jpickler:

Limitations of JPickler
=======================

1. **Non-Serializable Objects**:

   - Java objects that do not implement `java.io.Serializable` cannot be
     serialized with JPickler.

2. **Cross-Version Compatibility**:

   - Serialized Java objects may not be compatible across different JVM
     versions.

3. **Performance**:

   - Serialization and deserialization can be resource-intensive for large or
     complex objects.


.. _serialization_with_jpickler_use_cases_of_jpickler:

Use Cases of JPickler
=====================

1. **Persistence**:

   - Save Java objects to disk for later use.

   - Example: Storing application state or configuration.

2. **Data Exchange**:

   - Transfer Java objects between Python applications or systems.

   - Example: Network communication or distributed systems.

3. **Debugging**:

   - Capture the state of Java objects during execution for offline analysis.

   - Example: Serialize problematic objects for inspection after a crash.


.. _serialization_with_jpickler_conclusion_for_jpicker:

Conclusion for JPicker
======================

JPickler simplifies serialization of Java objects in Python, enabling seamless
integration between the two ecosystems. By following best practices and
understanding its limitations, you can use JPickler effectively for
persistence, data exchange, and debugging tasks.


.. _working_with_numpy:

Working with NumPy
******************

JPype provides seamless integration between Python's NumPy library and Java,
enabling efficient data exchange and manipulation across both ecosystems. By
leveraging JPype's ability to transfer arrays bidirectionally, users can combine
NumPy's powerful numerical computing capabilities with Java's robust libraries
for machine learning, scientific computing, and enterprise applications. Whether
transferring data to NumPy for analysis or sending arrays to Java for processing,
JPype ensures high performance and compatibility with minimal overhead. This
integration is particularly useful for applications requiring large-scale
numerical computations or interoperability between Python and Java-based systems.


.. _working_with_numpy_transferring_arrays_between_python_and_java:

Transferring Arrays Between Python and Java
===========================================

JPype supports bidirectional transfers of arrays between Python (NumPy) and Java.
This allows seamless integration of numerical libraries with Java's ecosystem.



.. _working_with_numpy_transferring_arrays_to_numpy:

Transferring Arrays to NumPy
----------------------------

Java arrays can be transferred into NumPy arrays using Python's `memoryview`.
This enables efficient bulk data transfer for rectangular arrays.

**Example: Transferring a Java Array to NumPy**

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a Java array
   java_array = jpype.JDouble[:]([1.1, 2.2, 3.3])

   # Transfer the Java array to NumPy
   numpy_array = np.array(memoryview(java_array))

   print(numpy_array)  # Output: [1.1 2.2 3.3]

**Constraints**:
- The Java array must be rectangular. Jagged arrays are not supported.
- Only primitive types (e.g., `double`, `int`) are supported for direct transfer.



.. _working_with_numpy_transferring_arrays_to_java:

Transferring Arrays to Java
---------------------------

NumPy arrays can be transferred to Java using the `JArray.of` function. This maps
the structure of a NumPy array to a Java multidimensional array.

**Example: Transferring a NumPy Array to Java**

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a NumPy array
   numpy_array = np.zeros((5, 10, 20))  # 5x10x20 array filled with zeros

   # Transfer the array to Java
   java_array = jpype.JArray.of(numpy_array)

   print(java_array[0][0][0])  # Output: 0.0

**Constraints**:
- The NumPy array must be rectangular. Jagged arrays are not supported.
- Data types must be compatible with Java primitives (e.g., `np.float64` → `double`).



.. _working_with_numpy_requirements_and_constraints:

Requirements and Constraints
----------------------------

1. **Rectangular Arrays**:

   - Both NumPy and Java arrays must be rectangular for direct transfer.

2. **Data Type Compatibility**:

   - NumPy types must map to Java primitives (e.g., `np.int32` → `int`).

3. **Error Handling**:

   - Jagged arrays or incompatible types will raise a `TypeError`.



.. _working_with_numpy_best_practices_with_numpy:

Best Practices with NumPy
-------------------------

1. **Validate Array Structure**:
   - Ensure arrays are rectangular before transferring.

2. **Optimize Data Types**:
   - Use NumPy types that map directly to Java primitives for efficiency.

3. **Monitor Memory Usage**:
   - Large arrays can consume significant memory. Monitor resources carefully.



.. _working_with_numpy_summary_of_numpy:

Summary of NumPy
----------------

JPype provides efficient bidirectional array transfers between Python and Java.
By following the outlined constraints and best practices, users can achieve
seamless integration for numerical and scientific applications.


.. _working_with_numpy_buffer_backed_numpy_arrays:

Buffer Backed NumPy Arrays
==========================

Java direct buffers provide a mechanism for shared memory between Java and
Python, enabling high-speed data exchange by bypassing the JNI layer. These
buffers are particularly useful for applications requiring efficient handling
of large datasets, such as scientific computing or memory-mapped files.

Direct buffers are part of the Java ``nio`` package and can be accessed using
the ``jpype.nio`` module. NumPy arrays can be backed by Java direct buffers,
allowing Python and Java to operate on the same memory space. However, direct
buffers are not managed by the garbage collector, so improper use may lead to
memory leaks or crashes.


.. _working_with_numpy_creating_buffer_backed_arrays:

Creating Buffer Backed Arrays
-----------------------------

To create a buffer-backed NumPy array, you can either originate the buffer in
Java or Python. The following examples demonstrate both approaches:

**Example 1: Creating a Buffer in Java**

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Allocate a direct buffer in Java
   jb = java.nio.ByteBuffer.allocateDirect(80)  # Allocates 80 bytes
   db = jb.asDoubleBuffer()                     # Converts to a double buffer

   # Convert the buffer to a NumPy array
   np_array = np.asarray(db)                    # NumPy array backed by Java buffer
   print(np_array)

**Example 2: Creating a Buffer in Python**

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a Python bytearray
   py_buffer = bytearray(80)                    # Allocates 80 bytes
   jb = jpype.nio.convertToDirectBuffer(py_buffer)  # Maps the bytearray to Java
   db = jb.asDoubleBuffer()                     # Converts to a double buffer

   # Convert the buffer to a NumPy array
   np_array = np.asarray(db)                    # NumPy array backed by Python buffer
   print(np_array)


.. _working_with_numpy_important_considerations_for_buffer_backed_arrays:

Important Considerations for Buffer Backed Arrays
-------------------------------------------------

1. **Buffer Lifetime**:

   - Python and NumPy cannot detect when a Java buffer becomes invalid. Once the
     JVM is shut down, all buffers originating from Java become invalid, and any
     access to them may result in crashes.

   - To avoid this, create buffers in Python and pass them to Java, ensuring
     Python retains control over the memory.

2. **JVM Shutdown**:

   - If buffers are created in Java, consider using ``java.lang.Runtime.exit``
     to terminate both the Java and Python processes simultaneously. This
     prevents accidental access to dangling buffers.

3. **Applications**:

   - Buffer-backed memory is not limited to NumPy. It can be used for shared
     memory between processes, memory-mapped files, or any application requiring
     efficient data exchange.


.. _working_with_numpy_summary_of_buffer_backed_arrays:

Summary of Buffer Backed Arrays
-------------------------------

Buffer-backed NumPy arrays provide a powerful mechanism for high-speed data
exchange between Python and Java. However, users must carefully manage buffer
lifetimes and ensure proper handling during JVM shutdown to avoid crashes or
memory leaks.


.. _working_with_numpy_numpy_primitives:

NumPy Primitives
================

JPype provides seamless integration with NumPy, allowing efficient data
transfers between Python and Java. NumPy arrays can be mapped to Java boxed
types or primitive arrays. However, certain types, such as `np.float16`, are
converted to compatible Java types during transfer.

.. _working_with_numpy_supported_numpy_types:

Supported NumPy Types
---------------------

The following table summarizes how NumPy types are mapped to Java boxed types
and primitive arrays:

=================  ============================
NumPy Type         Java Type (Boxed/Primitive)
=================  ============================
np.int8            java.lang.Byte / byte[]
np.int16           java.lang.Short / short[]
np.int32           java.lang.Integer / int[]
np.int64           java.lang.Long / long[]
np.float16         java.lang.Float / float[] (*)
np.float32         java.lang.Float / float[]
np.float64         java.lang.Double / double[]
=================  ============================

(*) `np.float16` will be automatically converted to `float32` (`java.lang.Float`
or `float[]`) during the transfer to Java.

.. note::
   `np.float16` can be transferred to Java, but it will be automatically
   converted to `float32` (`java.lang.Float` for boxed types or `float[]` for
   primitive arrays) on the Java side. This is because Java does not natively
   support `float16`. If precise handling of `float16` is required, consider
   converting the data to `float32` or `float64` explicitly in Python before
   transferring it.

.. _working_with_numpy_examples:

Examples
--------

The following examples demonstrate how to transfer `np.float16` data to Java
as boxed types or primitive arrays.

.. _working_with_numpy_example_1_transferring_float16_to_a_boxed_type:

Example 1: Transferring `float16` to a Boxed Type
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a NumPy array with float16 data
   float16_array = np.array([1.1, 2.2, 3.3], dtype=np.float16)

   # Transfer the array to a Java boxed type (java.util.ArrayList)
   java_list = jpype.java.util.ArrayList()
   for value in float16_array:
       java_list.add(jpype.JFloat(value))  # Automatically converted to float32

   print(java_list)  # Output: [1.1, 2.2, 3.3] (as float32)

.. _working_with_numpy_example_2_transferring_float16_to_a_primitive_array:

Example 2: Transferring `float16` to a Primitive Array
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a NumPy array with float16 data
   float16_array = np.array([1.1, 2.2, 3.3], dtype=np.float16)

   # Transfer the array to a Java primitive array
   java_primitive_array = jpype.JArray(jpype.JFloat)(float16_array)  # Converted
                                                                     # to float32[]

   print(java_primitive_array)  # Output: [1.1, 2.2, 3.3] (as float32[])

.. _working_with_numpy_summary_of_numpy_automatic_converstions:

Summary of NumPy Automatic Converstions
---------------------------------------

JPype supports transferring NumPy arrays to Java, with automatic conversions
for certain types. While `np.float16` can be transferred, it is converted to
`float32` on the Java side for compatibility. Users should be aware of this
behavior and plan accordingly when working with `float16` data.


.. _Proxies:

Calling Python Code from Java
*****************************
Proxies in JPype enable Python objects to implement Java interfaces directly,
allowing seamless interaction between Python and Java. These proxies are
specifically designed to implement Java interfaces, acting as wrapper classes
that disguise the Python nature of the object in a Java type-safe manner.

In Java, proxies are foreign elements that pretend to implement a Java
interface. JPype leverages this proxy API to allow Python code to implement any
Java interface. While proxies allow Python objects to fulfill the contract of a
Java interface, they are not equivalent to subclassing Java classes in Python.

Fortunately, many Java APIs are designed to minimize the need for subclassing.
For example, frameworks like AWT and SWING allow developers to create complete
user interfaces without requiring a single subclass. Subclassing is typically
reserved for more advanced features or specialized use cases.

For those cases where sub-classing is absolutely necessary (i.e. using Java's
SAXP classes), it is necessary to create an interface and a simple
subclass in Java that delegates the calls to that interface.  The interface
can then be implemented in Python using a proxy.

There are three APIs for supporting of Java proxies. The direct method is to
pass a Python function, method, bound method, or lambda to any Java method
that accepts a FunctionInterface or other SAM.  If more complex behaviors
need to be exchanged Python can implement a Java interface. Implementation of an
interface uses decorators which features strong error checking and easy
notation.  The older low-level interface allows any Python object or dictionary
to act as a proxy even if it does not provide the required methods for the
interface.

.. _calling_python_code_from_java_passing_python_callables_to_java_functional_interfaces:

Passing Python Callables to Java Functional Interfaces
=======================================================

JPype supports passing Python functions, methods, and bound methods directly to
Java methods or fields that implement `FunctionalInterface`. This allows Python
code to seamlessly integrate with Java's functional programming constructs,
such as lambdas and method references, without requiring a proxy or explicit
implementation of the interface.

### Supported Use Cases

This feature works with any Java method or field that expects a
`FunctionalInterface`. Common examples include:
- Java Streams (`java.util.stream`)
- Java Executors (`java.util.concurrent`)
- Custom functional interfaces defined in Java code

### Example: Passing a Python Function to a Java Method

Suppose you have a Java method that expects a `java.util.function.Function`:

.. code-block:: java

    import java.util.function.Function;

    public class Example {
        public static String applyFunction(Function<String, String> func, String input) {
         return func.apply(input);
        }
    }

You can pass a Python function directly to this method:

.. code-block:: python

    import jpype import jpype.imports
    jpype.startJVM()

    from java.util.function import Function from Example import Example

    # Define a Python function
    def to_uppercase(s):
        return s.upper()

    # Pass the Python function to the Java method
    result = Example.applyFunction(to_uppercase, "hello")
    print(result)  #Output: HELLO

### Example: Using a Lambda Expression

Python lambdas can also be passed to Java methods:

.. code-block:: python

    # Pass a lambda expression
    result = Example.applyFunction(lambda s: s[::-1], "hello")
    print(result)  #Output: olleh

### Example: Using a Bound Method

Bound methods of Python objects can be passed as well:

.. code-block:: python

    class StringManipulator:
        def reverse(self, s):
            return s[::-1]

    manipulator = StringManipulator()
    result = Example.applyFunction(manipulator.reverse, "hello")
    print(result)  # Output: olleh

### Notes and Best Practices

1. **Performance**: While using Python callables is convenient, it may not be
as performant as implementing a full Java proxy for high-frequency calls. Use
proxies for performance-critical applications.

2. **Error Handling**: If an exception occurs within the Python callable, it
will be wrapped in a `RuntimeException` when passed back to Java.

3. **Type Matching**: Ensure that the Python callable returns a type compatible
with the expected Java return type. Implicit conversions will be applied where
possible.

By leveraging this feature, you can simplify integration between Python and
Java, especially when working with Java's functional programming APIs.


.. _@JImplements:

Implements
==========

The newer style of proxy works by decorating any ordinary Python class to
designate it as a proxy.  This is most effective when you
control the Python class definition.  If you don't control the class definition
you either need to encapsulate the Python object in another object or
use the older style.

Implementing a proxy is simple.  First construct an ordinary Python class with
method names that match the Java interface to be implemented.  Then add
the ``@JImplements`` decorator to the class definition.  The first argument
to the decorator is the interface to implement.  Then mark each
method corresponding to a Java method in the interface with ``@JOverride``.
When the proxy class is declared, the methods will be checked against the Java
interface.  Any missing method will result in JPype raising an exception.

High-level proxies have one other important behavior.  When a proxy created
using the high-level API returns from Java it unpacks back to the original
Python object complete with all of its attributes.  This occurs whether the
proxy is the ``self`` argument for a method or
proxy is returned from a Java container such as a list.  This is accomplished
because the actually proxy is a temporary Java object with no substance,
thus rather than returning a useless object, JPype unpacks the proxy
to its original Python object.

.. _calling_python_code_from_java_proxy_method_overloading:

Proxy Method Overloading
------------------------

Overloaded methods will issue to a single method with the matching name.  If
they take different numbers of arguments then it is best to implement a method
dispatch:

.. code-block:: python

    @JImplements(JavaInterface)
    class MyImpl:
        @JOverride
        def callOverloaded(self, *args):
            # always use the wild card args when implementing a dispatch
            if len(args)==2:
                return self.callMethod1(*args)
            if len(args)==1 and isinstance(args[0], JString):
                return self.callMethod2(*args)
            raise RuntimeError("Incorrect arguments")

       def callMethod1(self, a1, a2):
            # ...
       def callMethod2(self, jstr):
            # ...

.. _calling_python_code_from_java_multiple_interfaces:

Multiple interfaces
-------------------

Proxies can implement multiple interfaces as long as none of those interfaces
have conflicting methods.  To implement more than one interface, use a
list as the argument to the JImplements decorator.  Each interface must be
implemented completely.

.. _calling_python_code_from_java_deferred_realization:

Deferred realization
--------------------

Sometimes it is useful to implement proxies before the JVM is started.  To
achieve this, specify the interface using a string and add the keyword argument
``deferred`` with a value of ``True`` to the decorator.

.. code-block:: python

    @JImplements("org.foo.JavaInterface", deferred=True)
    class MyImpl:
        # ...


Deferred proxies are not checked at declaration time, but instead at the time
for the first usage.  Because of this, when uses an deferred proxy the code
must be able to handle initialization errors wherever the proxy is created.

Other than the raising of exceptions on creation, there is no penalty to
deferring a proxy class. The implementation is checked once on the first
usage and cached for the remaining life of the class.

.. _calling_python_code_from_java_proxy_factory:

Proxy Factory
=============

When a foreign object from another module for which you do not control the class
implementation needs to be passed into Java, the low level API is appropriate.
In this API you manually create a JProxy object.  The proxy object must either
be a Python object instance or a Python dictionary.  Low-level proxies use the
JProxy API.

.. _calling_python_code_from_java_jproxy:

JProxy
------

The ``JProxy`` allows Python code to "implement" any number of Java interfaces,
so as to receive callbacks through them.  The JProxy factory has the signature::

   JProxy(intr, [dict=obj | inst=obj] [, deferred=False])

The first argument is the interface to be implemented.  This may be either
a string with the name of the interface, a Java class, or a Java class instance.
If multiple interfaces are to be implemented the first argument is
replaced by a Python sequence.  The next argument is a keyword argument
specifying the object to receive methods.  This can either be a dictionary
``dict`` which names the methods as keys or an object instance ``inst`` which
will receive method calls.  If more than one option is selected, a ``TypeError``
is raised.  When Java calls the proxy the method is looked up in either
the dictionary or the instance and the resulting method is called.  Any
exceptions generated in the proxy will be wrapped as a ``RuntimeException``
in Java.  If that exception reaches back to Python it is unpacked to return
the original Python exception.

Assume a Java interface like:

.. code-block:: java

  public interface ITestInterface2
  {
          int testMethod();
          String testMethod2();
  }

You can create a proxy *implementing* this interface in two ways.
First, with an object:

.. code-block:: python

  class C:
      def testMethod(self):
          return 42

      def testMethod2(self):
          return "Bar"

  c = C()  # create an instance
  proxy = JProxy("ITestInterface2", inst=c)  # Convert it into a proxy

or you can use a dictionary.

.. code-block:: python

    def _testMethod():
        return 32

    def _testMethod2():
        return "Fooo!"

    d = { 'testMethod': _testMethod, 'testMethod2': _testMethod2, }
    proxy = JProxy("ITestInterface2", dict=d)

.. _calling_python_code_from_java_wrapping_a_python_instance_with_new_behaviors_for_java:

Wrapping a Python instance with new behaviors for Java
======================================================

JPype allows a JProxy to implement Java interfaces using a combination of a
dictionary (dict) and an object instance (inst). This feature enables arbitrary
Python objects to dynamically define methods via a dictionary while also
providing methods from the object's class. The combined approach is
particularly useful for cases where some methods are predefined in a Python
class and others need to be dynamically added or overridden.  This is useful when
the names and functionality of a Python object need to be made to conform to
Java's expected behaviors.

.. _calling_python_code_from_java_syntax:

Syntax
------
The JProxy factory supports both dict and inst as keyword arguments. When both are provided:

 * Methods in the dictionary take precedence.
 * The inst object is passed as the self argument to methods defined in the dictionary.
 * If a method is not found in the dictionary, JPype will fall back to the default method implementation in Java.

.. code-block:: python

    JProxy(interface, dict=my_dict, inst=my_instance)

Example: Combining dict and inst
Suppose you have a Java interface:

.. code-block:: java

    public interface MyInterface {
        String method1();
        String method2();
    }

You can implement this interface using both a dictionary and an object instance:

.. code-block:: python

    public interface MyInterface {
        String method1();
        default String method2() {
            return "hello";
        }
    }
    You can implement this interface using both a dictionary and an object instance:

    .. code-block:: python

    from jpype import JProxy

    class MyClass:
        def __init__(self, name):
            self.name = name

    # Define a dictionary with methods
    my_dict = {
        "method1": lambda self: f"Hello, {self.name} from method1"
    }

    # Create an instance of the class
    my_instance = MyClass("Alice")

    # Combine the dictionary and instance in a JProxy
    proxy = JProxy("MyInterface", dict=my_dict, inst=my_instance)

    # Use the proxy in Java
    print(proxy.method1())  # Output: Hello, Alice from method1
    print(proxy.method2())  # Falls back to Java's default method implementation


.. _calling_python_code_from_java_notes_and_best_practices:

Notes and Best Practices
------------------------
Method Resolution:

 * Methods in the dictionary take precedence over methods in the instance.

 * If a method is not found in the dictionary, JPype will attempt to resolve it in the
   instance.

Error Handling:

 * If neither the dictionary nor the instance provides the required method, a NotImplementedError will be raised.

Flexibility:

This approach allows dynamic addition or overriding of methods via the dictionary while retaining the benefits of object-oriented programming with the instance.

Example: Dynamic Overrides
You can dynamically override methods in the instance using the dictionary:

.. code-block:: python

    class MyClass:
        def __init__(self, name):
            self.name = name

    # Define a dictionary to override methods
    my_dict = {
        "method1": lambda self: f"Overridden method1 for {self.name}"
    }

    my_instance = MyClass("Bob")

    proxy = JProxy("MyInterface", dict=my_dict, inst=my_instance)

    print(proxy.method1())  # Output: Overridden method1 for Bob
    print(proxy.method2())  # Falls back to Java's default method implementation


.. _calling_python_code_from_java_proxying_python_objects:

Proxying Python objects
=======================

Sometimes it is necessary to push a Python object into Java memory space as an
opaque object.  This can be achieved using be implementing a proxy for
an interface which has no methods.  For example, ``java.io.Serializable`` has
no arguments and little functionality beyond declaring that an object can be
serialized. As low-level proxies to not automatically convert back to Python
upon returning to Java, the special keyword argument ``convert`` should be set
to True.

For example, let's place a generic Python object such as NumPy array into Java.

.. code-block:: python

    import numpy as np
    u = np.array([[1,2],[3,4]])
    ls = java.util.ArrayList()
    ls.add(jpype.JProxy(java.io.Serializable, inst=u, convert=True))
    u2 = ls.get(0)
    print(u is u2)  # True!

We get the expected result of ``True``.  The Python has passed through Java
unharmed.  In future versions of JPype, this method will be extended to provide
access to Python methods from within Java by implementing a Java interface that
points to back to Python objects.


.. _calling_python_code_from_java_reference_loops:

Reference Loops
===============

It is strongly recommended that object used in proxies must never hold a
reference to a Java container.  If a Java container is asked to hold a Python
object and the Python object holds a reference to the container, then a
reference loop is formed.  Both the Python and Java garbage collectors are
aware of reference loops within themselves and have appropriate handling for
them.  But the memory space of the other machine is opaque and neither Java nor
Python is aware of the reference loop.  Therefore, unless you manually break
the loop by either clearing the container, or removing the Java reference from
Python these objects can never be collected.  Once you lose the handle they
will both become immortal.

Ordinarily the proxy by itself would form a reference loop.  The Python
object points to a Java invocation handler and the invocation handler points
back to Python object to prevent the Python object from going away as long
as Java is holding onto the proxy.  This is resolved internally by making
the Python weak reference the Java portion.  If Java ever garbage collects
the Java half, it is recreated again when the proxy is next used.

This does have some consequences for the use of proxies.  Proxies must never
be used as synchronization objects.  Whenever
they are garbage collected, they lose their identity.  In addition, their
hashCode and system id both are reissued whenever they are refreshed.
Therefore, using a proxy as a Java map key can be problematic.  So long as
it remains in the Java map, it will maintain the same identify.  But once
it is removed, it is free to switch identities every time it is garbage
collected.

.. _awtswing:

AWT/Swing
*********

Java GUI elements can be used from Python.  To use Swing
elements the event loop in Java must be started from a user thread.
This will prevent the JVM from shutting down until the user thread
is completed.

Here is a simple example which creates a hello world frame and
launches it from within Python.

.. code-block:: python

    import jpype
    import jpype.imports

    jpype.startJVM()
    import java
    import javax
    from javax.swing import *

    def createAndShowGUI():
        frame = JFrame("HelloWorldSwing")
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE)
        label = JLabel("Hello World")
        frame.getContentPane().add(label)
        frame.pack()
        frame.setVisible(True)

    # Start an event loop thread to handling gui events
    @jpype.JImplements(java.lang.Runnable)
    class Launch:
        @jpype.JOverride
        def run(self):
            createAndShowGUI()
    javax.swing.SwingUtilities.invokeLater(Launch())



.. _concurrent_processing:

Concurrent Processing
*********************

This chapter covers the topic of threading, synchronization, and multiprocess.
Much of this material depends on the use of Proxies_ covered in the prior
chapter.

.. _concurrent_processing_threading:

Threading
=========

JPype supports all types of threading subject to the restrictions placed by
Python.  Java is inherently threaded and support a vast number of threading
styles such as execution pools, futures, and ordinary thread.  Python is
somewhat more limited.  At its heart Python is inherently single threaded
and requires a master lock known as the GIL (Global Interpreter Lock) to
be held every time a Python call is made.  Python threads are thus more
cooperative that Java threads.

To deal with this behavior, JPype releases the GIL every time it leaves from
Python into Java to any user defined method.  Shorter defined calls such as
to get a string name from from a class may not release the GIL.  Every time
the GIL is released it is another opportunity for Python to switch to a different
cooperative thread.

.. _concurrent_processing_threading_python_threads:

Python Threads
--------------

For the most part, Python threads based on OS level threads (i.e. POSIX
threads) will work without problem. The only challenge is how Java sees threads.
In order to operate on a Java method, the calling thread must be attached to
Java.  Failure to attach a thread will result in a segmentation fault.  It used
to be a requirement that users manually attach their thread to call a Java
function, but as the user has no control over the spawning of threads by other
applications such as an IDE, this inevitably lead to unexpected segmentation
faults.  Rather that crashing randomly, JPype automatically attachs any
thread that invokes a Java method.  These threads are attached automatically as
daemon threads so that will not prevent the JVM from shutting down properly
upon request.  If a thread must be attached as a non-daemon, use the method
``java.lang.Thread.attach()`` from within the thread context.  Once this is
done the JVM will not shut down until that thread is completed.

There is a function called ``java.lang.Thread.isAttached()`` which will check
if a thread is attached.  As threads automatically attach to Java, the only
way that a thread would not be attached is if it has never called a Java method.

The downside of automatic attachment is that each attachment allocates a
small amount of resources in the JVM.  For applications that spawn frequent
dynamically allocated threads, these threads will need to be detached prior
to completing the thread with ``java.lang.Thread.detach()``.  When
implementing dynamic threading, one can detach the thread
whenever Java is no longer needed.  The thread will automatically reattach if
Java is needed again.  There is a performance penalty each time a thread is
attached and detached.

.. _concurrent_processing_threading_java_threads:

Java Threads
------------

To use Java threads, create a Java proxy implementins
``java.lang.Runnable``.  The Runnable can then be passed any Java threading
mechanism to be executed.  Each time that Java threads transfer control
back to Python, the GIL is reacquired.

.. _concurrent_processing_threading_other_threads:

Other Threads
-------------

Some Python libraries offer other kinds of thread, (i.e. microthreads). How
they interact with Java depends on their nature. As stated earlier, any OS-
level threads will work without problem. Emulated threads, like microthreads,
will appear as a single thread to Java, so special care will have to be taken
for synchronization.


.. _concurrent_processing_customizing_javalangthread:

Customizing java.lang.Thread
============================

.. _concurrent_processing_overview:

Overview
--------

JPype automatically attaches Python threads to the JVM when they interact with
Java resources. Threads are attached as daemon threads to ensure that they do
not block JVM shutdown. While this behavior simplifies integration, it can lead
to resource leaks in thread-heavy applications if threads are not properly
detached when they terminate.

To address this, JPype customizes `java.lang.Thread` with additional methods
for managing thread attachment and detachment. These methods allow developers
to explicitly detach threads, freeing resources in the JVM and preventing leaks.

.. _concurrent_processing_customized_methods:

Customized Methods
------------------

The following methods are added to `java.lang.Thread` by JPype:

.. method:: java.lang.Thread.attach()

   Attaches the current thread to the JVM as a user thread.

   User threads prevent the JVM from shutting down until they are terminated or
   detached. This method can be used to convert a daemon thread to a user
   thread.

   **Raises**:
     - `RuntimeError`: If the JVM is not running.

.. method:: java.lang.Thread.attachAsDaemon()

   Attaches the current thread to the JVM as a daemon thread.

   Daemon threads act as background tasks and do not prevent the JVM from
   shutting down. JPype automatically attaches threads as daemon threads when
   they interact with Java resources. Use this method to explicitly attach a
   thread as a daemon.

   **Raises**:
     - `RuntimeError`: If the JVM is not running.

.. method:: java.lang.Thread.detach()

   Detaches the current thread from the JVM.

   This method frees the associated resources in the JVM for the current thread.
   It is particularly important for thread-heavy applications to prevent leaks.
   Detaching a thread does not interfere with its ability to reattach later.

   **Notes**:
     - This method cannot fail and is safe to call even if the JVM is not
       running.
     - There is no harm in calling this method multiple times or detaching
       threads early.

.. _concurrent_processing_examples_with_java_thread:

Examples with Java Thread
-------------------------

Here are examples of how to use the customized methods for `java.lang.Thread`:

.. code-block:: python

   import jpype
   import jpype.imports

   jpype.startJVM()

   # Attach the thread as a user thread
   java.lang.Thread.attach()
   print("Thread attached as a user thread.")

   # Perform Java operations here...

   # Detach the thread after completing Java operations
   java.lang.Thread.detach()
   print("Thread detached from the JVM.")

   # Attach the thread as a daemon thread
   java.lang.Thread.attachAsDaemon()
   print("Thread attached as a daemon thread.")

.. _concurrent_processing_best_practices_for_java_thread:

Best Practices for Java Thread
------------------------------

- **Detach Threads When They End**: For thread-heavy applications, ensure that
  Python threads detach themselves from the JVM before they terminate. This
  prevents resource leaks and ensures efficient memory usage.

- **Avoid Excessive Attachments**: While JPype automatically attaches threads,
  excessive thread creation without proper detachment can lead to resource
  exhaustion in the JVM.

- **Detach Early**: Detaching threads early, after completing all Java
  operations, is safe and does not interfere with reattachment later. This is
  especially important for applications that spawn many short-lived threads.

- **Monitor Resource Usage**: Regularly monitor JVM memory usage in
  thread-heavy applications to identify potential leaks caused by lingering
  thread attachments.

.. _concurrent_processing_summary_of_java_thread:

Summary of Java Thread
----------------------

JPype customizes `java.lang.Thread` to provide additional methods for managing
thread attachment and detachment to/from the JVM. While JPype automatically
attaches threads as daemon threads, it is crucial to detach threads explicitly
in thread-heavy applications to prevent resource leaks. By following best
practices, developers can ensure efficient memory usage and smooth integration
between Python and Java.


.. _synchronized:

Synchronization
===============

Java synchronization support can be split into two categories. The first is the
``synchronized`` keyword, both as prefix on a method and as a block inside a
method. The second are the three methods available on the Object class
(``notify, notifyAll, wait``).

To support the ``synchronized`` functionality, JPype defines a method called
``synchronized(obj)`` to be used with the Python ``with`` statement, where
obj has to be a Java object. The return value is a monitor object that will
keep the synchronization on as long as the object is kept alive.  For example,

.. code-block:: python

    from jpype import synchronized

    mySharedList = java.util.ArrayList()

    # Give the list to another thread that will be adding items
    otherThread.setList(mySharedList)

    # Lock the list so that we can access it without interference
    with synchronized(mySharedList):
        if not mySharedList.isEmpty():
            ...  # process elements
    # Resource is unlocked once we leave the block

The Python ``with`` statement is used to control the scope.  Do not
hold onto the monitor without a ``with`` statement.  Monitors held outside of a
``with`` statement will not be released until they are broken when the monitor
is garbage collected.

The other synchronization methods are available as-is on any Java object.
However, as general rule one should not use synchronization methods on Java
String as internal string representations may not be complete objects.

For synchronization that does not have to be shared with Java code, use
Python's support directly rather than Java's synchronization to avoid
unnecessary overhead.


.. _concurrent_processing_threading_examples:

Threading examples
==================

Java provides a very rich set of threading tools.  This can be used in Python
code to extend many of the benefits of Java into Python.  However, as Python
has a global lock, the performance of Java threads while using Python is not
as good as native Java code.

.. _concurrent_processing_limiting_execution_time:

Limiting execution time
-----------------------

We can combine proxies and threads to produce achieve a number of interesting
results.  For example:

.. code-block:: python

    def limit(method, timeout):
        """ Convert a Java method to asynchronous call with a specified timeout. """
        def f(*args):
            @jpype.JImplements(java.util.concurrent.Callable)
            class g:
                @jpype.JOverride
                def call(self):
                    return method(*args)
            future = java.util.concurrent.FutureTask(g())
            java.lang.Thread(future).start()
            try:
                timeunit = java.util.concurrent.TimeUnit.MILLISECONDS
                return future.get(int(timeout*1000), timeunit)
            except java.util.concurrent.TimeoutException as ex:
                future.cancel(True)
            raise RuntimeError("canceled", ex)
        return f

    print(limit(java.lang.Thread.sleep, timeout=1)(200))
    print(limit(java.lang.Thread.sleep, timeout=1)(20000))

Here we have limited the execution time of a Java call.


.. _concurrent_processing_multiprocessing:

Multiprocessing
===============

Because only one JVM can be started per process, JPype cannot be used with
processes created with ``fork``.  Forks copy all memory including the JVM.  The
copied JVM usually will not function properly thus JPype cannot support
multiprocessing using fork.

To use multiprocessing with JPype, processes must be created with "spawn".  As
the multiprocessing context is usually selected at the start and the default
for Unix is fork, this requires the creating the appropriate spawn context.  To
launch multiprocessing properly the following recipe can be used.

.. code-block:: python

   import multiprocessing as mp

   ctx = mp.get_context("spawn")
   process = ctx.Process(...)
   queue = ctx.Queue()
   # ...

When using multiprocessing, Java objects cannot be sent through the default
Python ``Queue`` methods as calls pickle without any Java support.  This can be
overcome by wrapping Python ``Queue`` to first encode to a byte stream using
the JPickle package.  By wrapping a ``Queue`` with the Java pickler any
serializable Java object can be transferred between processes.

In addition, a standard Queue will not produce an error if is unable to pickle
a Java object.  This can cause deadlocks when using multiprocessing IPC, thus
wrapping any Queue is required.


.. _managing_crossplatform_gui_environments:

Managing Cross-Platform GUI Environments
****************************************

JPype provides utility functions, `setupGuiEnvironment` and
`shutdownGuiEnvironment`, to manage GUI environments across platforms,
ensuring compatibility with macOS, Linux, and Windows. These functions are
particularly useful for Swing and JavaFX-based applications, where macOS
imposes specific requirements for GUI event loops. Even on Linux and Windows,
using `setupGuiEnvironment` ensures consistent behavior and avoids potential
issues with threading and event loops.

.. _managing_crossplatform_gui_environments_setupguienvironmentcb:

setupGuiEnvironment(cb)
=======================

**Description**:

`setupGuiEnvironment` ensures that GUI applications can run correctly across
all platforms. It is specifically designed to address macOS's requirement for
the main thread to run the event loop, but it is also recommended for Swing
and JavaFX applications on Linux and Windows to maintain cross-platform
compatibility and proper threading behavior.

**Parameters**:

- **cb**: A callback function that initializes and launches the GUI application.

**Behavior**:

- **macOS**:
  - Creates a Java thread using a `Runnable` proxy.
  - Starts the macOS event loop using `PyObjCTools.AppHelper.runConsoleEventLoop()`.

- **Other Platforms (Linux, Windows)**:
  - Executes the callback function directly.

**Why Use This Function for Swing and JavaFX Applications?**

Swing and JavaFX applications often rely on proper threading and event loop
management to function correctly. While macOS has strict requirements for
running the event loop on the main thread, using `setupGuiEnvironment` on
Linux and Windows ensures consistent behavior and avoids potential threading
issues, such as race conditions or improper GUI updates.

**Example**:

.. code-block:: python

    from jpype import setupGuiEnvironment
    from javafx.application import Platform

    def say_hello_later():
        """Test function for scheduling a task on the JavaFX Application Thread."""
        print("Hello from JavaFX!")

    def launch_gui():
        """Launch the GUI application."""
        # Example: Schedule a task on the JavaFX Application Thread
        Platform.runLater(say_hello_later)
        print("GUI launched")

    # Use setupGuiEnvironment to ensure cross-platform compatibility
    setupGuiEnvironment(launch_gui)

.. _managing_crossplatform_gui_environments_reestablishing_an_interactive_shell_on_another_thread:

Reestablishing an Interactive Shell on Another Thread
=====================================================

When using `setupGuiEnvironment`, the main thread may be occupied by the GUI
event loop (particularly on macOS). To allow interactive debugging in Python,
you can launch an interactive shell (e.g., IPython) on a separate thread.

**Steps**:

1. Use `setupGuiEnvironment` to start the GUI application.
2. Launch an interactive shell on a separate thread using Python's `threading`
   module.

**Example**:

.. code-block:: python

    import threading
    import IPython

    def launch_interactive_shell():
        """Launch an interactive shell on a separate thread."""
        IPython.embed()

    # Start the interactive shell on another thread
    thread = threading.Thread(target=launch_interactive_shell)
    thread.start()

By combining this approach with `setupGuiEnvironment`, you can interact with
the Python environment while the GUI application is running.

.. _managing_crossplatform_gui_environments_shutdownguienvironment:

shutdownGuiEnvironment()
========================

**Description**:

`shutdownGuiEnvironment` is used to cleanly terminate the macOS event loop. On
other platforms, it performs no action.

**Behavior**:

- **macOS**:
  - Stops the macOS event loop using `PyObjCTools.AppHelper.stopEventLoop()`.

- **Other Platforms (Linux, Windows)**:
  - No action is taken.

**Example**:

.. code-block:: python

    from jpype import shutdownGuiEnvironment

    # Shutdown the GUI environment (macOS-specific)
    shutdownGuiEnvironment()

.. _managing_crossplatform_gui_environments_best_practices_on_guis:

Best Practices on GUIs
--------------------------

- **Use `setupGuiEnvironment` for All Platforms**:
  Even though macOS has specific requirements, using `setupGuiEnvironment`
  ensures consistent behavior across all platforms, particularly for Swing
  and JavaFX applications.

- **Thread Safety**:
  Always schedule GUI updates using JavaFX's `Platform.runLater` or Swing's
  `SwingUtilities.invokeLater` to ensure they occur on the appropriate thread.

- **Interactive Debugging**:
  Launch an interactive shell on a separate thread for debugging while the GUI
  application is running.

- **Exception Handling**:
  Wrap callback functions in `try-except` blocks to prevent unhandled
  exceptions from disrupting the GUI.

- **Cross-Platform Testing**:
  Test the application on macOS, Linux, and Windows to ensure compatibility.

.. _managing_crossplatform_gui_environments_summary_of_guis:

Summary of GUIs
===============

The `setupGuiEnvironment` function is a critical tool for managing GUI
environments across platforms, particularly for Swing and JavaFX-based
applications. It ensures compatibility with macOS's event loop requirements
while maintaining simplicity on other platforms. Combined with the ability to
launch an interactive shell on a separate thread, this approach provides a
robust solution for developing and debugging GUI applications in Python.


.. _miscellaneous_topics:

Miscellaneous topics
********************

This chapter contains all the stuff that did not fit nicely into the narrative
about JPype.  Topics include database interfacing, code completion, performance,
debugging Java within JPype, debugging JNI and other JPype failures, how caller sensitive
methods are dealt with, and finally limitations of JPype.


.. _miscellaneous_topics_database_access_with_jpypedbapi2:

Database Access with `jpype.dbapi2`
===================================

JPype provides the `jpype.dbapi2` module, which allows Python applications to
interact with Java-based database drivers using the Python Database API
Specification (PEP 249). This module bridges Python and Java, enabling seamless
access to databases that lack native Python drivers but provide JDBC drivers.

.. _miscellaneous_topics_key_features_of_dbapi2:

Key Features of dbapi2
----------------------
- **PEP 249 Compliance**: Implements the Python Database API Specification for
  standardized database interaction.
- **JDBC Integration**: Uses Java's JDBC (Java Database Connectivity) to connect
  to databases.
- **Cross-Platform**: Supports any database with a JDBC driver, including
  enterprise databases like Oracle, DB2, and SQL Server.

.. _miscellaneous_topics_prerequisites_for_dbapi2:

Prerequisites for dbapi2
------------------------
- Ensure the JVM is started with the appropriate classpath for the JDBC driver.
- Obtain the JDBC driver for the target database and include its path in the
  `classpath`.

.. _miscellaneous_topics_example_usage_of_dbapi2:

Example Usage of dbapi2
-----------------------
.. code-block:: python

    import jpype
    import jpype.dbapi2 as dbapi2

    # Start the JVM with the JDBC driver
    jpype.startJVM(classpath=["path/to/jdbc/driver.jar"])

    # Connect to the database
    connection = dbapi2.connect(
        "jdbc:database_url",  # JDBC URL for the database
        {"user": "username", "password": "password"}  # Connection properties
    )

    # Create a cursor and execute a query
    cursor = connection.cursor()
    cursor.execute("SELECT * FROM my_table")

    # Fetch and process results
    results = cursor.fetchall()
    for row in results:
        print(row)

    # Close the cursor and connection
    cursor.close()
    connection.close()

    # Shut down the JVM
    jpype.shutdownJVM()


.. _miscellaneous_topics_benefits_of_dbapi2:

Benefits of dbapi2
------------------
- Access databases that lack native Python drivers but provide JDBC drivers.
- Leverage advanced features of Java-based database drivers.
- Maintain compatibility with Python's standard database API.


.. _miscellaneous_topics_limitations_of_dbapi2:

Limitations of dbapi2
----------------------
- Requires a running JVM, which may introduce overhead compared to native Python
  database drivers.
- Performance may be slightly impacted due to Python-Java interaction.

.. _miscellaneous_topics_use_cases_of_dbapi2:

Use Cases of dbapi2
-------------------
- Connecting to enterprise databases like Oracle, SQL Server, or DB2.
- Utilizing advanced capabilities of JDBC drivers within Python applications.


.. _miscellaneous_topics_javadoc:

Javadoc
=======

JPype can display javadoc in ReStructured Text as part of the Python
documentation.  To access the javadoc, the javadoc package must be located on
the classpath.  This includes the JDK package documentation.

For example to get the documentation for ``java.lang.Class``, we start the JVM
with the JDK documentation zip file on the classpath.

.. code-block: java

     import jpype
     jpype.startJVM(classpath='jdk-11.0.7_doc-all.zip')

We can then access the java docs for the String with ``help(java.lang.String)``
or for the methods with ``help(java.lang.String.trim)``.  To use the javadoc
supplied by a third party include the both the jar and javadoc in the
classpath.

.. code-block: java

     import jpype
     jpype.startJVM(classpath=['gson-2.8.5.jar', 'gson-2.8.5-javadoc.jar'])

The parser will ignore any javadoc which cannot be extracted.  It has some
robustness against tags that are not properly closed or closed twice.  Javadoc
with custom page layouts will likely not be extracted.

If javadoc for a class cannot be located or extracted properly, default
documentation will be generated using Java reflection.

.. _miscellaneous_topics_autopep8:

Autopep8
========

When Autopep8 is applied a Python script, it reorganizes the imports to conform
to E402_. This has the unfortunate side effect of moving the Java imports above
the startJVM statement.  This can be avoided by either passing in ``--ignore
E402`` or setting the ignore in ``.pep8``.

.. _E402: https://www.flake8rules.com/rules/E402.html

Example:

.. code-block:: python

        import jpype
        import jpype.imports

        jpype.startJVM()

        from gov.llnl.math import DoubleArray


Result without ``--ignore E402``

.. code-block:: python

        from gov.llnl.math import DoubleArray  # Fails, no JVM running
        import jpype
        import jpype.imports

        jpype.startJVM()


.. _miscellaneous_topics_performance:

Performance
===========

JPype uses JNI, which is well known in the Java world as not being the most
efficient of interfaces. Further, JPype bridges two very different runtime
environments, performing conversion back and forth as needed. Both of these
can impose performance bottlenecks.

JNI is the standard native interface for most, if not all, JVMs, so there is
no getting around it. Down the road, it is possible that interfacing with CNI
(GCC's Java native interface) may be used. Right now, the best way to reduce
the JNI cost is to move time critical code over to Java.

Follow the regular Python philosophy : **Write it all in Python, then write
only those parts that need it in C.** Except this time, it's write the parts
that need it in Java.

Everytime an object is passed back and forth, it will incure a conversion
cost.. In cases where a given object (be it a string, an object, an array, etc
...) is passed often into Java, the object should be converted once and cached.
For most situations, this will address speed issues.

To improve speed issues, JPype has converted all of the base classes into
CPython.  This is a very significant speed up over the previous versions of
the module.  In addition, JPype provides a number of fast buffer transfer
methods. These routines are triggered automatically working with any buffer
aware class such as those in NumPy.

As a final note, while a JPype program will likely be slower than its pure
Java counterpart, it has a good chance of being faster than the pure Python
version of it. The JVM is a memory hog, but does a good job of optimizing
code execution speeds.


.. _miscellaneous_topics_code_completion:

Code completion
===============

Python supports a number of different code completion engines that are
integrated in different Python IDEs.  JPype has been tested with both the
IPython greedy completion engine and Jedi.  Greedy has the disadvantage
that is will execute code resulting potentially resulting in an undesirable
result in Java.

JPype is Jedi aware and attempts to provide whatever type information that
is available to Jedi to help with completion tasks.  Overloaded methods are
opaque to Jedi as the return type cannot be determined externally.  If all of
the overloads have the same return type, the JPype will add the return type
annotation permitting Jedi to autocomplete through a method return.

For example:

.. code-block:: python

        JString("hello").substring.__annotations__
        # Returns {'return': <java class 'java.lang.String'>}

Jedi can manually be tested using the following code.

.. code-block:: python

        js = JString("hello")
        src = 'js.s'
        script = jedi.Interpreter(src, [locals()])
        compl = [i.name for i in script.completions()]

This will produce a list containing all method and field that begin with
the letter "s".

JPype has not been tested with other autocompletion engines such as Kite.


.. _miscellaneous_topics_garbage_collection:

Garbage collection
==================

Garbage collection (GC) is supposed to make life easier for the programmer by
removing the need to manually handle memory.  For the most part it is a good
thing.  However, just like running a kitchen with two chiefs is a bad idea,
running with two garbage collections is also bad.  In JPype we have to contend
with the fact that both Java and Python provide garbage collection for their
memory and neither provided hooks for interacting with an external garbage
collector.

For example, Python is creating a bunch a handles to Java memory for a
period of time but they are in a structure with a reference loop internal to
Python.  The structures and handles are small so Python doesn't see an issue,
but each of those handles is holding 1M of memory in Java space.  As the heap
fills up Java begins garbage collecting, but the resources can't be freed
because Python hasn't cleanup up these structures.  The reverse occurs if a
proxy has any large NumPy arrays.  Java doesn't see a problem as it has plenty
of space to work in but Python is running its GC like mad trying to free up
space to work.

To deal with this issue, JPype links the two garbage collectors.  Python is
more aggressive in calling GC than Java and Java is much more costly than
Python in terms of clean up costs.  So JPype manages the balance.  JPype
installs a sentinel object in Java.  Whenever that sentinel is collected Java
is running out of space and Python is asked to clean up its space as well.  The
reverse case is more complicated as Python can't just call Java's expensive
routine any time it wants.  Instead JPype maintains a low-water and high-water
mark on Python owned memory.  Each time it nears a high-water mark during a
Python collection, Java GC gets called.  If the water level shrinks than
Java was holding up Python memory and the low-water mark is reset.
Depending on the amount of memory being exchanged the Java GC may trigger
as few as once every 50 Python GC cycles or as often as every other.
The sizing on this is dynamic so it should scale to the memory use of
a process.


.. _miscellaneous_topics_using_jpype_for_debugging_java_code:

Using JPype for debugging Java code
===================================

One common use of JPype is to function as a Read-Eval-Print Loop for Java. When
operating Java though Python as a method of developing or debugging Java there
are a few tricks that can be used to simplify the job.  Beyond being able to
probe and plot the Java data structures interactively, these methods include:

1) Attaching a debugger to the Java JVM being run under JPype.
2) Attaching debugging information to a Java exception.
3) Serializing the state of a Java process to be evaluated at a later point.

We will briefly discuss each of these methods.


.. _miscellaneous_topics_using_jpype_for_debugging_java_code_attaching_a_debugger:

Attaching a Debugger
--------------------

Interacting with Java through a shell is great, but sometimes it is necessary
to drop down to a debugger. To make this happen we need to start the JVM
with options to support remote debugging.

We start the JVM with an agent which will provide a remote debugging port which
can be used to attach your favorite Java debugging tool.  As the agent is
altering the Java code to create additional debugging hooks, this process can
introduce additional errors or alter the flow of the code.  Usually this is
used by starting the JVM with the agent, placing a pause marker in the Python
code so that developer can attach the Java debugger, executing the Python code
until it hits the pause, attaching the debugger, setting break point in Java,
and then asking Python to proceed.

So lets flesh out the details of how to accomplish this...

.. code-block:: python

    jpype.startJVM("-Xint", "-Xdebug", "-Xnoagent",
      "-Xrunjdwp:transport=dt_socket,server=y,address=12999,suspend=n")

Next, add a marker in the form of a pause statement at the location where
the debugger should be attached.

.. code-block:: python

    input("pause to attach debugger")
    myobj.callProblematicMethod()

When Python reaches that point during execution, switch to a Java IDE such as
NetBeans and select Debug : Attach Debugger.  This brings up a window (see
example below).  After attaching (and setting desired break points) go back to
Python and hit enter to continue.  NetBeans should come to the foreground when
a breakpoint is hit.

.. image:: attach_debugger.png


Attach data to an Exception
---------------------------

Sometimes getting to the level of a debugger is challenging especially if the
code is large and error occurs rarely. In this case, it is often beneficial to
attach data to an exception. To achieve this, we need to write a small utility
class. Java exceptions are not strictly speaking expandable, but they can be
chained. Thus, it we create a dummy exception holding a ``java.util.Map`` and
attach it to as the cause of the exception, it will be passed back down the
call stack until it reaches Python. We can then use ``getCause()`` to retrieve
the map containing the relevant data.


.. _miscellaneous_topics_capturing_the_state:

Capturing the state
-------------------

If the program is not running in an interactive shell or the program run time
is long, we may not want to deal with the problem during execution. In this
case, we can serialize the state of the relevant classes and variables. To use
this option, we mus make sure all of the classes in Java that we are using
are ``Serializable``.  Then add a condition that detects the faulty algorithm state.
When the fault occurs, create a ``java.util.HashMap`` and populate it with
the values to be examined from within Python.  Use serialization to write
the entire structure to a file.  Execute the program and collect all of the
state files.

Once the state files have been collected, start Python with an interactive
shell and launch JPype with a classpath for the jars.  Finally,
deserialize the state files to access the Java structures that have
been recorded.


.. _miscellaneous_topics_getting_additional_diagnostics:

Getting Additional Diagnostics
==============================

For the most part, JPype operates as intended, but that does not mean there are
no bugs or edge cases. Given the complexity of interactions between Python and
Java, untested scenarios may occasionally arise. JPype provides several
diagnostic tools to assist in debugging these issues. These tools require
accessing private JPype symbols, which may change in future releases. As such,
they should not be used in production code.

.. _checking-type-cast:

Checking the Type of a Cast
---------------------------

Sometimes it is difficult to understand why a particular method overload is
selected by the method dispatch. To check the match type for a conversion, use
the private method ``Class._canConvertToJava``. This will return a string
indicating the type of conversion performed: ``none``, ``explicit``,
``implicit``, or ``exact``.

To test the result of the conversion process, call ``Class._convertToJava``.
Unlike an explicit cast, this method attempts to perform the conversion without
bypassing the logic involved in casting. It replicates the exact process used
when a method is called or a field is set.

.. _cpp-exceptions:

C++ Exceptions in JPype
------------------------

Internally, JPype can generate C++ exceptions, which are converted into Python
exceptions for the user. To trace an error back to its C++ source, you can
enable stack traces for C++ exceptions. Use the following command:

.. code-block:: python

   import _jpype
   _jpype.enableStacktraces(True)

Once enabled, all C++ exceptions that fall through a C++ exception handling
block will produce an augmented stack trace. If the JPype source code is
available, the stack trace can even include the corresponding lines of code
where the exceptions occurred. This can help identify the source of errors that
originate in C++ code but propagate to Python as exceptions.

.. _tracing:

Tracing
-------

Tracing mode logs every JNI call, along with object addresses and exceptions,
to the console. To enable tracing, JPype must be recompiled with the
``--enable-tracing`` option.

Tracing is useful for identifying failures that originate in one JNI call but
manifest later. However, this mode produces verbose logs and is recommended
only for advanced debugging.


.. _instrumentation:

Instrumentation
---------------

JPype supports an instrumentation mode for testing error-handling paths. This
mode allows you to simulate faults at designated points in JPype's execution
flow. To enable instrumentation, recompile JPype with the ``--enable-coverage``
option.

Once instrumentation is enabled, use the private module command
``_jpype.fault`` to trigger an error. The argument to the fault command must be
the name of a function or a predefined fault point. When the fault point is
encountered, a ``SystemError`` is raised. Instrumentation is primarily useful
for verifying the robustness of JPype's exception handling mechanisms.


.. _using-debugger:

Using a Debugger
----------------

If JPype crashes, it may be necessary to use a debugger to obtain a backtrace.
However, debugging JPype can be challenging due to the JVM's handling of
segmentation faults. The JVM intercepts segmentation faults to allocate memory
or handle internal operations, which can corrupt stack frames.

To debug JPype using tools such as ``gdb`, you must configure the debugger to
ignore segmentation faults intentionally triggered by the JVM. For example, use
the following command to start ``gdb`` and ignore the first fault:

.. code-block:: shell

   gdb --args python script.py
   (gdb) handle SIGSEGV nostop noprint pass

This configuration allows the debugger to bypass JVM-related faults while
capturing legitimate errors. Additionally, disable Python's fault handler to
avoid interference with segmentation fault reporting.


.. _caller sensitive:

Caller-Sensitive Methods
-------------------------

Java's security model tracks the caller of certain methods to determine the
level of access. These methods, known as "caller-sensitive methods," require
special handling in JPype. Examples of caller-sensitive methods include
``Class.forName``, ``java.lang.ClassLoader`` methods, and certain methods in
``java.sql.DriverManager``.

To handle caller-sensitive methods, JPype routes calls through an internal
Java package, ``org.jpype``, which executes the method within the JVM. This
ensures proper security context and avoids access errors. Although this
mechanism introduces slight overhead, it is necessary for compatibility with
Java's security model.


.. _limitations_jvm:

JPype Known limitations
=======================

This section lists those limitations that are unlikely to change, as they come
from external sources.


.. _miscellaneous_topics_jpype_known_limitations_annotations:

Annotations
-----------

Some frameworks such as Spring use Java annotations to indicate specific
actions.  These may be either runtime annotations or compile time annotations.
Occasionally while using JPype someone would like to add a Java annotation to a
JProxy method so that a framework like Spring can pick up that annotation.

JPype uses the Java supplied ``Proxy`` to implement an interface.  That API
does not support addition of a runtime annotation to a method or class.  Thus,
all methods and classes when probed with reflection that are implemented in
Python will come back with no annotations.

Further, the majority of annotation magic within Java is actually performed at
compile time.  This is accomplished using an annotation processor.  When a
class or method is annotated, the compiler checks to see if there is an
annotation processor which then can produce new code or modify the class
annotations.  As this is a compile time process, even if annotations were added
by Python to a class they would still not be active as the corresponding
compilation phase would not have been executed.

This is a limitation of the implementation of annotations by the Java virtual
machine.  It is technically possible though the use of specialized code
generation with the ASM library or other code generation to add a runtime
annotation.  Or through exploits of the Java virtual machine annotation
implementation one can add annotation to existing Java classes.  But these
annotations are unlikely to be useful. As such JPype will not be able to
support class or method annotations.



.. _miscellaneous_topics_restarting_the_jvm:

Restarting the JVM
-------------------

JPype caches many resources to the JVM. Those resource are still allocated
after the JVM is shutdown as there are still Python objects that point to those
resources.  If the JVM is restarted, those stale Python objects will be in a
broken state and the new JVM instance will obtain the references to these
resulting in a memory leak. Thus it is not possible to start the JVM after it
has been shut down with the current implementation.


.. _miscellaneous_topics_running_multiple_jvm:

Running multiple JVM
--------------------

JPype uses the Python global import module dictionary, a global Python to Java
class map, and global JNI TypeManager map.  These resources are all tied to the
JVM that is started or attached. Thus operating more than one JVM does not
appear to be possible under the current implementation.  Further, as of Java
1.2 there is no support for creating more than one JVM in the same process.

Difficulties that would need to be overcome to remove this limitation include:

- Finding a JVM that supports multiple JVMs running in the same process.
  This can be achieved on some architectures by loading the same shared
  library multiple times with different names.
- Alternatively as all available JVM implementations support on one JVM
  instance per process, a communication layer would have to proxy JNI
  class from JPype to another process. But this has the distinct problem that
  remote JVMs cannot register native methods nor share memory without
  considerable effort.
- Which JVM would a static class method call. The class types
  would need to be JVM specific (ie. ``JClass('org.MyObject', jvm=JVM1)``)
- How would a wrapper from two different JVM coexist in the
  ``jpype._jclass`` module with the same name if different class
  is required for each JVM.
- How would the user specify which JVM a class resource is created in
  when importing a module.
- How would objects in one JVM be passed to another.
- How can boxed and String types hold which JVM they will box to on type
  conversion.

Thus it appears prohibitive to support multiple JVMs in the JPype
class model.


.. _miscellaneous_topics_jpype_known_limitations_errors_reported_by_python_fault_handler:

Errors reported by Python fault handler
---------------------------------------

The JVM takes over the standard fault handlers resulting in unusual behavior if
Python handlers are installed.  As part of normal operations the JVM will
trigger a segmentation fault when starting and when interrupting threads.
Pythons fault handler can intercept these operations and interpret these as
real faults.  The Python fault handler with then reporting extraneous fault
messages or prevent normal JVM operations.  When operating with JPype, Python
fault handler module should be disabled.

This is particularly a problem for running under pytest as the first action it
performs is to take over the error handlers. This can be disabled by adding
this block as a fixture at the start of the test suite.

.. code-block:: python

    try:
        import faulthandler
        faulthandler.enable()
        faulthandler.disable()
    except:
        pass

This code enables fault handling and then returns the default handlers which
will point back to those set by Java.

Python faulthandlers can also interfer with the JIT compiler.  The Java
Just-In-Time (JIT) compiler determines when a portion of code needs to be
compiled into machine code to improve performance.  When it does, it triggers
either a SEGSEGV or SEGBUS depending on the machine architecture which breaks
out any threads which are currently executing the existing code.  Because the
JIT compiler self triggers, this will cause a failure to appear in a call which
worked earlier in the execution without an issue.

If the Python fault handler interrupts this process, it will produce a "Fatal
Python Error:" followed by either SIGSEGV or SEGBUS.  This error message then
fails to hit the needed Java handler resulting in a crash.  This message will
disappear if the JIT compiler is disabled with the option "-Xint".  Running
without the JIT compiler creates a severe performance penalty so disabling the
fault handler should be the preferred solution.

Some modules such as Abseil Python Common Libraries (absl) automatically and
unconditionally install faulthandlers as part of their normal operation.  To
prevent an issue simply insert a call to disable the faulthandler after the
module has enabled it, using

.. code-block:: python

    import faulthandler
    faulthandler.disable()

For example, absl installs faulthandlers in ``app.run``, thus the first call to
main routine would need to disable faulthandlers to avoid potential crashes.



.. _miscellaneous_topics_unsupported_java_versions:

Unsupported Java Versions
-------------------------

JPype now requires the use of the module API, which was introduced in **Java
9**. As a result, the earliest version of Java supported by JPype is **Java
11**, which is part of the Long-Term Support (LTS) release.

If you need to use **Java 8**, you must use JPype version **1.5.2 or earlier**,
as newer versions of JPype no longer support Java 8.



.. _miscellaneous_topics_unsupported_python_versions:

Unsupported Python versions
---------------------------


.. _miscellaneous_topics_python_38_and_earlier:

Python 3.8 and earlier
~~~~~~~~~~~~~~~~~~~~~~

The oldest version of Python that we currently support is Python 3.5.  Before
Python 3.5 there were a number of structural difficulties in the object model
and the buffering API.  In principle, those features could be excised from
JPype to extend support to older Python 3 series version, but that is unlikely
to happen without a significant effort.  Recent changes in memory models
require Python 3.8 or later.


.. _miscellaneous_topics_python_2:

Python 2
~~~~~~~~

CPython 2 support was removed starting in 2020.  Please do not report to us
that Python 2 is not supported.  Python 2 was a major drag on this project for
years.  Its object model is grossly outdated and thus providing for it greatly
impeded progress.  When the life support was finally pulled on that beast,
I like many others breathed a great sigh of relief and gladly cut out the
Python 2 code.  Since that time JPype operating speed has improved anywhere
from 300% to 10000% as we can now implement everything back in CPython rather
than band-aiding it with interpreted Python code.


.. _miscellaneous_topics_pypy:

PyPy
~~~~

The GC routine in PyPy 3 does not play well with Java. It runs when it thinks
that Python is running out of resources. Thus a code that allocates a lot
of Java memory and deletes the Python objects will still be holding the
Java memory until Python is garbage collected. This means that out of
memory failures can be issued during heavy operation.  We have addressed linking
the garbage collectors between CPython and Java, but PyPy would require a
modified strategy.

Further, when we moved to a completely Python 3 object model we unfortunately
broke some of the features that are different between CPython and PyPy.  The
errors make absolutely no sense to me.  So unless a PyPy developer generously
volunteering time for this project, this one is unlikely to happen.


.. _miscellaneous_topics_jython_python:

Jython Python
~~~~~~~~~~~~~

If for some reason you wandered here to figure out how to use Java from
Jython using JPype, you are clearly in the wrong place. On the other hand,
if you happen to be a Jython developer who is looking for inspiration on how
to support a more JPype like API that perhaps we can assist you.  Jython aware
Python modules often mistake JPype for Jython at least up until the point
that differences in the API triggers an error.



.. _miscellaneous_topics_unsupported_java_virtual_machines:

Unsupported Java virtual machines
---------------------------------

The open JVM implementations *Cacao* and *JamVM* are known not to work with
JPype.


.. _miscellaneous_topics_unsupported_platforms:

Unsupported Platforms
---------------------

Some platforms are problematic for JPype due to interactions between the
Python libraries and the JVM implementation.


.. _miscellaneous_topics_cygwin:

Cygwin
~~~~~~

Cygwin was usable with previous versions of JPype, but there were numerous
issues for which there is was not good solution solution.

Cygwin does not appear to pass environment variables to the JVM properly
resulting in unusual behavior with certain windows calls. The path
separator for Cygwin does not match that of the Java DLL, thus specification
of class paths must account for this.  Threading between the Cygwin libraries
and the JVM was often unstable.

.. _freezing:

Freezing
========

JPype supports freezing and deployment with
`PyInstaller <https://pyinstaller.readthedocs.io/>`_.  The hook is included
with JPype installations and no extra configuration should be needed.


.. _miscellaneous_topics_glossary:

Useless Trivia
==============

Thrameos, the primary developer, maintains the correct pronounciation of
JPype is Jay-Pie-Pee like the word recipe.  His position is that application
of a silient e is inappropriate for this made up name.

1) Jay-Pie is more emblematic of the project goal which is connection of
Java and Python.

2) Rules in English regarding a silent e following a p are poor and depend
mostly on the origin of the word.  Is it like type from the Greek word
typus or French récipé?  As the y in not being modified as it would otherwise
be pronounced as Pie like NumPy and SciPy, that means the only logical
conclusion is that it was intended to be voiced.

3) Jay-pipe or gh-pipe implies that this project is built on pipes (also known
as series of tubes such as the internet).  JPype is based on JNI (Jay-eN-Ey).
If you are looking for a pipe based Java connection use Py4J.  If you are looking
for solution based on JNI use JPypé.





Glossary
========

.. _miscellaneous_topics_glossary_b:

B
-

**Boxed Types**
Immutable Java objects that wrap primitive types (e.g., `java.lang.Integer`
for `int`). Used when primitives need to be treated as objects.

.. _miscellaneous_topics_glossary_c:

C
-

**Caller Sensitive Methods**
Java methods that determine the caller's access level based on the call stack.
JPype uses special mechanisms to handle these methods safely.

**Classpath**
A parameter specifying the location of Java classes or JAR files required by
the JVM. It is essential for loading Java libraries.

.. _miscellaneous_topics_glossary_d:

D
-

**Deferred Proxy**
A proxy that is created before the JVM is started by specifying the interface
as a string and using the `deferred=True` argument. The implementation is
checked only when the proxy is first used.

.. _miscellaneous_topics_glossary_e:

E
-

**Exact Conversion**
A type conversion in JPype where the Python type matches the Java type
exactly. Example: Python `int` to Java `int`.

.. _miscellaneous_topics_glossary_f:

F
-

**Functional Interface**
A Java interface with a single abstract method (SAM). JPype allows Python
callables (e.g., functions, lambdas) to be passed directly to Java methods
expecting a functional interface.

.. _miscellaneous_topics_glossary_g:

G
-

**Garbage Collection (GC)**
The process of automatically reclaiming memory occupied by unused objects.
JPype links Python's and Java's garbage collectors to avoid memory issues.

.. _miscellaneous_topics_glossary_i:

I
-

**Implicit Conversion**
A type conversion in JPype where Python types are automatically converted to
compatible Java types. Example: Python `int` to Java `long`.

**Interface**
A Java construct that defines a set of methods without implementations. JPype
allows Python classes to implement Java interfaces using proxies.


.. _miscellaneous_topics_glossary_m:

M
-

**Mapping**
A Python concept for key-value pairs. JPype customizes Java `Map` classes to
behave like Python dictionaries.

**Multidimensional Arrays**
Java arrays with multiple dimensions. JPype supports creating and working with
these arrays using nested lists.

.. _miscellaneous_topics_glossary_n:

N
-

**NumPy Integration**
JPype's ability to efficiently transfer data between Java arrays and NumPy
arrays using memory buffers.

.. _miscellaneous_topics_glossary_o:

O
-

**Object Instance**
A Java object created from a class. Example: `obj = MyClass()` creates an
instance of `MyClass`.

**Overloaded Methods**
Java methods with the same name but different parameter types or counts. JPype
selects the appropriate overload based on the Python arguments provided.

.. _miscellaneous_topics_glossary_p:

P
-

**Primitive Types**
Basic data types in Java (e.g., `int`, `float`, `boolean`). JPype maps these
types to Python equivalents (e.g., `JInt`, `JFloat`, `JBoolean`).

**Proxy**
A proxy is an intermediary that allows Python objects to implement Java
interfaces. In the context of JPype, Proxies focus solely on providing the
required Java interface functionality.

.. _miscellaneous_topics_glossary_s:

S
-

**SAM (Single Abstract Method)**
A Java interface with a single abstract method. Used in functional programming
and supported by JPype for Python callables.

**Synchronized**
A Java keyword for thread-safe operations. JPype provides the
`jpype.synchronized()` method for similar functionality in Python.

.. _miscellaneous_topics_glossary_t:

T
-

**Type Factory**
A JPype mechanism for creating Java types in Python. Examples include `JClass`
for classes and `JArray` for arrays.

.. _miscellaneous_topics_glossary_u:

U
-

**UnsupportedClassVersionError**
A JVM error indicating that a JAR file was compiled for a newer Java version
than the JVM being used.

.. _miscellaneous_topics_glossary_w:

W
-

**Wrapper**
A wrapper is an object used to represent an object from one programming language
in another programming language, providing a native look and feel. In JPype,
wrappers are used to present Java objects to Python, making them behave like
native Python objects while retaining their underlying Java functionality.
Wrappers encapsulate Java references, such as class instances, arrays, or boxed
types, and may also include proxies when implementing Java interfaces.

Future versions of JPype aim to introduce the ability to manipulate Python
objects from Java. In this case, wrappers will represent Python objects from the
perspective of Java, providing a seamless interface for Java code to interact
with Python objects.
