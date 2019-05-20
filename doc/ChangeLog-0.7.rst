:orphan:

JPype 0.7 Core ChangeLog
========================

Here is the "complete" log of the changes I think I made.

Module changes
--------------

* Moved Python module objects to namespace ``PyJP`` so that they are consistent
  with a Python module namespace.  Renamed module classes presented to 
  ``_jpype`` extension to ``PyJP*`` to match the internal classes.  Though not
  exactly a standard convention, the types were internal anyway and having
  the names match the C structure makes it more clear what resource is being 
  accessed.  It also eliminates the confusion between ``jpype`` and ``_jpype``
  resources.

* Removed all usage of Capsule from the extension module. This was bridging
  between Python versions and had to be replicated on old platforms.  As the
  capsules were functioning as crippled objects, they could not have methods
  of their own.  Thus functionality that properly belonged to a specific class
  would get pushed to the base class.  This affected former capsules of
  ``JPObject``, ``JPProxy``, and ``JPArray``.  These are now formal classes in the
  module as ``PyJPValue``, ``PyJPProxy``, and ``PyJPArray``.

* Moved the initialization of each class to the ``__init__`` function.  Thus
  rather than creating the resource at the top level ``_jpype`` module (such as 
  ``_jpype.findClass('cls'))``, the resource is created by allocating a new 
  object (such as ``_jpype.PyJPClass('cls')``).

* The presentation of ``JPArrayClass`` has been merged as a generic ``JPClass``.  
  The only requirement for creation of an array instance is that the supplied 
  ``PyJPClass`` satisfy ``isArray()``.

* Removed direct dependencies that objects holding resource be exactly the
  type in ``jpype`` module.  This reduces the restrictions in the underlying 
  Python layer and allows for multiple classes such as ``JavaArray``, ``JWrapper``,
  and ``JavaClass`` to all be recognized as holding resources.  This simplifies
  some paths in the ``jpype`` module where we needed to simply access a single
  method during bootstrapping and we were forced to construct complete
  classes necessitating the order of resource loading.

* Remove ``JPObject`` concept and replaced it with ``JPValue``.  ``JPValue`` holds
  the type of the object and a ``jvalue`` union.  Both ``JavaClass`` and
  ``JWrapper`` now point to these classes as ``__javavalue__``.  Anything with a
  ``__javavalue__`` with type ``_jpype.PyJPValue`` is now recognized as being a
  Java object.

* Changed the recognization of a ``JavaClass`` to any object holding
  ``__javaclass__`` with type ``_jpype.PyJPClass``.  This allows array classes,
  object classes, and wrappers classes to be used together.

* Added hooks to direct convert ``PyJPClass`` to a ``PyJPValue`` with a type of
  ``java.lang.class`` and an object to the class. This replaces the need for
  calling ``forName`` to get to the existing class.

* Changed ``PyJPField`` and ``PyJPMethod`` to descriptors so that we do not
  need to mess with  ``__getattribute__`` and ``__setattr__`` in many places.
 
* Eliminated the unnecessary class bound method.


C++ Reorg
~~~~~~~~~

* Reorganized the type tree in the C++ layer to better match the Java
  structures.  

* Flattened out the redundant layers so that ``JPType`` is now ``JPClass``
  corresponding to an instance of a ``jclass``.

* ``JPClass`` is not the base class.  Arrays are now objects and have 
  base classes and methods.

* Split ``JPClass`` into a separate type for each specialized object class for
  boxed, ``java.lang.Object``, and ``java.lang.Class`` which all required
  specialized conversion rules.

* Boxed, string, base ``java.lang.Object`` and base ``java.lang.Class`` are now
  specialized with their required conversion rules.


Path reduction
~~~~~~~~~~~~~~

* Removed ``HostRef`` and all of its usage.  It was a halfway memory
  management method.  To be passed around it was being held as a dynamically
  allocated pointer type with no static presence for cleanup.  This defeats
  the point of having a smart point wrapper if the smart pointer is being used
  as a pointer itself.  Thus it was only as safe as the user applied
  conventions rather than safe by design.

