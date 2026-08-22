Developer Guide
===============

Overview
--------

This document lays out the architecture of the JPype code -- the parts that
are hard to reconstruct from reading the source alone, because they record
*why* the code is shaped the way it is, not just what it does. For anything
a general user needs to debug a crash (gdb, attaching debuggers), see
:doc:`debugging_py`; this guide is for developing and debugging JPype's own
internals. We use the royal we throughout.

JPype predates this rewrite by over 10 years, originally designed with a
multilanguage abstraction layer (to support Ruby and other interpreters
alongside Python) on top of separate Java, C++, and Python reference-counting
schemes. That extra layer was cut: Java's own JNI local-frame system replaced
the custom referencing, and the Java/C++ class hierarchies were realigned to
match one another directly. The result is a three-layer design -- Python
front end, a thin CPython C API wrapper, and a C++ JNI layer -- each
described in its own section below, followed by the Java-embeds-Python
reverse bridge, which is a separate, newer subsystem with its own layering.


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
  ``_JModule``, the module has multiple Python classes to support the native jpype
  code such as ``_JClass``, ``_JArray``, ``_JObject``, etc.

CPython API wrapper
  In addition to the exposed Python module layer, there is also a C++ wrapper
  for the Python API. This is located in ``native/python`` and has the prefix
  ``JPPy`` for all classes.  ``jp_pythontypes`` wraps the required parts of
  the CPython API in C++ for use in the C++ layer.

C++ JNI layer
  The guts that drive Java are in the C++ layer located in ``native/common``. This layer
  has the namespace ``JP``. The code is divided into wrappers for each Java type,
  a typemanager for mapping from Java names to class instances, support classes
  for proxies, and a thin JNI layer used to help ensure rigorous use of the same
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
with statically created wrappers, instead jpype dynamically creates
Python wrappers as requested by the user.

The wrapping process is triggered in two ways. The user can manually
request creating a class by importing a class wrapper with jpype.imports
or ``JPackage`` or by manually invoking it with ``JClass``. Or the class wrapper
can be created automatically as a result of a return type or exception
thrown to the user.

Because the classes are created dynamically, the class structure
uses a lot of Python meta programming.
Each class wrapper derives from the class wrappers of each of the
wrappers corresponding to the Java classes that each class extends
and implements. The key to this is a hacked ``mro``. The ``mro``
orders each of the classes in the tree such that the most derived
class methods are exposed, followed by each parent class. This
must be ordered to break ties resulting from multiple inheritance
of interfaces.  The factory classes are grafted into the type system
using ``__instancecheck__`` and ``__subtypecheck__``.

resource types
~~~~~~~~~~~~~~

JPype largely maps to the same concepts as Python with a few special elements.
The key concept is that of a Factory which serves to create Java resources
dynamically as requested.  For example there is no Python notation to
create a ``int[][]`` as the concept of dimensions are fluid in Python.
Thus a factory type creates the actual object instance type with
``JArray(JInt,2)``  Like Python objects, Java objects derives from a
type object which is called ``JClass`` that serves as a meta type for
all Java derived resources.  Additional type like object ``JArray``
and ``JInterface`` serve to probe the relationships between types.
Java object instances are created by calling the Java class wrapper just
like a normal Python class.  A number of pseudo classes serve as placeholders
for Java types so that it is not necessary to create the type instance
when using.  These aliased classes are ``JObject``, ``JString``, and
``JException``.   Underlying all Java instances is the concept of a
``jvalue``.

``jvalue``
++++++++++

In the earlier design, wrappers, primitives and objects were all separate
concepts. At the JNI layer these are unified by a common element called
jvalue. A ``jvalue`` is a union of all primitives with the jobject. The jobject
can represent anything derived from Java object including the pseudo class
jstring.

This has been replaced with a Java slot concept which holds an instance of
``JPValue`` which holds a pointer to the C++ Java type wrapper and a Java
jvalue union.  We will discuss this object further in the CPython section.

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
blindly.  The order of bootstrapping is controlled by specific sequence
of boot actions after the JVM is started in ``startJVM``.  The class instance
``class_`` may not be accessed until after all of the basic class, object,
and exception types have been loaded.


Factories
~~~~~~~~~

