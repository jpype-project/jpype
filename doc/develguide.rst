Developer Guide
===============

Overview
--------

This document describes the guts of jpype.  It is intended 
lay out the architecture of the jpype code to aid intrepid lurkers 
to develop and debug the jpype code once I am run over by a bus.


History
~~~~~~~

When I started work on this project it had already existed for over 10 years.
The original developer had intended a much larger design with modules to
support multiple languages such as Ruby.  As such it was constructed with 
three layers of abstraction.  It has a wrapper layer over Java in C++, a
wrapper layer for the Python api in C++, and an abstraction layer intended
to bridge Python and other interpreted languages.  This multilayer abstraction
ment that every debugging call had to drop through all of those layers.
Memory management was split into multiple pieces with Java controlling a
portion of it, C++ holding a bunch of resources, Python holding additional
resources, and HostRef controlling the lifetime of objects shared between the
layers.  It also had its own reference counting system for handing Java
references on a local scale.

This level of complexity was just about enough to scare off all but the most
hardened programmer.  Thus I set out to eliminate as much of this as I could.
Java already has its own local referencing system to deal in the form of 
LocalFrames.  It was simiply a matter of setting up a C++ object to 
hold the scope of the frames to eliminate that layer.  The Java abstraction
was laid out in a fashion somewhat orthagonally to the Java inheritance 
diagram.  Thus that was reworked to something more in line which could be 
safely completed without disturbing other layers.  The multilanguage 
abstraction layer was already perced in multiple ways for speed.  However, 
as this interwove throughout all the kit it was a terrible lift to remove
and thus required gutting the Python layer as well to support the operations
that were being performed by the HostRef.  

The remaining codebase is fairly slim and reasonably streamlined.  This 
rework cut out about 30% of the existing code and sped up the internal
operations.  The Java C++ interface matches the Java class hierachy.


Architecture
~~~~~~~~~~~~

JPype is split into several distinct pieces.  

``jpype`` Python module
  The majority of the front end logic for the toolkit is in Python jpype module.  
  This module deals with the construction of class wrappers and control functions.  
  The classes in the layer are all prefixed by ``J``.

``_jpype`` CPython module  
  The native module is supported by a CPython module called ``_jpype``.  The ``_jpype`` 
  module is located in ``native/python`` and has C style classes with a prefix ``PyJP``.

  This CPython layer acts as a front end for passing to the C++ layer.  
  It performs some error checking.  In addition to the module functions in 
  ``PyJPModule``, the module has multiple Python classes to support the native jpype
  code such as ``PyJPClass``, ``PyJPArray``, ``PyJPValue``, ``PyJPValue``, etc.  
  
CPython API wrapper  
  In addition to the exposed Python module layer, there is also a C++ wrapper
  for the Python API.  This is located in ``native/python`` and has the prefix
  ``JPPy`` for all classes.
  
  There are two parts of this api wrapper, ``jp_pythonenv`` and ``jp_pythontypes``.  
  The ``jp_pythonenv`` holds all of the resources that C++ needs to communicate 
  with the jpype native module.  ``jp_pythontypes`` wraps the required parts of 
  the CPython API in C++ for use in the C++ layer.

C++ JNI layer
  The guts that drive Java are in the C++ layer located in ``native/common``.  This layer
  has the namespace ``JP``.  The code is divided into wrappers for each Java type,
  a typemanager for mapping from Java names to class instances, support classes
  for proxies, and a thin JNI layer used to help ensure rigerous use of the same
  design patterns in the code.  The primary responsibility of this layer is 
  type conversion and matching of method overloads.

Java layer
  In addition to the C++ layer, jpype has a native Java layer.  This code
  is compiled as a "thunk" which is loaded into the JVM in the form of a
  a binary stored as a string.  Code for Java is found in ``native/java``.
  The Java layer is divided into two parts,
  a bootstrap loader and a jar containing the support classes.  The Java
  layer is responsible managing the lifetime of shared Python, Java, and C++ objects.


``jpype`` module
-----------------

The jpype module itself is made of a series of support classes which 
act as factories for the individual wrappers that are created to mirror
each Java class.  Because it is not possible to wrap all Java classes 
with staticly created wrappers, instead jpype dynamically creates
Python wrappers as requested by the user.