* Replaced all the ``HostRef`` methods and ``JPy*`` Python object wrappers with
  a new smart pointer concept (namespace ``JPPy``).  This removes the
  redundant host and ``JPy*`` wrapper layers.

* Removed multiple optimization paths such as bypassing between ``jchar`` and
  ``unicode`` if the size matched.  These paths were for speed reasons, but they
  could only be tested on particular machines.  Thus it was difficult to tell
  if something was broken.  It is better to have one tested code path
  that is slight slower, then a faster path that is busted.

* Removed dead class ``JPCharString``.

* **(bug)** Replaced all string handling with conversion through UTF8.  Java and
  Python use different UTF8 encodings and thus those paths that were trying
  to short cut directly through from one system to another were badly flawed.
  By forcing a conversion to and from each time a Java string or Python string
  are passed eliminates conversion problems.  This should resolve user issues
  having to do with truncating extended unicode characters.

* Combined all code paths in ``canConvertToJava`` and ``convertToJava`` to use the
  ``JPValue``

* Combined code paths from ``check`` and ``get`` for ``JPValue``, ``JPClass`` and
  ``JPProxy`` ``get`` patterns when fetching from Python.  Almost always we want
  to use the object immediately and just check if we can.

* Removed the entirely redundant Primitive type ``setRange`` and ``getRange``.
  That code was entirely dead because it could not be reached. Renamed the
  direct methods as they now have the same function.

* Removed ``JPTypeName``.  This concept will be phased out to
  support lambdas. ``TypeManager`` now used ``getCanonicalName()``.
  Transferred responsibility for conversion to native names to Python module 
  interface.

* Introduced named classes for all specialized instances of classes to be
  held in ``TypeManager`` namespace. Thus converted most of the "is this type" to 
  comparison of ``JPClass*`` pointers in place of string level comparisons.  

* Removed near duplicate methods.  ``JProxy`` was requesting slightly altered
  copies of many conversions to support its usage.  These operations could
  be supported by just splitting to two existing methods.  Thus we could
  eliminate a lot of stray methods that served this specialized purpose.

* ``JPArray`` is now a method holder rather than the primary object like
  ``JPBoundMethod``.  All array objects in Python now hold both a ``__javaarray__``
  and a ``__javavalue__``.  This eliminates need for special paths for 
  arrays.

* ``_getClassFor`` is now overloaded to work with array classes.  Thus
  asking for a ``JClass('[java.lang.Object;')`` will now correctly 
  return a JavaArrayClass.

* Constructing a string now shortcuts to avoid methodoverload resolution on 
  new instance if given a Python string.

* Reworked the GIL handling.  The previous model was doing all the release
  locks on the JPJni calls automatically for almost all jni transactions.
  This would be fine, except that many utility functions were using those same
  calls regardless of whether is was a good time to release the lock.  This
  ultra fine grain locking was effectively allowing any call to JPJni methods
  to become a break point, including those calls in critical sections such as
  ensureTypeCache and TypeManager::findClass.  Any time it loaded a class or
  looked up a name it could be interrupted and thus end up in a corrupt state.
  Thus I moved all of the GIL calls to those places where we call user code on
  the type returns and the object constructors.  Thus cuts the number of GIL
  transactions greatly and eliminates the need to deal with trampling global
  resources.  The refactor exposed this a bit more because the removal of
  TypeName meant that we did a lot more transactions to get the class name.
  But that does not mean the flaw was not there before.  If our tests cases had
  been any more aggressive about creating class instances during execution it
  would have overrun the TypeManager table and all would have failed.

* Removed the previous default option to automatically convert
  ``java.lang.String`` to either a Python string or a unicode when returning
  from Java.  This does mean some string operations now require calling the
  Java string method rather than the Python one.  Having strings not convert
  but rather remain on the jvm until needed cuts the conversion costs when
  working with Java heavy code.  I added a caching mech so that if we need to
  convert the string multiple times, we don't pay additional over the previous
  option.

* A special ``toString`` method was added to ``PyJPValue`` to convert Java 
  strings to Python strings. This can convert Java string resources to 
  Python ones on request.


Proxy changes
~~~~~~~~~~~~~

