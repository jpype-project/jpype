Developer Guide
===============

Overview
--------

This document describes the guts of jpype. It is intended
lay out the architecture of the jpype code to aid intrepid lurkers
to develop and debug the jpype code once I am run over by a bus.
For most of this document I will use the royal we, except where
I am giving personal opinions expressed only by yours truly, the
author Thrameos.


History
~~~~~~~

When I started work on this project it had already existed for over 10 years.
The original developer had intended a much larger design with modules to
support multiple languages such as Ruby. As such it was constructed with
three layers of abstraction. It has a wrapper layer over Java in C++, a
wrapper layer for the Python api in C++, and an abstraction layer intended
to bridge Python and other interpreted languages. This multilayer abstraction
ment that every debugging call had to drop through all of those layers.
Memory management was split into multiple pieces with Java controlling a
portion of it, C++ holding a bunch of resources, Python holding additional
resources, and HostRef controlling the lifetime of objects shared between the
layers. It also had its own reference counting system for handing Java
references on a local scale.

This level of complexity was just about enough to scare off all but the most
hardened programmer. Thus I set out to eliminate as much of this as I could.
Java already has its own local referencing system to deal in the form of
LocalFrames. It was simiply a matter of setting up a C++ object to
hold the scope of the frames to eliminate that layer. The Java abstraction
was laid out in a fashion somewhat orthagonally to the Java inheritance
diagram. Thus that was reworked to something more in line which could be
safely completed without disturbing other layers. The multilanguage
abstraction layer was already pierced in multiple ways for speed. However,
as the abastraction interwove throughout all the library it was a terrible
lift to remove and thus required gutting the Python layer as well to support
the operations that were being performed by the HostRef.

The remaining codebase is fairly slim and reasonably streamlined. This
rework cut out about 30% of the existing code and sped up the internal
operations. The Java C++ interface matches the Java class hierachy.


Architecture
~~~~~~~~~~~~

JPype is split into several distinct pieces.

``jpype`` Python module
  The majority of the front end logic for the toolkit is in Python jpype module.
  This module deals with the construction of class wrappers and control functions.
  The classes in the layer are all prefixed by ``J``.

``_jpype`` CPython module
  The native module is supported by a CPython module called ``_jpype``. The ``_jpype``
  module is located in ``native/python`` and has C style classes with a prefix ``PyJP``.

  This CPython layer acts as a front end for passing to the C++ layer.
  It performs some error checking. In addition to the module functions in
  ``PyJPModule``, the module has multiple Python classes to support the native jpype
  code such as ``PyJPClass``, ``PyJPArray``, ``PyJPValue``, ``PyJPValue``, etc.

CPython API wrapper
  In addition to the exposed Python module layer, there is also a C++ wrapper
  for the Python API. This is located in ``native/python`` and has the prefix
  ``JPPy`` for all classes.

  There are two parts of this api wrapper, ``jp_pythonenv`` and ``jp_pythontypes``.
  The ``jp_pythonenv`` holds all of the resources that C++ needs to communicate
  with the jpype native module. ``jp_pythontypes`` wraps the required parts of
  the CPython API in C++ for use in the C++ layer.

C++ JNI layer
  The guts that drive Java are in the C++ layer located in ``native/common``. This layer
  has the namespace ``JP``. The code is divided into wrappers for each Java type,
  a typemanager for mapping from Java names to class instances, support classes
  for proxies, and a thin JNI layer used to help ensure rigerous use of the same
  design patterns in the code. The primary responsibility of this layer is
  type conversion and matching of method overloads.

Java layer
  In addition to the C++ layer, jpype has a native Java layer. This code
  is compiled as a "thunk" which is loaded into the JVM in the form of a
  a binary stored as a string. Code for Java is found in ``native/java``.
  The Java layer is divided into two parts,
  a bootstrap loader and a jar containing the support classes. The Java
  layer is responsible managing the lifetime of shared Python, Java, and C++ objects.


``jpype`` module
-----------------

The ``jpype`` module itself is made of a series of support classes which
act as factories for the individual wrappers that are created to mirror
each Java class. Because it is not possible to wrap all Java classes
with staticly created wrappers, instead jpype dynamically creates
Python wrappers as requested by the user.

The wrapping process is triggered in two ways. The user can manually
request creating a class by importing a class wrapper with jpype.imports
or ``JPackage`` or by manually invoking it with ``JClass``. Or the class wrapper
can be created automatically as a result of a return type or exception
thrown to the user.

Because the classes are created dynamically, the class structure
uses a lot of Python meta programming. As the programming
model for Python 2.7 and 3 series are rather different, the
exact formuation is restricted to a set of common formulations
that are shared between the versions.