The wrapping process is triggered in two ways.  The user can manually
request creating a class by importing a class wrapper with jpype.imports
or ``JPackage`` or by manually invoking it with ``JClass``.  Or the class wrapper
can be created automatically as a result of a return type or exception
thrown to the user.

Because the classes are created dynamically, the class structure
uses a lot of Python meta programming.  As the programming 
model for Python 2.7 and 3 series are rather different, the
exact formuation is restricted to a set of common formulations 
that are shared between the versions.  

Each class wrapper derives from the class wrappers of each of the 
wrappers corresponding to the Java classes that each class extends 
and implements.  The key to this is to hacked ``mro``.  The ``mro``
orders each of the classes in the tree such that the most drived 
class methods are exposed, followed by each parent class.  This
must be ordered to break ties resulting from multiple inheritance
of interfaces.  

The ``mro`` has one aspect that breaks the Python object model.  Normally
it is a requirement that every class that inherits from another class
must inherit all of the previous parents.  However, Java has two distinct
types of inheritance, extension and implementation.  As such we delete
the JInterface parent from all concrete class implementation during 
the mro resolution phase.

jpype objects work with the inner layers primarily through duck typing
using a series of special fields.  These fields correspond to a
JNI type such as ``jvalue``, ``jclass``, ``jobject``, and ``jstring``.  But as these
resources cannot be held directly, they are provided as resources exposed
as ``_jpype.PyJP`` classes.

``jvalue``
+++++++++++++++++++++++++++++++

In the earlier design, wrappers, primitives and objects were all seperate
concepts.  At the JNI layer these are unified by a common element called 
jvalue.  A ``jvalue`` is a union of all primitives with the jobject.  The jobject
can represent anything derived from Java object including the pseudo class
jstring.  To represent these in a common way all jpype objects that 
represent an instance of a Java resource have a ``__javavalue__`` field which 
should be of type ``_jpype.PyJPValue``.  Rather than forcing checking of individual
types, we use duck typing of simply checking for a ``PyJPValue`` in this field.
In addition to the union, the jvalue also carries a hidden type.  Thus hidden
type is used to help in casting the object and resolving method overloads.
We will discuss this object further in the CPython section.

``jclass``
+++++++++++++++++++++++++++++++

In addition class wrappers have a ``_jpype.PyJPClass`` field which represents the 
internal class wrapper.  This class wrapper holds the reflection api used to create
the class methods and fields.  This field is stored as the ``__javaclass__``
in the class wrapper.  As the class wrapper is used to create an object instance
``__javaclass__`` also appears in the objects.  Only objects that have a ``__javaclass__``
and lack a ``__javavalue__`` are treated as class wrappers for the purposes of 
duck typing.

Because the Java class is both an object and type, we used the duck typing to 
allow the class pointer to be converted into a class instance.  This is exposed
as property called ``class_``.  The this ``class_`` property is the equivalent of the 
.class member of a Java class name or ``getClass()`` of a class instance.  As it 
is an object instance of ``java.lang.Class`` it can be used for any reflection 
needs by the user.  However, when working with the jpype core module, on
needs to be aware of that the this class instance is not available until 
the key wrappers are created.  See bootstrapping for further details.

``jarray``
+++++++++++++++++++++++++++++++

Java arrays are a special form of objects.  They have no real methods as they
are not extendable.  To help in accessing the additonal special methods associated 
with an array, Java array instances have an additional field ``__javaarray__`` of
type ``PyJParray``.

``jstring``
+++++++++++++++++++++++++++++++

For most practical purposes Java strings are treated as objects.  However, they 
also need to be able to interact with Python strings.  In the previous version,
strings were automatically converted to Python strings on return.  This resulted
in rather strange behavior when interacting with the methods of ``java.lang.String``
as rather than getting the expected Java object for chaining of commands, the 
string object would revert to Python.  To avoid this fate, we now require string
objects to be convert manually with the ``str()`` method in Python.  There are 
still places where the conversion will trigger automatically such as pushing the 
string into string substitution.  This does generate some potential for errors 
especially since it makes order important when using the equals operator when 
comparing a Java and Python string.  Also it causes minor issues when using a 
Java string as a key to a dict.  There is no special fields associated with 
the jstring.


Bootstrapping
~~~~~~~~~~~~~