The key objects exposed to the user (``JClass``, ``JObject``, and ``JArray``) are each
factory meta classes. These classes serve as the gate keepers to creating the
meta classes or object instances. These factories inherit from the Java class meta
and have a ``class_`` instance inserted after the the JVM is started.  They do not
have exposed methods as they are shadows for action for actual Java types.

The user calls with the specified arguments to create a resource. The factory
calls the ``__new__`` method when creating an instance of the derived object. And
the C++ wrapper calls the method that internally constructs the resource, such
as ``_JClass`` or ``_JObject``.  Most of the internal calls currently create the
resource directly without calling the factories.  The gateway for this is
``PyJPValue_assignJavaSlot``, which attaches the C++ ``JPValue`` to the
already-allocated Python instance's Java slot (see :ref:`javaslots` below).


Style
~~~~~

JPype's factories deliberately collapse distinct operations into one
callable: boxing a primitive, casting to a specific type, and creating a new
object are all tied together in the single ``JObject`` factory, which also
doubles as a base class so it works with ``issubclass``/``isinstance``.
Customizers extend this by hooking Java classes into native Python syntax
(``with`` for try-with-resources and ``synchronized``, iteration protocols
for collections, etc). When adding a feature to the Python layer, prefer
hiding it inside existing Python syntax over exposing a new function.

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
When a class was created, it went through the list and asked the class if
it matched that class name. If it matched, it altered the dict of members
to be created so when the dynamic class was finished it had the custom
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

Jpype objects work with the inner layers by inheriting from a set of special
``_jpype`` classes.  This class hierarchy is maintained by the meta class
``_jpype._JClass``.  The meta class does type hacking of the Python API
to insert a reserved memory slot for the ``JPValue`` structure.  The meta
class is used to define the Java base classes:

 * ``_JClass`` - Meta class for all Java types which maps to a java.lang.Class
   extending Python type.
 * ``_JArray`` - Base class for all Java array instances.
 * ``_JObject`` - Base type of all Java object instances extending Python object.
 * ``_JNumberLong`` - Base type for integer style types extending Python int.
 * ``_JNumberFloat`` - Base type for float style types extending Python float.
 * ``_JChar`` - Special wrapper type for JChar and java.lang.Character
   types extending Python str.
 * ``_JBoolean`` - Base type for JBoolean extending Python int.
 * ``_JException`` - Base type for exceptions extending Python Exception.
 * ``_JBuffer`` - Base type for Java buffer-backed types supporting the
   Python buffer protocol.

There is no single generic capsule type any more: an earlier design had
one (``_JValue``), but it was removed once every concrete type above
carried its own Java slot directly (see :ref:`javaslots` below).

These types are exposed to Python to implement Python functionality specific
to the behavior expected by the Python type.  Under the hood these types are
largely ignored.  Instead the internal calls for the Java slot to determine
how to handle the type.  Therefore, internally often Python methods will be
applied to the "wrong" type as the requirement for the method can be satisfied
by any object with a Java slot rather than a specific type.

See the section regarding Java slots for details.


``PyJPModule`` module
~~~~~~~~~~~~~~~~~~~~~~

This is the front end for all the global functions required to support the
Python native portion. Most of the functions provided in the module are
for control and auditing.

Resources are created by setting attributes on the ``_jpype`` module
prior to calling ``startJVM``.   When the JVM is started each of the
required resources are copied from the module attribute lists to the
module internals.  Setting the attributes after the JVM is started has
no effect.  Resources are verified to exist when the JVM is started
and any missing resources are reported as an error.

``_JClass`` class
~~~~~~~~~~~~~~~~~~~

The class wrappers have a metaclass ``_jpype._JClass`` which serves as
the guardian to ensure the slot is attached, provide for the inheritance
checks, and control access to static fields and methods.  The slot holds
a java.lang.Class instance but it does not have any of the methods normally
associate with a Java class instance exposed.  A java.lang.Class instance
can be converted to a Java class wrapper using ``JClass``.


``_JMethod`` class
~~~~~~~~~~~~~~~~~~~~

This class acts as descriptor with a call method.  As a descriptor accessing its
methods through the class will trigger its ``__get__`` function, thus
getting ahold of it within Python is a bit tricky.  The ``__get__`` method
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


