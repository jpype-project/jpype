:orphan:

JPype 0.8 Core ChangeLog
========================

Here is the "complete" log of the changes I think I made.

Module changes
--------------

* Transferred all responsibilities from C++ to Java with respected to 
  TypeManager. Java now controls deleting of C++ resources.
* Created TypeFactory service as the Java class to create all C++ resources.
  This is a native JNI class. Used org.jpype.manager.TypeFactoryHarness to
  test the creation of resources.
* As all of the resources requiref for the C++ objects is now passed from
  transactions with the TypeFactory service, there is little need for most
  of JPJni. All of those now useless methods are removed.
* Removed all global variables from native/common. Moved those resources to 
  JPContext. Added JPContext to all classes as a requirement to start a 
  JPJavaFrame. JPContext absorbed the remaining JPJni and JPEnv functions.
* Added PyJPContext as a front end for JVM related services. Removed methods
  from the _jpype module.
* Distributed all the global Java methodID that remained to the corresponding 
  classes that made the required call. When the class is initialized, Java
  passes the jclass reference in and we can take the methods we need there.
  This eliminates all need for centerialized management and those redundant
  global references.
* JPProxy was a special case where
* Defined the Context as holding 
  - The JVM being run
  - Services for JPTypeFactory, JPTypeManager, JPReferenceQueue,
    JPClassLoader, and JPProxyFactory.
* Referencing model will be required to hold the PyJPContext open until the 
  last Python object that points to that JVM dies. The PyJPContext will
  serve as the central reference point to tell dead objects they are no longer
  alive.
* JPProxy was gutted and rewritten. Rather than having to call many method and
  setting fields when we need a new instance, instead we make a JPypeProxy
  object in Java. It is responsible creating the InvocationHandler saving
  several transactions. Like before the new instance is bound to the Python
  object.
* Moving to services within the context lead to renaming the ::init static
  methods as the constructor for the corresponding service. 
* Centralized the loading of the JPype services within the JPype context. The
  only odd ball currently is JPTypeManager as it needs to get its half
  post startup. Thus its constructor just gets the methodIDs and after
  start it fetches its required resource. If more items start to fit this 
  pattern then it will need to be formalized.