The most challenging part in working with the jpype module other than the 
need to support both major Python versions with the same codebase is the 
bootstrapping of resources.  In order to get the system working we must pass 
the Python resources so the ``_jpype`` CPython module can acquire resources and then
construct the wrappers for ``java.lang.Object`` and ``java.lang.Class``.  The key
difficulty is that we need reflection to get methods from Java and those
are part of ``java.lang.Class``, but class inherits from ``java.lang.Object``.
Thus Object and the interfaces that Class inherits must all be created
blindly.  To support this process, a partial implmentation of the class
reflection is implemented in ``PyJPClass``.

The bootstrapping issue currently prevents further simplification of the 
internal layer as we need these hard coded support paths.  To help keep 
the order of the bootstrapping consistent and allow the module to load before
the JVM is started, actions are delayed in the jpype module.  Those
delayed actions are placing in initialize routines that are automatically
called once the JVM is started.

Where accessing the class instance is required while building the class,
the module globals are checked.  If these globals are not yet loaded, 
the class structure can't be accessed.  This means that ``java.lang.Object``,
``java.lang.Class``, and a few interfaces don't get the full class wrapper 
treatment.  Fortunately, these classes are pretty light and don't contain
addition resources such as inner classes that would require full reflection.

Factories
~~~~~~~~~

The key objects exposed to the user (``JClass``, ``JObject``, and ``JArray``) are each 
factory meta classes.  These classes serve as the gate keepers to creating the
meta classes or object instances.  We found only one working pattern that 
support both Python versions.  The pattern requires the class to have a 
polymorphic ``__new__`` function that depends on the arguments called.  When called 
to access the factory, the ``__new__`` method redirects to the meta producing 
factory function.  If called with any other arguments, falls to the correct 
``__new__`` super method.

When dealing with these remember they are called typically three ways.   The
user calls with the specified arguments to create a resource.  The factory
calls the ``__new__`` method when creating an instance of the derived object.  And
the C++ wrapper calls the method with internally construct resource such as
``PyJPClass`` or ``PyJPValue``.  Deciding which of the three ways it has been called
is usually simple, but it does constrain the operation of the factories as
conflicts in the three paths would lead to a failure.  Forcing keyword arguments
for one of the paths could be used to resolve the dependency.

For example, if ``JClass`` was called with a string argument it from the user.  
If it was called with a ``PyJPClass``, it came from internal module.  If called
with a class name, tuple, and list of members, it was a request from Python
to create a new dynamic type.  As the first two formulas have only one argument,
both transfer the factory dispatch to create the dynamic resource.  The other
method transfers to the Python type object to create the actual class instance.

Style
~~~~~

One of the key aspects of the jpype design is elegance of the factory patterns.
Rather than expose the user a large number of distinct concepts with different
names, the factories provide powerfull functionality with the same syntax for 
related things.  Boxing a primitive, casting to a specific type, and creating 
a new object are all tied together in one factory, ``JObject``.  By also making that 
factory an effective base class, we allow it to be used for issubtype and isinstance.

This philosophy is further enhanced by silent customizers which integrate 
Python functionality into the wrappers such that Java classes can be used
effectively with Python syntax.  Consistent use and misuse of Python concepts
such as ``with`` for defining blocks such as try with resources and synchronized
hide the underlying complexity and give the feeling to the user that the
module is integrated completely as a solution such as jython.

When adding a new feature to the Python layer, consider carefully if the 
feature needs to be exposed a new function or if it can be hidden in the 
normal Python syntax.

JPype does somewhat break the Python naming conventions.  Because Java and
Python have very different naming schemes, at least part of the kit would
have a different convention.  To avoid having one portion break Python conventions
and another part conform, we choose to use Java notation consistently 
throughout.  Package names should be lower with underscores, classes should
camel case starting upper, functions and method should be camel case starting
lower.  All private methods and classes start with a leading underscore 
and are not exported.


``_jpype`` CPython module
--------------------------

Diving deeper into the onion, we have the Python front end.  This is divided
into a number of distinct pieces.  Each piece is found under ``native/python``
and is named according to the piece it provides.  For example,
``PyJPModule`` is found in the file ``native/python/pyjp_module.cpp``

Earlier versions of the module had all of the functionality in the 
modules global space.  This functionality is now split into a number 
of classes.  These classes each have a constructor that is used to create
an instance which will correspond to a Java resource such as class, array,
method, or value.

