:orphan:

JPype 0.8 Core ChangeLog
========================

Here is the "complete" log of the changes I think I made. Based on current
progress we should be able to complete the 0.8 series by mid 2020.

native/common
-------------

This section relates to changes in ``JP`` named C++ classes. The central theme
of these modifications is reducing this layer to bare minimum. The C++ layer is
the most challeging to debug as it has no visibility in either Python or Java.
Thus the more code we can pull out of it the easier it is to develop and
maintain.

These changes are all internal to the C++ layer and thus have little or no
changes from the user perspective, unless we accidentally break or change the
method resolution or type conversion rules. Because this layer and the 
new Java layer are so tightly linked some changes are noted in both places.

* All globals must die.

* Transferred all responsibilities from C++ to Java with respected to 
  ``TypeManager``. Java now controls deleting of C++ resources.

* As all of the resources required for the C++ objects is now passed from
  transactions with the ``TypeFactory`` service, there is little need for most
  of ``JPJni``. All of those now useless methods are removed.

* Removed all global variables from native/common. Moved those resources to 
  ``JPContext``. Added ``JPContext`` to all classes as a requirement to start a
  ``JPJavaFrame``. ``JPContext`` absorbed the remaining ``JPJni`` and ``JPEnv``
  functions.

* Distributed all the global Java methodID that remained to the corresponding 
  classes that made the required call. When the class is initialized, Java
  passes the jclass reference in and we can take the methods we need there.  This
  eliminates all need for centerialized management and those redundant global
  references.

* Defined the Context as holding 
  - The JVM being run
  - JPype spawned services in the JVM
  - Common types that require frequent acces.

* ``JPProxy`` was gutted and rewritten. Rather than having to call many method
   and setting fields when we need a new instance, instead we make a
  ``JPypeProxy`` object in Java. It is responsible creating the
  ``InvocationHandler`` saving several transactions. Like before the new instance
  is bound to the Python object. It also creates the corresponding reference. We
  just need to add the extra reference for the Python object.

* Moving to services within the context lead to renaming the ``::init`` static
  methods as the constructor for the corresponding service. 

* Centralized the loading of the JPype services within the JPype context. The
  only odd ball currently is ``JPTypeManager`` as it needs to get its half post
  startup. Thus its constructor just gets the methodIDs and after start it
  fetches its required resource. If more items start to fit this pattern then it
  will need to be formalized.

* Rename the types for ``java.lang.Object``, ``java.lang.String``, and 
  ``java.lang.Class`` to ``JPObjectType``, ``JPStringType``, and 
  ``JPClassType`` for clarity.

* Renamed the ``jp_env`` file to ``jp_javaframe`` as that was the only
  material left after conversion.

* Many changes to the logging system made as part of the effort to track
  down the stack corruption. Most are positive changes either to expand
  capabilities or ensure that the tracing system can't create its own 
  issues.

* **issue** after two weeks of struggling, I have no clue why the stack 
  is getting corrupted when calling a proxy from multiple threads. Trying
  different APIs, checking the Python API and references does not yield a useful
  result. Based on my observations there is no reason for the stack to be
  corrupted when calling a proxy. The problem occurs whenever a proxy call is
  interrupted by the Python threading mechanism usually while in the middle of
  calling the user hook, or converting resources.  When the thread returns the
  stack is corrupt resulting in a fatal and usually untracable crash. As the
  stack is fine if the call completes normally is seems like there is a problem
  in Python. It is possible that we are not holding a reference properly over the
  interval, resulting in a garbage collecting of some resource. Though I can't
  rule that out, I can't find it. 

* **issue** start up costs are currently too high for the JVM. In the 
  previous system, methods were not loaded until used. This meant there were
  serious bootstrapping issues during startup. This was switched to better match
  the Java model where all linked resources are loaded when a class is loaded
  (everything must be resolved). But this is currently forcing 10k objects to be
  instantiated at the startup of the JVM.  Given the speed demon that JNI is this
  creates a notable lag in starting.  We need to figure out how to use the old
  method of deferring loading of method parameters until usage. This will be
  slightly complicated in that it is desireable to know all of the return types
  when the Python class structure is created. More consideration of the
  architecture is required.


``org.jpype`` Package
---------------------

Many resources were moved to Java from C++. This conversion cuts development
time greatly, as we can now use the externally compiled jar for testing.
To use it just add ``org.jpype.jar`` from the project directory to 
the classpath used for testing. This will override the bootloader copy
allowing for testing without a recompile.