* Proxy as implemented previously held only a pointer to the proxy object
  and from this proxy object it lookup up the callable using either a
  dictionary or an instance.  The majority of the resources were held
  by the ``jpype.Proxy``.   This was replaced with a more general function
  in which the ``PyJPProxy`` proxy holds two resources.  One is an object
  instance and the other is a lookup function that turns the name to a 
  function definition. This supports the same use cases but eliminates
  the need for finding resources by convention.  There is no need for 
  the proxy in Python to have any specific layout other than holding a
  PyJPProxy as ``__javaproxy__``.  Thus allowing alternive structures 
  such as Proxy by inheritance to work.

* Memory handling was changes slightly as a result so that the reference
  queue is now responsible for cleaning up the proxy.  Proxy handle instances 
  are generated whenever the proxy is passed to Java.  Thus we form no
  counting loops as the proxy has no reference to the handles and the 
  handles hold a reference to the proxy. 


Exception changes
~~~~~~~~~~~~~~~~~

* Changed all exception paths to use ``JPypeException`` exclusively.  The prior
  system did way to much in the Exception constructors and would themselves
  crash if anything unusual happened making changing of the system nearly 
  prohibitive to debug.  Everything bubbles down to ``toJava`` and ``toPython``
  where we perform all the logging and pass the exception off.  This also
  centralizes all the handling to one place.

* This pulls all the logic from ``JPProxy`` so that we can now reuse that 
  when returning to any Java jni native implemented function.

* Same thing for Python, but that was already centralized on ``rethrow``.

* Reworked exception macros to include more info and introduced ``JPStackInfo``.
  It may be possible to connect all the stack info into the Python traceback 
  (via a proxy class) to present a more unified error reporting.  But this
  work is currently incomplete without a Python layer support class.

* Integrated ``JPStackInfo`` into tracer to give more complete logs when
  debugging.


Code quality
~~~~~~~~~~~~

* Applied a source formatter in netbeans.  It is not perfect as it tends to 
  add some extra spaces, but it does make faster work of the refactor. 
  Custom spacing rules were applied to netbeans to try to minimize the total
  changes in the source.

* Improved error handling where possible.

* Rework ``JPTracer`` so that reporting from places that do not have a formal
  frame or could not properly throw (such as destructors) and still appear in
  the trace log.  All ``TRACE`` macros were moved to ``JP_`` so that were less
  likely to hit conflicts.  Removed guards that complete disabled Tracer from
  compiling when ``TRACE`` was not enabled so that unconditional logging for
  serious failure such as suppressed exceptions in destructors can report.

* Defensively added ``TRACE`` statements whenever entering the module for a
  nontrivial action so that errors could be located more quickly.

* Removed ``MTRACE`` layer as Java local frame handles all cleaning tasks for
  that now.

* Replaced TRACE1, TRACE2, TRACE3 with a variodic argument macro ``JP_TRACE``
  because I am too lazy to remember to count.

* Renamed functions to best match the documented corresponding function in
  the language it was taken from.  Thus making it easier to find the needed
  documentation.  (Ie  ``JPyString::isString()`` becomes 
  ``JPPyString::check()`` if the corresponding language concept is 
  ``PyString_Check()``).  This does mean that naming is mixed for the 
  Java/Python layers but it is better to be able to get the documentation 
  than be a naming idealist.

* Used javadoc comments on header of base clases.  These strings are picked
  up by netbeans for document critical usage.

* Moved method implementations and destructors out of headers except in 
  the case of a truly trivial accessor.  This has a small performance loss
  because of removal of inline option.  This reduces the number of
  redundant implementation copies at link time and ensures the virtual
  destructor is fixed in a specific object.  We can push those back to the 
  header if there is a compelling need.


``jpype`` module changes
---------------------------

Because these do affect the end user, we have marked them as enhance, change, remove, bug fix, or internal.

General
~~~~~~~

* **(enhance)** ``__all__`` added to all modules so that we have a well defined
  export rather that leaking symbols everywhere. Eliminated stray imports in
  the jpype namespace.

* **(enhance)** Add ``@deprecated`` to ``_core`` and marked all functions that are
  no longer used appropraitely.  Use ``-Wd`` to see deprecated function warnings.