``PyJPModule`` module
~~~~~~~~~~~~~~~~~~~~~~

This is the front end for all the global functions required to support the 
Python native portion.  Most of the functions provided in teh module are
for control and auditing.  

One important method is the setResource command.  The ``setResource`` takes a 
name and a Python function or class, and passes it to ``jp_pythonenv.cpp``.  Prior
to using duck typing to recognize jpype entities, a large number of 
resources had to be loaded to function.  With the rewrite this has been 
reduced considerably to just function required to create a Python wrapper for 
a Java class, ``GetClassMethod``.  However, now that the kit has been streamlined
additional Python resources will likely be required for new features.

``PyJPClass`` class
~~~~~~~~~~~~~~~~~~~

This class supplied the portion of the reflection API required to create 
classes without the aid of the ``java.lang.Class`` structure.


CPython API layer
------------------

To make creation of the C++ layer easier a thin wrapper over the CPython API was 
developed.  This layer provided for handling the CPython referencing using a 
smart pointer, defines the exception handling for Python, and provides resource
hooks for duck typing of the ``_jpype`` classes.

This layer is located with the rest of the Python codes in ``native/python``, but
has the prefix ``JPPy`` for its classes.  As the bridge between Python and C++, 
these support classes appear in both the ``_jpype`` CPython module and the C++
JNI layer.


Exception handling
~~~~~~~~~~~~~~~~~~

A key piece of the jpype interaction is the transfer of exceptions from 
Java to Python.  To accomplish this Python method that can result in a call to 
Java must have a ``try`` block around the contents of the function.  

We use a routine pattern of code to interact with Java to achieve this: ::

    PyObject* dosomething(PyObject* self, PyObject* args)
    {
       // Tell the logger where we are
       JP_TRACE_IN("dosomething");

       // Start a block to catch Java emitted errors
       try 
       {
          // Make sure there is a jvm to receive the call.
          ASSERT_JVM_RUNNING("dosomething");

          // Make a resource to capture any Java local references
          JPJavaFrame frame;

          // Call our Java methods
          ...

          // Return control to Python
          return obj.keep();
       }

       // Use the standard catch to transfer any exceptions back
       // to Python
       PY_STANDARD_CATCH;

       // Close out tracing
       JP_TRACE_OUT;
    }

All entry points from Python into ``_jpype`` should be guarded with this pattern.

There are exceptions to this pattern such as removing the logging, operating on 
a call that does not need the jvm running, or operating where the frame is 
already supported by the method being called.


Python referencing
~~~~~~~~~~~~~~~~~~

One of the most miserable aspects of programming with CPython is the relative 
inconsistancy of referencing.  Each method in Python may use a Python object or steal
it, or it may return a borrowed reference or give a fresh reference.  Similar
command such as getting an element from a list and getting an element from a tuple 
can have different rules.  Because the was a constant source of bugs requiring 
consultation of the Python manual for every line of code, we wrapped all of the
Python calls we were required to work with in ``jp_pythontypes``.  

Included in this wrapper is a Python reference counter called ``JPPyObject``.  
Whenever an object is returned from Python it is immediately placed in smart 
pointer ``JPPyObject`` with the policy that it was created with such as 
``use_``, ``borrowed_``, ``claim_`` or ``call_``.

``use_`` 
  This policy means that the reference counter needs to be incremented and the start 
  and the end.  We must reference it because it we don't and some Python call
  destroys the refernce out from under us, the system may crash and burn.

``borrowed_``
  This policy means we were to be give a borrowed reference that we are expected
  to reference and unreference when complete, but the command that returned it 
  can fail.  Thus before reference it, the system must check if an error has
  occurred.  If there is an error, it is promoted to an exception.

``claim_``
  This policy is used when we are given a new object with is already referenced
  for us.  Thus we are to steal the reference for the duration of our use and
  then dereference when we are done to keep it from leaking.

``call_``
  This policy both steals the reference and verifies there were no errors
  prior to continuing.  Errors are promoted to exceptions when this reference 
  is created.

If we need to pass an object which is held in a smart pointer to Python
which requires a reference, we call keep on the reference which transfers
control to a ``PyObject*`` and prevents the pointer from removing the reference.
As the object handle is leaving our control keep should only be called the
return statement.