Each class wrapper derives from the class wrappers of each of the
wrappers corresponding to the Java classes that each class extends
and implements. The key to this is to hacked ``mro``. The ``mro``
orders each of the classes in the tree such that the most drived
class methods are exposed, followed by each parent class. This
must be ordered to break ties resulting from multiple inheritance
of interfaces.

The ``mro`` has one aspect that breaks the Python object model. Normally
it is a requirement that every class that inherits from another class
must inherit all of the previous parents. However, Java has two distinct
types of inheritance, extension and implementation. As such we delete
the ``JInterface`` parent from all concrete class implementation during
the mro resolution phase.

resource types
~~~~~~~~~~~~~~

jpype objects work with the inner layers primarily through duck typing
using a series of special fields. These fields correspond to a
JNI type such as ``jvalue``, ``jclass``, ``jobject``, and ``jstring``. But as these
resources cannot be held directly, they are provided as resources exposed
as ``_jpype.PyJP`` classes.

``jvalue``
++++++++++

In the earlier design, wrappers, primitives and objects were all seperate
concepts. At the JNI layer these are unified by a common element called
jvalue. A ``jvalue`` is a union of all primitives with the jobject. The jobject
can represent anything derived from Java object including the pseudo class
jstring. To represent these in a common way all jpype objects that
represent an instance of a Java resource have a ``__javavalue__`` field which
should be of type ``_jpype.PyJPValue``. Rather than forcing checking of individual
types, we use duck typing of simply checking for a ``PyJPValue`` in this field.
In addition to the union, the jvalue also carries a hidden type. Thus hidden
type is used to help in casting the object and resolving method overloads.
We will discuss this object further in the CPython section.

``jclass``
++++++++++

In addition class wrappers have a ``_jpype.PyJPClass`` field which represents the
internal class wrapper. This class wrapper holds the reflection api used to create
the class methods and fields. This field is stored as the ``__javaclass__``
in the class wrapper. As the class wrapper is used to create an object instance
``__javaclass__`` also appears in the objects. Only objects that have a ``__javaclass__``
and lack a ``__javavalue__`` are treated as class wrappers for the purposes of
duck typing.

Because the Java class is both an object and type, we used the duck typing to
allow the class pointer to be converted into a class instance. This is exposed
as property called ``class_``. The ``class_`` property is the equivalent of the
.class member of a Java class name or ``getClass()`` of a class instance. As it
is an object instance of ``java.lang.Class`` it can be used for any reflection
needs by the user. However, when working with the jpype core module, on
needs to be aware of that the this class instance is not available until
the key wrappers are created. See bootstrapping_ for further details.

``jarray``
++++++++++

Java arrays are a special form of objects. They have no real methods as they
are not extendable. To help in accessing the additonal special methods associated
with an array, Java array instances have an additional field ``__javaarray__`` of
type ``PyJParray``.

``jstring``
+++++++++++

For most practical purposes Java strings are treated as objects. However, they
also need to be able to interact with Python strings. In the previous version,
strings were automatically converted to Python strings on return. This resulted
in rather strange behavior when interacting with the methods of ``java.lang.String``
as rather than getting the expected Java object for chaining of commands, the
string object would revert to Python. To avoid this fate, we now require string
objects to be convert manually with the ``str()`` method in Python. There are
still places where the conversion will trigger automatically such as pushing the
string into string substitution. This does generate some potential for errors
especially since it makes order important when using the equals operator when
comparing a Java and Python string. Also it causes minor issues when using a
Java string as a key to a ``dict``. There is no special fields associated with
the ``jstring``.

.. _bootstrapping:

Bootstrapping
~~~~~~~~~~~~~

The most challenging part in working with the jpype module other than the
need to support both major Python versions with the same codebase is the
bootstrapping of resources. In order to get the system working, we must pass
the Python resources so the ``_jpype`` CPython module can acquire resources and then
construct the wrappers for ``java.lang.Object`` and ``java.lang.Class``. The key
difficulty is that we need reflection to get methods from Java and those
are part of ``java.lang.Class``, but class inherits from ``java.lang.Object``.
Thus Object and the interfaces that Class inherits must all be created
blindly. To support this process, a partial implmentation of the class
reflection is implemented in ``PyJPClass``.

The bootstrapping issue currently prevents further simplification of the
internal layer as we need these hard coded support paths. To help keep
the order of the bootstrapping consistent and allow the module to load before
the JVM is started, actions are delayed in the jpype module. Those
delayed actions are placing in initialize routines that are automatically
called once the JVM is started.

