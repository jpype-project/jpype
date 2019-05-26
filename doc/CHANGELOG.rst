Changelog
=========

This changelog *only* contains changes from the *first* pypi release (0.5.4.3) onwards.

- **Next version - unreleased**

- **0.7.0 - 2019**

  - Complete rewrite of the core module code to deal unattached threads,
    improved hardening, and member management.  Massive number of internal 
    bugs were identified during the rewrite and corrected.
    See the :doc:`ChangeLog-0.7` for details of all changes.

  - API breakage:

     - Java strings no longer convert automatically to Python strings.
       The previous behavior was switchable, but only the default
       convert to Python was working. Converting to automatically
       lead to problems in which is was impossible to work with
       classes like StringBuilder in Java. To convert a Java
       string use ``str()``.

     - Java exceptions are now derived from Python exception. The
       old wrapper types have been removed. Catch the exception
       with the actual Java exception type rather than ``JException``.
       
     - Undocumented exceptions issued from within JPype have
       been mapped to the corresponding Python exception types
       such as ``TypeError`` and ``ValueError`` appropriately.
       Code catching exceptions from previous versions should 
       be checked to make sure all exception paths are being
       handled.

     - Undocumented property import of Java bean pattern
       get/set accessors was removed as the default. It is 
       available with ``import jpype.beans``, but its
       use is discouraged.

  - API rework:

     - JPype factory methods now act as base classes for dynamic
       class trees. 
     - Static fields and methods are now available in object
       instances.
     - Inner classes are now imported with the parent class.
     - ``jpype.imports`` works with Python 2.7.
     - Proxies and customizers now use decorators rather than
       exposing internal classes.  Existing ``JProxy`` code
       still works.
     - Decorator style proxies use ``@JImplements`` and ``@JOverload``
       to create proxies from regular classes.
     - Decorator style customizers use ``@JImplementionFor``
     - Module ``jpype.types`` was introduced containing only
       the Java type wrappers. Use ``from jpype.types import *`` to
       pull in this subset of JPype.

  - ``synchronized`` using the Python ``with`` statement now works
    for locking of Java objects.

  - Previous bug in initialization of arrays from list has been
    corrected.

  - Added extra verbiage to the to the raised exception when an overloaded
    method could not be matched.  It now prints a list of all possible method
    signatures.

  - The following is now DEPRECATED

    - ``jpype.reflect.*`` - All class information is available with ``.class_``
    - Unncessary ``JException`` from string now issues a warning.

  - The followind is now REMOVED

    - Python thread option for ``JPypeReferenceQueue``.  References are always handled with
      with the Java cleanup routine.  The undocumented ``setUsePythonThreadForDaemon()``
      has been removed.
    - Undocumented switch to change strings from automatic to manual
      conversion has been removed.
    - Artifical base classes ``JavaClass`` and ``JavaObject`` have been removed.
    - Undocumented old style customizers have been removed.
    - Many internal jpype symbols have been removed from the namespace to 
      prevent leakage of symbols on imports.

  - promoted *`--install-option`* to a *`--global-option`* as it applies to the build as well
    as install.
  - Added *`--enable-tracing`* to setup.py to allow for compiling with tracing
    for debugging.
  - Ant is required to build jpype from source, use ``--ant=`` with setup.py
    to direct to a specific ant.

- **0.6.3 - 2018-04-03**

  - Java reference counting has been converted to use JNI
    PushLocalFrame/PopLocalFrame.  Several resource leaks
    were removed.

  - ``java.lang.Class<>.forName()`` will now return the java.lang.Class.
    Work arounds for requiring the class loader are no longer needed.
    Customizers now support customization of static members.

  - Support of ``java.lang.Class<>``

    - ``java.lang.Object().getClass()`` on Java objects returns a java.lang.Class
      rather than the Python class
    - ``java.lang.Object().__class__`` on Java objects returns the python class
      as do all python objects
    - ``java.lang.Object.class_`` maps to the java statement ``java.lang.Object.class`` and
      returns the ``java.lang.Class<java.lang.Object>``
    - java.lang.Class supports reflection methods
    - private fields and methods can be accessed via reflection
    - annotations are avaiable via reflection

  - Java objects and arrays will not accept setattr unless the
    attribute corresponds to a java method or field whith
    the exception of private attributes that begin with
    underscore.

  - Added support for automatic conversion of boxed types.

     - Boxed types automatically convert to python primitives.
     - Boxed types automatically convert to java primitives when resolving functions.
     - Functions taking boxed or primitives still resolve based on closest match.

  - Python integer primitives will implicitly match java float and double as per
    Java specification.

  - Added support for try with resources for ``java.lang.Closeable``.
    Use python "with MyJavaResource() as resource:" statement
    to automatically close a resource at the end of a block.