C++ JNI layer
-------------

The C++ layer has a number of tasks.  It is used to load thunks, call jni
methods, provide reflection of classes, determine if a conversion is possible,
perform conversion, match arguments to overloads, and convert return values
back to Java.

Type wrappers
~~~~~~~~~~~~~

Each Java type has a C++ wrapper class.  These classes provide a number of methods.
Primitives each have their own unit type wrapper.  Object, arrays, and class 
instances share a C++ wrapper type.  Special instances are used for 
``java.lang.Object`` and ``java.lang.Class``.  The type wrapper are named for the class
they wrap such as ``JPIntType``.  

Type conversion
++++++++++++++++

For type conversion, a C++ class wrapper provides four methods.

``canConvertToJava``
  This method must consult the supplied Python object to determine the type
  and then make a determination of whether a conversion is possible. 
  It reports ``none_`` if there is no possible conversion, ``explicit_`` if the
  conversion is only acceptable if forced such as returning from a proxy,
  ``implicit_`` if the conversion is possible and acceptable as part of an 
  method call, or ``exact_`` if this type converts without ambiguity.  It is excepted
  to check for something that is already a Java resource of the correct type
  such as ``JPValue``, or something this is implementing the behavior as an interface
  in the form of a ``JPProxy``.

``convertToJava``
  This method consults the type and produces a conversion.  The order of the match
  should be identical to the ``canConvertToJava``.  It should also handle values and
  proxies.

``convertToPythonObject``
  This method takes a jvalue union and converts it to the corresponding 
  Python wrapper instance.  

``getValueFromObject``
  This converts a Java object into a ``JPValue`` corresponding.  This unboxes
  primitives.

Array conversion
++++++++++++++++++

In addition to converting single objects, the type rewrappers also serve as the
gateway to working with arrays of the specified type.  Five methods are used to 
work with arrays:  ``newArrayInstance``, ``getArrayRange``, ``setArrayRange``, 
``getArrayItem``, and ``setArrayItem``.  

Invocation and Fields
++++++++++++++++++++++

To convert a return type produced from a Java call, each type needs to be
able to invoke a method with that return type.  This corresponses the underlying
JNI design.  The methods invoke and invokeStatic are used for this purpose.  
Similarly accessing fields requires type conversion using the methods
``getField`` and ``setField``.

Instance verses Type wrappers
+++++++++++++++++++++++++++++++

Instances of individual Java classes are made from ``JPClass``.  However, two
special sets of conversion rules are required.  These are in the form 
of specializations ``JPObjectBaseClass`` and ``JPClassBaseClass`` corresponding
to ``java.lang.Object`` and ``java.lang.Class``.

Support classes
~~~~~~~~~~~~~~~

In addition to the type wrappers, there are several support classes. These are:

``JPTypeManager``
  The typemanager serves as a dict for all type wrappers created during the 
  operation.  

``JPReferenceQueue``
  Lifetime manager for Java and Python objects.

``JPProxy``
  Proxies implement a Java interface in Python.

``JPClassLoader``
  Loader for Java thunks.

``JPEncoding``
  Decodes and encodes Java UTF strings.

``JPTypeManager``
++++++++++++++++++

C++ typewrappers are created as needed.  Instance of each of the 
primitives along with ``java.lang.Object`` and ``java.lang.Class`` are preloaded.  
Additional instances are created as requested for individual Java classes.
Currently this is backed by a C++ map of string to class wrappers.  

The typemanager provides a number lookup methods.  ::

  // Call from within Python
  JPClass* JPTypeManager::findClass(const string& name)

  // Call from a defined Java class
  JPClass* JPTypeManager::findClass(jclass cls)

  // Call used when returning an object from Java
  JPClass* JPTypeManager::findClassForObject(jobject obj)

``JPReferenceQueue``
++++++++++++++++++++

When a Python object is presented to Java as opposed to a Java object, the 
lifespan of the Python object must be extended to match the Java wrapper.  
The reference queue adds a reference to the Python object that will be 
removed by the Java layer when the garbage collection deletes the wrapper.
This code is almost entirely in the Java library, thus only the portion 
to support Java native methods appears in the C++ layer.

Once started the reference queue is mostly transparent.  registerRef is used
to bind a Python object live span to a Java object. ::

  void JPReferenceQueue::registerRef(jobject obj, PyObject* hostRef)