Where accessing the class instance is required while building the class,
the module globals are checked. If these globals are not yet loaded,
the class structure can't be accessed. This means that ``java.lang.Object``,
``java.lang.Class``, and a few interfaces don't get the full class wrapper
treatment. Fortunately, these classes are pretty light and don't contain
addition resources such as inner classes that would require full reflection.

Factories
~~~~~~~~~

The key objects exposed to the user (``JClass``, ``JObject``, and ``JArray``) are each
factory meta classes. These classes serve as the gate keepers to creating the
meta classes or object instances. We found only one working pattern that
support both Python versions. The pattern requires the class to have a
polymorphic ``__new__`` function that depends on the arguments called. When called
to access the factory, the ``__new__`` method redirects to the meta producing
factory function. If called with any other arguments, falls to the correct
``__new__`` super method.

When dealing with these remember they are called typically three ways.  The
user calls with the specified arguments to create a resource. The factory
calls the ``__new__`` method when creating an instance of the derived object. And
the C++ wrapper calls the method with internally construct resource such as
``PyJPClass`` or ``PyJPValue``. Deciding which of the three ways it has been called
is usually simple, but it does constrain the operation of the factories as
conflicts in the three paths would lead to a failure. Forcing keyword arguments
for one of the paths could be used to resolve the dependency.

For example, if ``JClass`` was called with a string argument it originates from the user.
If it was called with a ``PyJPClass``, it came from internal module. If called
with a class name, tuple, and list of members, it was a request from Python
to create a new dynamic type. As the first two formulas have only one argument,
both transfer the factory dispatch to create the dynamic resource. The other
method transfers to the Python type object to create the actual class instance.

Style
~~~~~

One of the key aspects of the jpype design is elegance of the factory patterns.
Rather than expose the user a large number of distinct concepts with different
names, the factories provide powerfull functionality with the same syntax for
related things. Boxing a primitive, casting to a specific type, and creating
a new object are all tied together in one factory, ``JObject``. By also making that
factory an effective base class, we allow it to be used for ``issubtype`` and
``isinstance``.

This philosophy is further enhanced by silent customizers which integrate
Python functionality into the wrappers such that Java classes can be used
effectively with Python syntax. Consistent use and misuse of Python concepts
such as ``with`` for defining blocks such as try with resources and synchronized
hide the underlying complexity and give the feeling to the user that the
module is integrated completely as a solution such as jython.

When adding a new feature to the Python layer, consider carefully if the
feature needs to be exposed a new function or if it can be hidden in the
normal Python syntax.

JPype does somewhat break the Python naming conventions. Because Java and
Python have very different naming schemes, at least part of the kit would
have a different convention. To avoid having one portion break Python conventions
and another part conform, we choose to use Java notation consistently
throughout. Package names should be lower with underscores, classes should
camel case starting upper, functions and method should be camel case starting
lower. All private methods and classes start with a leading underscore
and are not exported.

Customizers
~~~~~~~~~~~

There was a major change in the way the customizers work between versions.
The previous system was undocumented and has now been removed, but as
someone may have used of it previously, we will contrast it with the
revised system so that the customizers can be converted.

In the previous system, a global list stored all customizers.
When a class was created, it went though the list and asked the class if
it matched that class name. If it matched, it altered the dict of members
to be created so when the dynamic class was finished it had the custome
behavior.  This system wasn't very scalable as each customizer added more
work to the class construction process.

The revised system works by storing a dictionary keyed to the class name.
Thus the customizer only applies to the specific class targeted to the
customizer. The customizer is specified using annotation of a prototype
class making methods automatically copy onto the class. However, sometimes
a customizer needs to be applied to an entire tree of classes such as
all classes that implement ``java.util.List``.  To handle this case,
the class creation system looks for a special method ``__java_init__``
in the tree of base classes and calls it on the newly created class.
Most of the time the customization was the same simple pattern so we
added a ``sticky`` flag to build the initialization method directly.
This method can alter the class to make it add the new behavior.  Note
the word alter. Where before we changed the member prior to creating the
class, here we are altering the class. Thus the customizer is expected
to monkey patch the existing class. There is only one pattern of
monkey patching that works on both Python 2 and Python 3 so be sure to
use the ``type.__setattr__`` method of altering the class dictionary.

It is possible to apply customizers after the class has already been
created because we operate by monkey patching. But there is a limitation
that there can only be one ``__java_init__`` method and thus two
customizers specifying a global behavior on the same class wrapper will
lead to unexpected behavior.


``_jpype`` CPython module
--------------------------

Diving deeper into the onion, we have the Python front end. This is divided
into a number of distinct pieces. Each piece is found under ``native/python``
and is named according to the piece it provides. For example,
``PyJPModule`` is found in the file ``native/python/pyjp_module.cpp``