* **(enhance)** Exposed ``JavaInterface``, ``JavaObject``, ``JavaClass`` so that they
  can be used in ``issubclass`` and ``isinstance`` statement.
  ``JavaClass.__new__`` method was pushed to factory to make it safe for external
  use.

* **(enhance)** mro for Java Classes removes ``JavaInterface`` so that
  ``issubclass(cls, JavaInterface)`` is only true if the class not derived from
  ``JavaObject``.

* **(enhance)** All classes derived from ``java.lang.Throwable`` are now usable as
  thrown exceptions.  No requirement to access special inner classes with 
  exception types.  Exceptions can be raised directly from within
  a Python context to be passed to Java when in proxy.  Throwables now 
  use a standard customizer to set their base class to the Python
  Exception tree. Deprecated ``JException``

* **(enhance)** ``args`` is a property of ``java.lang.Throwable`` containing the
  message and the cause if specified.  

* **(enhance)** ``JChar`` array now converts to a string and compares with string
  properly.  Conversion uses range so that it does not try to convert
  character by character.

* **(remove)** ``JByte`` array is not a string type.  It is not a string in Java
  and should not be treated as a string without explicit conversion.
  Conversion path was horribly inefficient converting each byte as a Python
  object.  Test marked as skip.

* **(change)** Array conversion errors produce ``TypeError`` rather than
  ``RunTimeError``.

* **(enhance)** ``JArray`` now supports using raw Python types as the specifier for
  array types.  It will convert to the most appropraite type or return an
  error.  

* **(remove)** property conversion customizer is deactivated by default.  This
  one proved very problematic.  It overrided certain customizers, hid
  intentionally exposed fields, bloated the dictionary tables, and interferred
  with the unwrapping of exception types.  We can try to make it an optional
  system with ``import jpype.properties`` or some such but it will still have all
  those problems.  Best to kill this misfeature now.

* **(enhance)** ``JArray`` classes now have ``class_``.  We can access the component
  type.  This makes them more consistent with ``JClass``.  (required for
  testing)

* **(enhance)** Use of constructor call pattern eliminated the need for use of a 
  separate factory and type.  Thus we are back to the original design in 
  which we only need to expose a small number of "types".  This was applied to 
  ``JArray``, ``JClass``, ``JException``, and ``JObject``.  Use of ``isinstance()`` and
  ``issubclass`` now supported. The only challenge was keeping box types working.

* **(remove)** Functions that return a string now return a ``java.lang.String``
  rather than converting to Python.  Thus when chaining elements together in
  Java will get the full benefit matching types.  The previous auto convert
  has been removed.

* **(enhance)** ``java.lang.String`` now has much more complete set of Python 
  operations.  String conversions are now cached, so the penalty of 
  converting is kept to a minimum.

Wrappers
~~~~~~~~~

* **(internal)** Rewrote the ``JWrapper`` module from scratch to reflect the use i
  of ``JPValue``.  Renamed ``_jwrapper`` to ``_jtypes``.  The concept of wrappers 
  has now been lost internally.  All objects and primitives are just values.

* **(enhance)** Created import module containing all of the symbols needed for 
  creating types in jpype so that we can support a limited import statement 
  ``from jpype.types import *``

* **(enhance)** ``JString`` contructor now returns a ``java.lang.String`` object.  
  Removed ``JStringWrapper`` as ``java.lang.String`` serves its purpose.

* **(enhance)** ``JObject`` now returns an object with the Java type as a functional
  object rather than a dead end wrapper.  This does allow some redundant 
  things such as converting a Python class wrapper into a class 
  ``JObject(java.lang.String) == java.lang.String.class_`` but otherwise seems 
  good.

* **(enhance)** 'JObject' and 'JString' accept 0 arguments to generate a generic 
  object and empty string.