``_JField`` class
~~~~~~~~~~~~~~~~~~~

This class is a descriptor with ``__get__`` and ``__set__`` methods.
When called at the static class layer it operates on static fields.  When
called on a Python object, it binds to the object making a ``this`` pointer.
If the field is static, it will continue to access the static field, otherwise,
it will provide access to the member field. This trickery allows both
static and member fields to wrap as one type.

This class is stored directly in the class wrappers.

``_JArray`` class
~~~~~~~~~~~~~~~~~~~

Java arrays are extensions of the Java object type.  It has both methods associated
with java.lang.Object and Python array functionality.  Primitives have
specialized implementations to allow for the Python buffer API.


``_JMonitor`` class
~~~~~~~~~~~~~~~~~~~~~

This class provides ``synchronized`` to JPype.  Instances of this
class are created and held using ``with``.  It has two methods
``__enter__`` and ``__exit__`` which hook into the Python RAII
system.


The Java slot
~~~~~~~~~~~~~~~~~~~

Java primitive and object instances derive from the special Python types
listed above (``_JObject``, ``_JNumberLong``, ``_JChar``, ...).  These each
have the Python functionality to be exposed and a Java slot -- a reserved
block of memory (see :ref:`javaslots` below) holding a C++ ``JPValue``.
There is no separate generic wrapper class holding that slot; the ``PyJPValue_*``
C functions (``PyJPValue_getJavaSlot``, ``PyJPValue_assignJavaSlot``, ...)
operate directly on any CPython object that carries the slot, regardless of
its concrete Python type.

Specific implementations exist for object, numbers, characters, and
exceptions.  But fundamentally all are treated the same internally
and thus the CPython type is effectively erased outside of Python.

Unlike ``jvalue`` we hold the object type in the C++ ``JPValue``
object.  The class reference is used to determine how to match the arguments
to methods. The class may not correspond to the actual class of the
object. Using a class other than the actual class serves to allow
an object to be cast and thus treated like another type for the purposes
of overloading. This mechanism is what allows the ``JObject`` factory
to perform a typecast to make an object instance act like one of its
base classes.

.. _javaslots:

Java Slots
------------------

The key to achieving reasonable speed within CPython is the use of slots.
A slot is a dedicated memory location that can be accessed without consulting
the dictionary or bases of an object.  CPython achieves this by reserving space
within the type structure and by using a set of bit flags so that it can avoid
costly dictionary lookups.
The reserved space is ordered by number and thus avoids the need to access the
dictionary while the bit flags serve to determine the type without traversing
the ``__mro__`` structure.  We had to implement the same effect while deriving
from a wide variety of Python types including type, object, int, long, and
Exception.  Adding the slot directly to the type and objects base memory
does not work because these types all have different memory layouts.  We could
have a table look up based on the type but because we must obey both the CPython
and the Java object hierarchy at the same time it cannot be done within the
memory layout of Python objects.  Instead we have to think outside the box,
or rather outside the memory footprint of Python objects.

CPython faces the same conflict internally as inheritance often forces adding
a dictionary or weak reference list onto a variably size type such as long.
For those cases it adds extra space to the basesize of the object and then
ignores that space for the purposes of checking inheritance. It pairs this
with an offset slot that allows for location of the dynamic placed slots.
We cannot replicate this in the same way because the CPython internals are
all specialized static members and there is no provision for introducing
user defined dynamic slots.

Therefore, instead we will add extra memory outside the view of Python
objects through the use of a custom allocator. We intercept the call to
create an object allocation and then call the regular Python allocators
with the extra memory added to the request.  As our extra slot has
resources in the form of Java global references associated with it, we
must deallocate those resources regardless of the type that has been
extended.  We perform this task by creating a custom finalize method to
serve as the destructor.  Thus a Java slot requires
overriding each of ``tp_alloc``, ``tp_free`` and ``tp_finalize``.  The
class meta gatekeeper creates each type and verifies that the required
hooks are all in place.  If the user tries to bypass this it should
produce an error.

In place of Python bit flags to check for the presence of a Java slot
we instead test the slot table to see if our hooks are in place.
We can test if the slot is present by looking to see if both `tp_alloc` and
`tp_finalize` point to our Java slot handlers.  This means we are still
effectively a slot as we can test and access with O(1).