Earlier versions of the module had all of the functionality in the
modules global space. This functionality is now split into a number
of classes. These classes each have a constructor that is used to create
an instance which will correspond to a Java resource such as class, array,
method, or value.

``PyJPModule`` module
~~~~~~~~~~~~~~~~~~~~~~

This is the front end for all the global functions required to support the
Python native portion. Most of the functions provided in the module are
for control and auditing.

One important method is the setResource command. The ``setResource`` takes a
name and a Python function or class, and passes it to ``jp_pythonenv.cpp``. Prior
to using duck typing to recognize jpype entities, a large number of
resources had to be loaded to function. With the rewrite this has been
reduced considerably to just function required to create a Python wrapper for
a Java class, ``GetClassMethod``. However, now that the kit has been streamlined
additional Python resources will likely be required for new features.

``PyJPClass`` class
~~~~~~~~~~~~~~~~~~~

This class supplied the portion of the reflection API required to create
classes without the aid of the ``java.lang.Class`` structure.  It can be constructed
either from within JPype or directly from Python.  It points to a 
``JPClass``.  It has methods for diagnostics and reflection.


``PyJPMethod`` class
~~~~~~~~~~~~~~~~~~~~

This class acts as descriptor with a call method.  As a descriptor accessing its
methods through the class will trigger its ``__get__`` function, thus
getting ahold of it within Python is a bit tricky.  The ``__get__`` mathod
is used to bind the static unbound method to a particular object instance
so that we can call with the first argument as the ``this`` pointer.

It has some reflection and diagnostics methods that can be useful
it tracing down errors. The beans methods are there just to support
the old properties API.

The naming on this class is a bit deceptive. It does not correspond
to a single method but rather all the overloads with the same name.  
When called it passes to with the arguments to the C++ layer where
it must be resolved to a specific overload. 

This class is stored directly in the class wrappers.


``PyJPField`` class
~~~~~~~~~~~~~~~~~~~

This class is a descriptor with ``__get__`` and ``__set__`` methods.
When called at the static class layer it operates on static fields.  When
called on a Python object, it binds to the object making a ``this`` pointer.
If the field is static, it will continue to access the static field, otherwise,
it will provide access to the member field. This trickery allows both
static and member fields to wrap as one type.

This class is stored directly in the class wrappers.

``PyJPArray`` class
~~~~~~~~~~~~~~~~~~~

This class serves as to provide the extra methods that are needed for 
working with Java arrays. It is not a descriptor and thus is hidden in
the python class as ``__javaarray__``.  A Python array wrapper should have
both the ``__javavalue__`` and the ``__javaarray__`` field to function.


``PyJPMonitor`` class
~~~~~~~~~~~~~~~~~~~~~

This class provides ``synchronized`` to JPype.  Instances of this
class are created and held using ``with``.  It has two methods 
``__enter__`` and ``__exit__`` which hook into the Python RAII
system.  


``PyJPValue`` class
~~~~~~~~~~~~~~~~~~~

This class holds all types of Java resources that correspond to the Java
jvalue union.  This includes both objects and primitives.  It provides 
a cache for string conversions so the when ``str()`` is called on a
Java string we only pay for the conversion cost once.  Both Java and 
Python strings are immutable thus this is a valid cache operation.

Unlike ``jvalue`` we hold the object type in the C++ ``JPValue``
object.  The class reference is used to determine how to match the arguments
to methods. The class may not correspond to the actual class of the 
object. Using a class other than the actual class serves to allow
an object to be cast and thus treated like another type for the purposes
of overloading. This mechanism is what allows the ``JObject`` factory
to perform a typecast.

``PyJPValue`` is held in the private python field ``__javavalue__``.


CPython API layer
------------------

To make creation of the C++ layer easier a thin wrapper over the CPython API was
developed. This layer provided for handling the CPython referencing using a
smart pointer, defines the exception handling for Python, and provides resource
hooks for duck typing of the ``_jpype`` classes.

This layer is located with the rest of the Python codes in ``native/python``, but
has the prefix ``JPPy`` for its classes. As the bridge between Python and C++,
these support classes appear in both the ``_jpype`` CPython module and the C++
JNI layer.


Exception handling
~~~~~~~~~~~~~~~~~~

A key piece of the jpype interaction is the transfer of exceptions from
Java to Python. To accomplish this Python method that can result in a call to
Java must have a ``try`` block around the contents of the function.

We use a routine pattern of code to interact with Java to achieve this:

.. code-block:: cpp 

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
inconsistancy of referencing. Each method in Python may use a Python object or steal
it, or it may return a borrowed reference or give a fresh reference. Similar
command such as getting an element from a list and getting an element from a tuple
can have different rules. This was a constant source of bugs requiring
consultation of the Python manual for every line of code. Thus we wrapped all of the
Python calls we were required to work with in ``jp_pythontypes``.