* Tried to be more consistent about returning errors that are valid in Python.

   - Too many or two few arguments to a function will throw a ``TypeError``
   - Value conversion out of range will throw ``OverFlowError``
   - Value conversions that are the right type but invalid value will 
     give ``ValueError`` (char from string too long)
   - Type conversions that cannot be completed should give ``TypeError``.
   - Errors setting attributes should give ``AttributeError`` such as 
     trying to set a final field or trying to get an instance field from a 
     static object.
   - Arrays access should produce ``IndexError`` on bad range.
     (it would be nice if these also mapped to Java errors and the corresponding
     errors in Java were derived from the Python error so that we can properly
     look for ArrayIndexOutOfBoundsException (derived from IndexException).  But
     that is too heavy to attempt now.)

* **(enhance)** ``JArray``, ``JException`` and ``JObject`` report as JavaClass when 
  using issubclass.

* **(enhance)** Short cut for just adding a base class as a customizer.


Internal
~~~~~~~~~
* **(internal)** Changes corresponding to the ``__init__`` rework to match revised 
  ``PyJP*`` classes.

* **(internal)** Changes corresponding to the capsule removal.

* **(internal)** Remove ``SPECIAL_CONSTRUCTOR_KEY`` as everything that uses it can 
  recognize a PyJPValue as indicating they are receiving an existing Java 
  resource as input.  All special handling required to construct objects from 
  within C++ layer were thus eliminated. 

* **(internal)** Removed almost all required resources from Python needing to be 
  register in ``_jpype`` with the exception of getClassMethod.  

* **(internal)** Java class customizers did not need to be deferred until after 
  the JVM is initialized.  Pushing them into the dictionary immediately 
  fixes issues in which a customizer was not applied to classes during 
  early bootstrapping.  This eliminates a large number of the need for 
  calling initialize on each jpype module in ``_core``.

* **(internal)** ``JArrayClass`` and ``JClass`` are the same for purposes of 
  Customizers and class tree.

* **(internal)** Customizer code and dictionary moved to ``_jcustomizer`` so that i
  it can be shared between Object and Array classes.

* **(internal)** Converted ``JavaClass`` to more Python like "try first, eat an 
  exception if it fails" philosophy to increase robustness to failure.  This 
  eliminates the problems when a new base class is introduced with a 
  customizer without setting up a meta class.

* **internal/enhance** Broke connections between boxed types and wrappers.  
  User supplied wrappers can implements specified "<type>Value" method.  
  Wrapper types now have similar methods to boxed types with appropriate 
  range checks.

* **(internal)** All ``$Static`` meta classes have been eliminated.  There is now 
  only one tree of classes.  A single meta class ``JClass`` serves as the type 
  for all classes.

Bugs
~~~~~~~

* **(bug fix)** Fixed bug in ``jpype.imports`` in which it would not install its 
  hooks if loaded afer the jvm was started.

* **(bug fix)** Fixed bug in JBoxed type wrappers in Python which would lead 
  ``java.lang.Double`` and ``java.lang.Float`` to have an integer value when 
  boxed was corrected.

* **(bug fix)** Fixed bug in ``JObject`` that was preventing classes from being 
  wrapped as objects.  Verified a number of test cases in the test suite.

* **(bug fix)** Reenabled the throw from Java test during proxy.  The issue was 
  that jpype was releasing resources before it could transfer control
  a ``PyErr_Clear`` removed the reference and thus our throwable was invalid.  
  It was dastardly to find, but the fix was moving a statement one line up.


Documentation changes
~~~~~~~~~~~~~~~~~~~~~~

* Documentation of major class methods have been added as well as marker 
  whereever the underlying assumptions are not reasonably transparent.

* Action items for further work have been marked as FIXME for now.


Incomplete
----------

These tasks had to be pushed over post 0.7 release.

* Finish specialization of ``JPArray`` classes for ``byte[]`` and ``char[]``

* Deal with fast array conversions misuse of types. ``int[]<=>float[]``

* Direct bridge methods for ``char[]`` are currently bypassing the unicode 
  translation layer.  It is unclear what Java does with extended unicode 
  when dealing with ``char[]``.  

* Add a system to register a translation customizer so that we do not need to
  modify C++ code to add new simple translations like Python date to Java 
  Instant.  These would be installed into the PyJPClass during class 
  wrapper customization. We will need to make sure each class has a Python
  type wrapper cached in ensureTypeCache so we are guaranteed to find
  the conversion.

* Add tests for Exception.args