* **bug** build.xml for the ``org.jpype`` jar incorrectly auto versioned
  the binary format. This meant that we end up with versions of the 
  internal jar tied to later versions of the JVM. To prevent this the
  binary is now tied to Java 1.7. Java 1.6 is too old to be useful.
  Java 1.7 is still out there but should not really need support after 
  2020. Thus we will hold the line there until that time.

* Services for ``JPTypeFactory``, ``JPTypeManager``, ``JPReferenceQueue``,
  ``JPClassLoader``, and ``JPProxyFactory`` were created.

* Tests of the JPype services can now live outside of the JPype module
  allowing unit tests without having to develop exercises from within
  Python. This requires the creating of test harnesses that serve the 
  roll of the native sections of the code. These can be fully instrumented
  to check for things like leaked resources, access to the wrong type,
  or access before creation.

* A centeralized ``org.jpype.JPypeContext`` object was created to hold all the
  instances needed. This removes the individual singletons that were previously
  created. Startup and shutdown actions are all executed by this central
  module. This allows for flexibility in testing. 

* ``org.jpype.manager.TypeManager`` takes over all of the role that the 
  previous C++ type manager did. This simplifies the system a lot as
  Java has much easier access to the Java reflection system.

* A new ``org.jpype.manager.MethodResolution`` class creates the 
  method order and override table for method resolution. There was a minor snag
  here as it appears that Java itself does not have the full method binding
  rules especially as relates to primitives.

* Created ``TypeFactory`` service as the Java class to create all C++
  resources.  This is a native JNI class. Used
  ``org.jpype.manager.TypeFactoryHarness`` to test the creation of resources.

* Method resolution order in ``org.jpype`` is opposite to the order used by the
  method dispatch. This is an intentional design because the order of creation
  requires that the method override list of each method include all of the
  methods that are hidden.  The lookup must be done in reverse with higher
  presidence methods first followed by more general.


``_jpype`` changes
------------------

This section documents that changes to the ``PyJP`` named CPython 
extension class. Most of the changes are to improve capabilities
in the future.

* Added ``PyJPContext`` as a front end for JVM related services. Removed
  methods from the ``_jpype`` module.

* Module related functions move from module scoped to the ``PyJPContext``
  The exception is the ``setResources`` method.

* Memory management for ``PyJPContext``:  Each Python object needs
  to hold a reference to the ``PyJPContext`` in order for the context to be
  held for the duration of the lifespan of the objects.  But the context itself
  will serve as a general container for the resources to be stored from
  ``jpype``. Thus all objects in the ``_jpype`` must use the Python container API
  so that they can handle circular references properly.


``jpype`` Module
----------------

These are broken down by **bug**, **enhance**, **internal**.
The goal for the 0.8 series is no API breakage, thus most changes
need to be under level of documented

* **bug** Corrected an issue with circular references on JProxy. New style
  proxies reference the ``__javaproxy__`` within the creating class.
  Unfortunately, this prevents them from being properly collected. Using 
  the container API for Python extensions corrects this issue.

* **enhance** removed the need for a lookup function of ``JProxy``. Instead
  everything uses standard ``getattr`` API. The only form that needs to 
  be modified was the ``dict`` form.

* **internal** Started adding ``jvm`` to API so that we can support 
  shutting down, restarting, and managing multiple JVM. For now this 
  will default to the default JVM created at the start of the module.

* **internal** global resources held in the module move to within
  the context instance. This ensures that all resources clean up when 
  the JVM is shutdown.

* **internal** switched to ``collections.abc`` to satisfy Python 3.8

* **internal** stopped using Java to get known Java constants such
  as the range of integers. These are hard coded constants in the 
  Java language. Looking up a well known constant is just adding to
  the complexity of the boot up sequence and adds no real advantage.



Notes
-----

* Attempted to apply the initialize structure scheme that is shown in the
  Python documentation. It has a lot of advantages in clarity and
  simplifications to the Python extension classes. Unfortunately this portion
  of C99 was never transferred to C++, thus the syntax required is not
  available. This mean the old position based method is requred.

* During a review of the type conversion rules in the Java specification, we
  see there are more than one set of rules for conversion.  Method looks up and
  assignment have different conversion rules. Thus far I don't see a benefit of
  having a seperated canConvert for each path, but I can't rule out if there is
  an edge case that is relevant to users.


TODO
----

* Move primitives internally to have two copies. A JVM neutral version
  which can be share with any JVM and a JVM specific copy to use for 
  conversion. This will simplify the process because primitives will
  not expire. May need to pass the context to ``convertToJava`` to 
  make that happen.

* Add a conversion utility should check for potential conversions 
  during match up. Each Java class should have a list of duck
  type properties and concrete types to allow for conversion.

* **enhance** Work to add ``__repr__`` to JPype classes

* Implement container API for all ``PyJP`` classes.