Included in this wrapper is a Python reference counter called ``JPPyObject``.
Whenever an object is returned from Python it is immediately placed in smart
pointer ``JPPyObject`` with the policy that it was created with such as
``use_``, ``borrowed_``, ``claim_`` or ``call_``.

``use_``
  This policy means that the reference counter needs to be incremented and the start
  and the end. We must reference it because if we don't and some Python call
  destroys the refernce out from under us, the system may crash and burn.

``borrowed_``
  This policy means we were to be give a borrowed reference that we are expected
  to reference and unreference when complete, but the command that returned it
  can fail. Thus before reference it, the system must check if an error has
  occurred. If there is an error, it is promoted to an exception.

``claim_``
  This policy is used when we are given a new object with is already referenced
  for us. Thus we are to steal the reference for the duration of our use and
  then dereference when we are done to keep it from leaking.

``call_``
  This policy both steals the reference and verifies there were no errors
  prior to continuing. Errors are promoted to exceptions when this reference
  is created.

If we need to pass an object which is held in a smart pointer to Python
which requires a reference, we call keep on the reference which transfers
control to a ``PyObject*`` and prevents the pointer from removing the reference.
As the object handle is leaving our control keep should only be called the
return statement.  The smart pointer is not used on method passing in which
the parent explicitly holds a reference to the Python object. As all tuples
passed as arguments operate like this, that means much of the API accepts
bare ``PyObject*`` as arguments.  It is the job of the caller to hold the
reference for its scope.

On CPython extensions
~~~~~~~~~~~~~~~~~~~~~

CPython is somewhat of a nightmare to program in. It is not that they did not
try to document the API, but it is darn complex. The problems extend well
beyond the reference counting system that we have worked around.  In
particular, the object model though well developed is very complex, often to
get it to work you must follow letter for letter the example on the CPython
user guide, and even then it may all go into the ditch.

The key problem is that there are a lot of very bad examples of how to write
CPython extension modules out there. Often the these examples bypass the
appropriate macro and just call the field, or skip the virtual table and try to
call the Python method directly. It is true that these things do not break
there example, but they are conditioned on these methods they are calling
directly to be the right one for the job, but depends a lot on what the
behavior of the object is supposed to be. Get it wrong and you get really nasty
segfault.

CPython itself may be partly responsible for some of these problems.  They
generally seem to trust the user and thus don't verify if the call makes sense.
It is true that it will cost a little speed to be aggressive about checking the
type flags and the allocator match, but not checking when the error happens,
means that it fails far from the original problem source. I would hope that we
have moved beyond the philosophy that the user should just to whatever they
want so it runs as fast as possible, but that never appears to be the case. Of
course, I am just opining from the outside of the tent and I am sure the issues
are much more complicated it appears superficially. Then again if I can manage
to provide a safe workspace while juggling the issues of multiple virtual
machines, I am free to have opinions on the value of trading performance and
safety.

In short when working on the extension code, make sure you do everything by the
book, and check that book twice. Always go through the types virtual table and
use the propery macros to access the resources. Miss one line in some complex
pattern even once and you are in for a world of hurt. There are very few guard
rails in the CPython code.


C++ JNI layer
-------------

The C++ layer has a number of tasks. It is used to load thunks, call JNI
methods, provide reflection of classes, determine if a conversion is possible,
perform conversion, match arguments to overloads, and convert return values
back to Java.

Memory management
~~~~~~~~~~~~~~~~~

Java provides built in memory management for controlling the lifespan of 
Java objects that are passed through JNI. When a Java object is created
or returned from the JVM it returns a handle to object with a reference
counter. To manage the lifespan of this reference counter a local frame
is created. For the duration of this frame all local references will
continue to exist. To extend the lifespan either a new global reference
to the object needs to be created, or the object needs to be kept.  When
the local frame is destroyed all local references are destroyed with 
the exception of an optional specified local return reference.  

We have wrapped the Java reference system with the wrapper ``JPLocalFrame``.
This wrapper has three functions. It acts as a RAII (Resource acquisition
is initialization) for the local frame. Further, as creating a local
frame requires creating a Java env reference and all JNI calls require
access to the env, the local frame acts as the front end to call all
JNI calls. Finally as getting ahold of the env requires that the 
thread be attached to Java, it also serves to automatically attach
threads to the JVM. As accessing an unbound thread will cause a segmentation
fault in JNI, we are now safe from any threads created from within
Python even those created outside our knowledge.  (I am looking at 
you spyder)