``JPProxy``
++++++++++++

In order to call Python functions from within Java, a Java proxy is used. The
majority of the code is in Java.  The C++ code holds the Java native portion.
The native implement of the proxy call is the only place in with the pattern
for reflecting Python exceptions back into Java appears.

As all proxies are ties to Python references, this code is strongly tied to
the reference queue.

``JPClassLoader``
++++++++++++++++++

This code is responsible for loading the Java class thunks.  AS it is difficult
to ensure we can access a Java jar from within Python, all Java native code
is stored in a binary thunk compiled into the C++ layer as a header.  The 
class loader provides a way to load this embedded jar first by bootstrapping
a custom Java classloader and then using that classloader to load the internal
jar.

The classloader is mostly transparent.  It provides one method called findClass
which loads a class from the internal jar. ::
  
  jclass JPClassLoader::findClass(string name)


``JPEncoding``
+++++++++++++++

Java concept of UTF is pretty much orthagonal to the rest of the world.  Java
used 16 bits for its native characters.  But this was inadequate for all of the
unicode characters, thus longer unicode character had to be encoded in the 
16 bit space.  Rather the directly providing methods to convert to a standard
encoding such as UTF8, Java used UTF16 encoded in 8 bits.  ``JPEncoding`` deals
with converting this unusual encoding into something that Python can understand.

The key method in this module is transcribe with signature ::

  std::string transcribe(const char* in, size_t len,
      const JPEncoding& sourceEncoding,
      const JPEncoding& targetEncoding)

There are two encodings provided, ``JPEncodingUTF8`` and ``JPEncodingJavaUTF8``.  
By selecting the source and traget encoding transcribe can covert to or
from Java to Python encoding.


Java native code
----------------

At the lowest level of the onion is the native Java layer.  Although this 
layer is most remote from Python, ironically it is the easiest layer to communicate
with.  As the point of jpype is to communicate with Java, it is possible to 
directly communicate with the jpype Java internals.  These can be imported
from the package ``org.jpype``.  The code for the Java layer is located in
``native/java``.  It is compiled into a jar in the build directory and then converted
to a C++ header to be compiled into the ``_jpype`` module.

The Java layer currently houses the reference queue, a classloader which can
load a Java class from a bytestream source, the proxy code for implementing
Java interfaces, and a memory compiler module which allows Python to directly 
create a class from a string.

  
Tracing
---------

Because the relations between the layers can be daunting especially when things
go wrong.  The CPython and C++ layer have a built in logger.  This logger 
must be enabled with a compiler switch to activate.  To active the logger, touch 
one of the cpp files in the native directory to mark the build as dirty, then
compile the ``jpype`` module with: ::

     python setup.py --enable-tracing devel

Once built run a short test program that demonstrates the problem and capture the
output of the terminal to a file.  This should allow the developer to isolate 
the fault to specific location where it failed.

To use the logger in a function start the ``JP_TRACE_IN(function_name)`` which will 
open a ``try catch`` block.


Future directions
-----------------

Although the majority of the code has been reworked for JPype 0.7, there is still
further work to be done.  Almost all Java constructs can be exercised from within
Python, but Java and Python are not static.  Thus, we are working on further 
improvements to the jpype core focusing on making the package faster, more
efficient, and easier to maintain.  This section will discuss a few of these options.

Java based code is much easier to debug as it is possible to swap the thunk code 
with an external jar.  Further, Java has much easier management of resources.  
Thus pushing a portion of the C++ layer into the Java layer could further reduce
the size of the code base.  In particular, deciding the order of search for 
method overloads in C++ attempts to reconstruct the Java overload rules.  But these
same rules are already available in Java. Further, the C++ layer is designed
to make many frequent small calls to Java methods.  This is not the preferred
method to operate in JNI.  It is better to have specialized code in Java which
preforms large tasks such as collecting all of the fields needed for a type 
wrapper and passing it back in a single call, rather than call twenty different
general purpose methods.  This would also vastly reduce the number of ``jmethods``
that need to be bound in the C++ layer.

The world of JVMs is currently in flux.  Jpype needs to be able to support 
other JVMs.  In theory, so long a JVM provides a working JNI layer, there 
is no reason the jpype can't support it.  But we need loading routines for 
these JVMs to be developed.