Accessing the slot requires testing if the slot exists for the object,
then computing the size of the object using the basesize and itemsize
associated with the type and then offsetting the Python object pointer
appropriately.  The overall cost is O(1), though is slightly more
expensive than directly accessing an offset.


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
       JP_PY_TRY("dosomething");

       // Make sure there is a jvm to receive the call.
       ASSERT_JVM_RUNNING("dosomething");

       // Make a resource to capture any Java local references
       JPJavaFrame frame;

       // Call our Java methods
       ...

       // Return control to Python
       return obj.keep();

       // Use the standard catch to transfer any exceptions back
       // to Python
       JP_PY_CATCH(NULL);
    }

All entry points from Python into ``_jpype`` should be guarded with this pattern.

There are exceptions to this pattern such as removing the logging, operating on
a call that does not need the JVM running, or operating where the frame is
already supported by the method being called.


Python referencing
~~~~~~~~~~~~~~~~~~

One of the most miserable aspects of programming with CPython is the relative
inconsistency of referencing. Each method in Python may use a Python object or steal
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
  This policy means that the reference counter needs to be incremented at the start
  and decremented at the end. We must reference it because if we don't and some Python call
  destroys the reference out from under us, the system may crash and burn.

``borrowed_``
  This policy means we were given a borrowed reference that we are expected
  to reference and unreference when complete, but the command that returned it
  can fail. Thus before referencing it, the system must check if an error has
  occurred. If there is an error, it is promoted to an exception.

``claim_``
  This policy is used when we are given a new object which is already referenced
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

CPython's C API has very few guard rails: many examples in the wild bypass
the type's virtual table and call a field or method directly rather than
going through the proper macro. That works for the example's own narrow
case, but it's conditioned on assumptions about the object's behavior that
don't generalize -- get it wrong on a type you didn't anticipate and you get
a segfault far from the actual mistake, since CPython mostly trusts the
caller instead of validating type flags and allocator match. When working on
the extension code, always go through the type's virtual table and use the
proper accessor macros; skipping this even once in a complex pattern is
where the memory corruption bugs come from.


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
be treated accordingly. If you have to hold a global use the appropriate ``JPRef``
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
  ``implicit_`` if the conversion is possible and acceptable as part of a
  method call, or ``exact_`` if this type converts without ambiguity. It is expected
  to check for something that is already a Java resource of the correct type
  such as ``JPValue``, or something that is implementing the behavior as an interface
  in the form of a ``JPProxy``.

``convertToJava``
  This method consults the type and produces a conversion. The order of the match
  should be identical to the ``canConvertToJava``. It should also handle values and
  proxies.

``convertToPythonObject``
  This method takes a jvalue union and converts it to the corresponding
  Python wrapper instance.

``getValueFromObject``
  This converts a Java object into a corresponding ``JPValue``. This unboxes
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
able to invoke a method with that return type. This corresponds to the underlying
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
By selecting the source and target encoding transcribe can convert to or
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


Embedded Python (reverse bridge)
---------------------------------

Everything above is the *Python calls Java* direction. JPype also supports
the reverse: a Java application embedding CPython and calling into it. This
is a separate, newer subsystem -- ``native/jpype_module`` -- with its own
layering that mirrors the forward bridge's shape (a native lifecycle
manager, generated front-end wrapper types, a proxy/reference-lifetime
mechanism) but is not built from the same code. The user-facing chapters
(:doc:`quickguide_java` onward, paired with each forward-bridge chapter)
document behavior; this section covers what isn't visible just by reading
those or the source: why the pieces are shaped the way they are.

``org.jpype.MainInterpreter``
  The singleton entry point, analogous to ``startJVM``/``shutdownJVM`` but
  inverted: it boots an embedded CPython interpreter inside the running JVM
  process rather than a JVM inside a running Python process. Unlike the
  forward bridge's JVM, an embedded CPython interpreter cannot currently be
  restarted or run twice in one process -- CPython's own global state
  doesn't support it cleanly -- which is why this is a singleton rather
  than a factory (see :doc:`limitations_java`). ``org.jpype.SubInterpreter``
  and ``SubInterpreterBuilder`` provide the (experimental, GIL-isolated)
  multiple-interpreter escape hatch where CPython's subinterpreter support
  allows it.