Using this pattern makes the JPype core safe by design.  Forcing JNI
calles to be called using the frame ensures:

  - Every local reference is destroyed.
  - Every thread is properly attached before JNI is used.
  - The pattern of keep only one local reference is obeyed.

To use a local frame, use the pattern shown in this example.

.. code-block:: cpp

    jobject doSomeThing(std::string args)
    {
        // Create a frame at the top of the scope
        JPLocalFrame frame;

        // Do the required work
        jobject obj =frame.CallObjectMethodA(globalObj, methodRef, params);

        // Tell the frame to return the reference to the outer scope.
        //   once keep is called the frame is destroyed and any
        //   call will fail.
        return frame.keep(obj);
    }

Note that the value of the object returned and the object in the function
will not be the same. The returned reference is owned by the enclosing
local frame and points to the same object. But as its lifespan belongs
to the outer frame, its location in memory is different.  You are allowed
to ``keep`` a reference that was global or was passed in, in either of
those case, the outer scope will get a new local reference that points
to the same object. Thus you don't need to track the origin of the object.

The changing of the value while pointing is another common problem.
A routine error is to get a local reference, call ``NewGlobalRef``
and then keeping the local reference rather than the shiny new
global reference it made. This is not like the Python reference system
where you have the object that you can ref and unref. Thus make sure
you always store only the global reference.

.. code-block:: cpp

    jobject global;

    // we are getting a reference, may be local, may be global.
    // either way it is borrowed and it doesn't belong to us.
    void elseWhere(jvalue value)
    {
      JPLocalFrame frame;

      // Bunch of code leading us to decide we need to
      // hold the resource longer.
      if (cond)
      {
        // okay we need to keep this reference, so make a 
        // new global reference to it.
        global = frame.NewGlobalRef(value.l);
      }
    }

But don't mistake this as an invitation to make global references everywhere.
Global reference are global, thus will hold the member until the reference is
destroyed. C++ exceptions can lead to missing the unreference, thus global
references should only happen when you are placing the Java object into a class
member variable or a global variable. 

To help manage global references, we have ``JPRef<>`` which holds a global
reference for the duration of the C++ lifespace.  This is the base class for
each of the global reference types we use.

.. code-block:: cpp

    typedef JPRef<jclass> JPClassRef;
    typedef JPRef<jobject> JPObjectRef;
    typedef JPRef<jarray> JPArrayRef;
    typedef JPRef<jthrowable> JPThrowableRef;


For functions that expect the outer scope to already have created a frame
for this context, we use the pattern of extending the outer scope rather
than creating a new one.

.. code-block:: cpp

    jobject doSomeThing(JPLocalFrame& frame, std::string args)
    {
        // Do the required work
        jobject obj = frame.CallObjectMethodA(globalObj, methodRef, params);

        // We must not call keep here or we will terminate 
        // a frame we do not own.
        return obj;
    }

Although the system we have set up is "safe by design", there are things that
can go wrong is misused.  If the caller fails to create a frame prior to
calling a function that returns a local reference, the reference will go into
the program scoped local references and thus leak. Thus, it is usually best to
force the user to make a scope with the frame extension pattern. Second, if any
JNI references that are not kept or converted to global, it becomes invalid.
Further, since JNI recycles the reference pointer fairly quickly, it most
likely will be pointed to another object whose type may not be expected. Thus,
best case is using the stale reference will crash and burn. Worse case, the
reference will be a live reference to another object and it will produce an
error which seems completely irrelevant to anything that was being called.
Horrible case, the live object does not object to bad call and it all silently
proceeds down the road another two miles before coming to flaming death. 

Moral of the story, always create a local frame even if you are handling a global 
reference. If passed or returned a reference of any kind, it is a borrowed reference 
belonging to the caller or being held by the current local frame. Thus it must
be treated accordingly. If you have to hold a global use the appropraite ``JPRef``
class to ensure it is exception and dtor safe. For further information 
read ``native/common/jp_javaframe.h``.


Type wrappers
~~~~~~~~~~~~~

Each Java type has a C++ wrapper class. These classes provide a number of methods.
Primitives each have their own unit type wrapper. Object, arrays, and class
instances share a C++ wrapper type. Special instances are used for
``java.lang.Object`` and ``java.lang.Class``. The type wrapper are named for the class
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
  method call, or ``exact_`` if this type converts without ambiguity. It is excepted
  to check for something that is already a Java resource of the correct type
  such as ``JPValue``, or something this is implementing the behavior as an interface
  in the form of a ``JPProxy``.

``convertToJava``
  This method consults the type and produces a conversion. The order of the match
  should be identical to the ``canConvertToJava``. It should also handle values and
  proxies.