- **0.6.2 - 2017-01-13**

  - Fix JVM location for OSX.
  - Fix a method overload bug.
  - Add support for synthetic methods

- **0.6.1 - 2015-08-05**

  - Fix proxy with arguments issue.
  - Fix Python 3 support for Windows failing to import winreg.
  - Fix non matching overloads on iterating java collections.

- **0.6.0 - 2015-04-13**

  - Python3 support.
  - Fix OutOfMemoryError.

- **0.5.7 - 2014-10-29**

  - No JDK/JRE is required to build anymore due to provided jni.h. To
    override this, one needs to set a JAVA_HOME pointing to a JDK
    during setup.
  - Better support for various platforms and compilers (MinGW, Cygwin,
    Windows)

- **0.5.6 - 2014-09-27**

  - *Note*: In this release we returned to the three point number
    versioning scheme.
  - Fix #63: 'property' object has no attribute 'isBeanMutator'
  - Fix #70: python setup.py develop does now work as expected
  - Fix #79, Fix #85: missing declaration of 'uint'
  - Fix #80: opt out NumPy code dependency by '--disable-numpy'
    parameter to setup.  To opt out with pip
    append --install-option="--disable-numpy".
  - Use JVMFinder method of @tcalmant to locate a Java runtime

- **0.5.5.4 - 2014-08-12**

  - Fix: compile issue, if numpy is not available (NPY_BOOL
    n/a). Closes #77

- **0.5.5.3 - 2014-08-11**

  - Optional support for NumPy arrays in handling of Java arrays. Both
    set and get slice operators are supported. Speed improvement of
    factor 10 for setting and factor 6 for getting. The returned
    arrays are typed with the matching NumPy type.
  - Fix: add missing wrapper type 'JShort'
  - Fix: Conversion check for unsigned types did not work in array
    setters (tautological compare)

- **0.5.5.2 - 2014-04-29**

  - Fix: array setter memory leak (ISSUE: #64)

- **0.5.5.1 - 2014-04-11**

  - Fix: setup.py now runs under MacOSX with Python 2.6 (referred to
    missing subprocess function)

- **0.5.5 - 2014-04-11**

  - *Note* that this release is *not* compatible with Python 2.5 anymore!
  - Added AHL changes

    * replaced Python set type usage with new 2.6.x and higher
    * fixed broken Python slicing semantics on JArray objects
    * fixed a memory leak in the JVM when passing Python lists to
      JArray constructors
    * prevent ctrl+c seg faulting
    * corrected new[]/delete pairs to stop valgrind complaining
    * ship basic PyMemoryView implementation (based on numpy's) for Python 2.6 compatibility

  - Fast sliced access for primitive datatype arrays (factor of 10)
  - Use setter for Java bean property assignment even if not having a
    getter by @baztian
  - Fix public methods not being accessible if a Java bean property
    with the same name exists by @baztian (*Warning*: In rare cases
    this change is incompatibile to previous releases. If you are
    accessing a bean property without using the get/set method and the
    bean has a public method with the property's name you have to
    change the code to use the get/set methods.)
  - Make jpype.JException catch exceptions from subclasses by @baztian
  - Make more complex overloaded Java methods accessible (fixes
    https://sourceforge.net/p/jpype/bugs/69/) by @baztian and
    anonymous
  - Some minor improvements inferring unnecessary copies in extension
    code
  - Some JNI cleanups related to memory
  - Fix memory leak in array setters
  - Fix memory leak in typemanager
  - Add userguide from sourceforge project by @baztian

- **0.5.4.5 - 2013-08-25**

  - Added support for OSX 10.9 Mavericks by @rmangino (#16)

- **0.5.4.4 - 2013-08-10**

  - Rewritten Java Home directory Search by @marsam (#13, #12 and #7)
  - Stylistic cleanups of setup.py

- **0.5.4.3 - 2013-07-27**

  - Initial pypi release with most fixes for easier installation