``python.lang``, ``python.collections``, ``python.io``, ``python.datetime``, ``python.decimal``, ``python.pathlib``, ``python.exceptions``
  The generated-feeling but hand-written front-end packages: each is a set
  of Java interfaces (``PyObject``, ``PyDict``, ``PyPath``, ...) backing
  onto live Python objects. These are not special-cased in the bridge --
  every one of them, including ``python.io``, is an ordinary
  ``org.jpype.WrapperService`` SPI provider, resolved lazily or eagerly per
  module. This is deliberate: it means a third-party library can add a
  Java-interface wrapper for its own Python types (e.g. ``numpy.ndarray``)
  without touching JPype core. See :doc:`spi` for the full mechanism and
  ``package-info.java`` in each ``python.*`` package for that package's
  design rationale.

Dispatch and lifetime
  A call through a ``python.*`` interface resolves to a Python callable by
  name, with a ``$``-mangled fallback path for names that collide with
  Java keywords or ``Object`` methods (mirrors the forward bridge's own
  name-mangling problem, solved independently on this side -- see
  :doc:`customizers_java`). Keeping a live Python object reachable from
  Java is symmetric to the forward bridge's ``JPReferenceQueue``: the
  native reference queue (``org.jpype.ref``) pins the Python refcount for
  as long as a Java-side handle exists, releasing it when the Java object
  is collected or explicitly closed (:doc:`tooling_java`).

GIL model
  Every call from Java into Python acquires the GIL for the duration of
  that call and releases it on return -- there is no persistent
  "current thread owns the interpreter" state to manage from the Java
  side, which is what makes it safe to call from multiple Java threads
  without a manual locking scheme. The cost and the async call-pool built
  on top of it are covered in :doc:`threading_java`; this is the
  one-sentence version of why that design was chosen: it trades per-call
  overhead for not having to reason about GIL ownership across arbitrary
  Java thread lifetimes.


Tracing and crash diagnosis
----------------------------

For the walkthrough of attaching gdb, the Open-JDK/gdb ``nmethod`` issue,
``PYTHONMALLOC``, the ``--enable-tracing`` build flag, and what it means if
you hit JPype's deliberate-crash mechanism, see :doc:`debugging_py`'s
"Diagnosing crashes in JPype itself" -- that content is written for anyone
hitting a native crash, not just core contributors, so it lives in the user
guide rather than here.

The one piece worth knowing as a developer specifically: the deliberate
crash (a null-pointer write, so gdb gets a real stack trace instead of a
signal swallowed by Java or a silent ``terminate``) is reserved for
catastrophic paths where no exception can be delivered to either side --
most commonly a resource-loading failure during startup. The pattern lives
in ``jp_context.cpp``; if you're reordering resource loading there, that's
the failure mode to watch for.

.. code-block:: cpp

   int *i = nullptr;
   *i = 0;  // Trigger deliberate crash for gdb backtrace


Coverage
--------
Some of the tests require additional instrumentation to run, this can be enabled
with the CMake ``ENABLE_COVERAGE`` option::

    pip install -e . --config-setting cmake.args="-DENABLE_COVERAGE=ON"



Future directions
-----------------

The roadmap this section originally described (0.7 hardening, then 0.8 for
pickle support and deeper Python/Java integration) is complete -- pickling
(:doc:`pickling_py`), the reverse bridge (Java embedding Python, see below),
and SPI-based extensibility (:doc:`spi`) all shipped. The architectural
ideas worth carrying forward now:

Pushing more of the overload-resolution and type-collection logic from the
C++ layer into the Java thunk remains attractive: Java already has to
implement its own overload rules, so C++ reconstructing them is duplicated
work, and JNI performs better with a few large calls (e.g. "collect all
fields for this type wrapper") than many small ones. This hasn't been done;
it would shrink the C++ layer and the number of bound ``jmethods``.

Longer term, JNI itself is a large piece of surface area to keep correct
(see `Memory management`_ above). A from-scratch replacement built on the
Java Panama/FFM API is being prototyped as a separate project; if it
matures, it would replace the C++ JNI layer described in this guide rather
than extend it.