``convertToPythonObject``
  This method takes a jvalue union and converts it to the corresponding
  Python wrapper instance.

``getValueFromObject``
  This converts a Java object into a ``JPValue`` corresponding. This unboxes
  primitives.

Array conversion
++++++++++++++++++

In addition to converting single objects, the type rewrappers also serve as the
gateway to working with arrays of the specified type. Five methods are used to
work with arrays:  ``newArrayInstance``, ``getArrayRange``, ``setArrayRange``,
``getArrayItem``, and ``setArrayItem``.

Invocation and Fields
++++++++++++++++++++++

To convert a return type produced from a Java call, each type needs to be
able to invoke a method with that return type. This corresponses the underlying
JNI design. The methods invoke and invokeStatic are used for this purpose.
Similarly accessing fields requires type conversion using the methods
``getField`` and ``setField``.

Instance versus Type wrappers
+++++++++++++++++++++++++++++++

Instances of individual Java classes are made from ``JPClass``. However, two
special sets of conversion rules are required. These are in the form
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

C++ typewrappers are created as needed. Instance of each of the
primitives along with ``java.lang.Object`` and ``java.lang.Class`` are preloaded.
Additional instances are created as requested for individual Java classes.
Currently this is backed by a C++ map of string to class wrappers.

The typemanager provides a number lookup methods.

.. code-block:: cpp 

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

Once started the reference queue is mostly transparent. registerRef is used
to bind a Python object live span to a Java object.

.. code-block:: cpp 

  void JPReferenceQueue::registerRef(jobject obj, PyObject* hostRef)


``JPProxy``
++++++++++++

In order to call Python functions from within Java, a Java proxy is used. The
majority of the code is in Java. The C++ code holds the Java native portion.
The native implement of the proxy call is the only place in with the pattern
for reflecting Python exceptions back into Java appears.

As all proxies are ties to Python references, this code is strongly tied to
the reference queue.

``JPClassLoader``
++++++++++++++++++

This code is responsible for loading the Java class thunks. As it is difficult
to ensure we can access a Java jar from within Python, all Java native code
is stored in a binary thunk compiled into the C++ layer as a header. The
class loader provides a way to load this embedded jar first by bootstrapping
a custom Java classloader and then using that classloader to load the internal
jar.

The classloader is mostly transparent. It provides one method called findClass
which loads a class from the internal jar.

.. code-block:: cpp 

  jclass JPClassLoader::findClass(string name)


``JPEncoding``
+++++++++++++++

Java concept of UTF is pretty much out of sync with the rest of the world. Java
used 16 bits for its native characters. But this was inadequate for all of the
unicode characters, thus longer unicode character had to be encoded in the 16
bit space. Rather the directly providing methods to convert to a standard
encoding such as UTF8, Java used UTF16 encoded in 8 bits which they dub
Modified-UTF8. ``JPEncoding`` deals with converting this unusual encoding into
something that Python can understand.

The key method in this module is transcribe with signature 

.. code-block:: cpp 

  std::string transcribe(const char* in, size_t len,
      const JPEncoding& sourceEncoding,
      const JPEncoding& targetEncoding)

There are two encodings provided, ``JPEncodingUTF8`` and ``JPEncodingJavaUTF8``.
By selecting the source and traget encoding transcribe can convert to or
from Java to Python encoding.

Incidentally that same modified UTF coding is used in storing symbols in the
class files. It seems like a really poor design choice given they have to document
this modified UTF in multiple places. As far as I can tell the internal
converter only appears on ``java.io.DataInput`` and ``java.io.DataOutput``.

Java native code
----------------

At the lowest level of the onion is the native Java layer. Although this layer
is most remote from Python, ironically it is the easiest layer to communicate
with. As the point of jpype is to communicate with Java, it is possible to
directly communicate with the jpype Java internals. These can be imported from
the package ``org.jpype``. The code for the Java layer is located in
``native/java``. It is compiled into a jar in the build directory and then
converted to a C++ header to be compiled into the ``_jpype`` module.

The Java layer currently houses the reference queue, a classloader which can
load a Java class from a bytestream source, the proxy code for implementing
Java interfaces, and a memory compiler module which allows Python to directly
create a class from a string.


Tracing
---------

Because the relations between the layers can be daunting especially when things
go wrong. The CPython and C++ layer have a built in logger. This logger
must be enabled with a compiler switch to activate. To active the logger, touch
one of the cpp files in the native directory to mark the build as dirty, then
compile the ``jpype`` module with: ::

     python setup.py --enable-tracing devel

Once built run a short test program that demonstrates the problem and capture the
output of the terminal to a file. This should allow the developer to isolate
the fault to specific location where it failed.

To use the logger in a function start the ``JP_TRACE_IN(function_name)`` which will
open a ``try catch`` block.

The JPype tracer can be augmented with the Python tracing module to give
a very good picture of both JPype and Python states at the time of the crash.
To use the Python tracing, start Python with... ::

    python -m trace --trace myscript.py


Debugging issues
----------------

If the tracing function proves inadequate to identify a problem, we often need
to turn to a general purpose tool like gdb or valgrind.  The JPype core is not
easy to debug. Python can be difficult to properly monitor especially with
tools like valgrind due to its memory handling. Java is also challenging to
debug. Put them together and you have the mother of all debugging issues. There
are a number of complicating factors. Let us start with how to debug with gdb. 

Gdb runs into two major issues, both tied to the signal handler.
First, Java installs its own signal handlers that take over the entire process
when a segfault occurs. This tends to cause very poor segfault stacktraces
when examining a core file, which often is corrupt after the first user frame.
Second, Java installs its signal handlers in such as way that attempting to run
under a debugger like gdb will often immediately crash preventing one from
catching the segfault before Java catches it. This makes for a catch 22,
you can't capture a meaningful non-interactively produced core file, and you
can't get an interactive session to work.

Fortunately there are solutions to the interactive session issue. By disabling 
the SIGSEGV handler, we can get past the initial failure and also we can catch
the stack before it is altered by the JVM. ::

    gdb -ex 'handle SIGSEGV nostop noprint pass' python

Thus far I have not found any good solutions to prevent the JVM from altering
the stack frames when dumping the core. Thus interactive debugging appears
to be the best option. 

There are additional issues that one should be aware of. Open-JDK 1.8 has had a
number of problems with the debugger. Starting JPype under gdb may trigger, may
trigger the following error. ::

    gdb.error: No type named nmethod.

There are supposed to be fixes for this problem, but none worked for me.
Upgrading to Open-JDK 9 appears to fix the problem.

Another complexity with debugging memory problems is that Python tends to
hide the problem with its allocation pools. Rather than allocating memory
when a new object is request, it will often recycle and existing object 
which was collect earlier. The result is that an object which turns out is
still live becomes recycled as a new object with a new type. Thus suddenly
a method which was expected to produce some result instead vectors into
the new type table, which may or may not send us into segfault land
depending on whether the old and new objects have similar memory layouts.

This can be partially overcome by forcing Python to use a different memory
allocation scheme. This can avoid the recycling which means we are more likely
to catch the error, but at the same time means we will be excuting different
code paths so we may not reach a similar state. If the core dump is vectoring
off into code that just does not make sense it is likely caused by the memory
pools. Starting Python 3, it is possible to select the memory allocation policy
through an enviroment variable.  See the ``PYTHONMALLOC`` setting for details.


Future directions
-----------------

Although the majority of the code has been reworked for JPype 0.7, there is still
further work to be done. Almost all Java constructs can be exercised from within
Python, but Java and Python are not static. Thus, we are working on further
improvements to the jpype core focusing on making the package faster, more
efficient, and easier to maintain. This section will discuss a few of these options.

Java based code is much easier to debug as it is possible to swap the thunk code
with an external jar. Further, Java has much easier management of resources.
Thus pushing a portion of the C++ layer into the Java layer could further reduce
the size of the code base. In particular, deciding the order of search for
method overloads in C++ attempts to reconstruct the Java overload rules. But these
same rules are already available in Java. Further, the C++ layer is designed
to make many frequent small calls to Java methods. This is not the preferred
method to operate in JNI. It is better to have specialized code in Java which
preforms large tasks such as collecting all of the fields needed for a type
wrapper and passing it back in a single call, rather than call twenty different
general purpose methods. This would also vastly reduce the number of ``jmethods``
that need to be bound in the C++ layer.

The world of JVMs is currently in flux. Jpype needs to be able to support
other JVMs. In theory, so long a JVM provides a working JNI layer, there
is no reason the jpype can't support it. But we need loading routines for
these JVMs to be developed if there are differences in getting the JVM
launched.

There is a project page on github shows what is being developed for the 
next release. Series 0.6 was usable, but early versions had notable issues
with threading and internal memory management concepts had to be redone for
stability.  Series 0.7 is the first verion after rewrite for 
simplication and hardening.  I consider 0.7 to be at the level of production 
quality code suitable for most usage though still missing some needed 
features. Series 0.8 will deal with higher levels of Python/Java integration such as Java
class extension and pickle support.  Series 0.9 will be dedicated to any
additional hardening and edge cases in the core code as we should have complete
integration.  Assuming everything is completed, we will one day become a
real boy and have a 1.0 release. 
