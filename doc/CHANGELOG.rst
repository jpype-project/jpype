Changelog
=========

This changelog *only* contains changes from the *first* pypi release (0.5.4.3) onwards.

- **Next version - unreleased**
  - java.util.Map conforms to Python abc.Mapping API.

  - float properly follows Java rules for conversion from double.
    floats outside of range map to inf and -inf.

  - Fix bug that was causing java.lang.Comparable, byte[], and char[] to be unhashable.

  - Corrected an issue with creation of Exceptions which lack a
    default constructor.

- **0.7.2 - 2-28-2019**

  - C++ and Java exceptions hold the traceback as a Python exception
    cause.  It is no longer necessary to call stacktrace() to retrieve
    the traceback information.

  - Speed for call return path has been improved by a factor of 3.

  - Multidimensional array buffer transfers increase speed transfers
    to numpy substantially (orders of magnitude).  Multidimension primitive
    transfers are read-only copies produced inside the JVM with C contiguous
    layout.

  - All exposed internals have been replaced with CPython implementations
    thus symbols `__javaclass__`, `__javavalue__`, and `__javaproxy__`
    have been removed.  A dedicated Java slot has been added to all CPython
    types derived from `_jpype` class types.  All private tables have been
    moved to CPython.  Java types must derive from the metaclass `JClass`
    which enforces type slots.  Mixins of Python base classes is not
    permitted.  Objects, Proxies, Exceptions, Numbers, and Arrays 
    derive directly from internal CPython implementations.
    See the :doc:`ChangeLog-0.7.2` for details of all changes.

  - Internal improvements to tracing and exception handling.

  - Memory leak in convertToDirectBuffer has been corrected.

  = Arrays slices are now a view which support writeback to the original
    like numpy array.  Array slices are no longer covariant returns of 
    list or numpy.array depending on the build procedure.

  - Array slices support steps for both set and get.

  - Arrays now implement `__reversed__`

  - Incorrect mapping of floats between 0 and 1 to False in setting
    Java boolean array members is corrected.

  - Java arrays now properly assert range checks when setting elements
    from sequences.

  - Java arrays support memoryview API and no longer required numpy
    to transfer buffer contents.

  - Numpy is no longer an optional extra.  Memory transfer to numpy
    is available without compiling for numpy support.

  - JInterface is now a meta class.  Use isinstance(cls, JInterface)
    to test for interfaces.

  - Fixed memory leak in Proxy invocation

  - Fixed bug with Proxy not converting when passed as an argument to
    Python functions during execution of proxies

  - Missing tlds "mil", "net", and "edu" added to default imports.

  - Enhanced error reporting for UnsupportedClassVersion during startup.

  - Corrections for collection methods to improve complience with
    Python containers.  
    
    - java.util.Map gives KeyError if the item is not found.  Values that
      are ``null`` still return ``None`` as expected.  Use ``get()`` if 
      empty keys are to be treated as ``None``.
      
    - java.util.Collection ``__delitem__`` was removed as it overloads
      oddly between ``remove(Object)`` and ``remove(int)`` on Lists.
      Use Java ``remove()`` method to access the original Java behavior,
      but a cast is strongly recommended to to handle the overload.

  - java.lang.IndexOutOfBoundsException can be caught with IndexError
    for complience when accessing ``java.util.List`` elements.


- **0.7.1 - 12-16-2019**

  - Updated the keyword safe list for Python 3.

  - Automatic conversion of CharSequence from Python strings.

  - java.lang.AutoCloseable supports Python "with" statement.

  - Hash codes for boxed types work properly in Python 3 and can be
    used as dictionary keys again (same as JPype 0.6).  Java arrays
    have working hash codes, but as they are mutable should not 
    be used as dictionary keys.  java.lang.Character, java.lang.Float,
    and java.lang.Double all work as dictionary keys, but due to
    differences in the hashing algorithm do not index to the same
    location as Python native types and thus may cause issues
    when used as dictionary keys. 

  - Updated getJVMVersion to work with JDK 9+.

  - Added support for pickling of Java objects using optional module
    ``jpype.pickle``

  - Fixed incorrect string conversion on exceptions.  `str()` was
    incorrectly returning `getMessage` rather than `toString`.

  - Fixed an issue with JDK 12 regarding calling methods with reflection.
  
  - Removed limitations having to do with CallerSensitive methods. Methods
    affected are listed in :doc:`caller_sensitive`. Caller sensitive 
    methods now receive an internal JPype class as the caller

  - Fixed segfault when converting null elements while accessing a slice
    from a Java object array.

  - PyJPMethod now supports the FunctionType API.

  - Tab completion with Jedi is supported.  Jedi is the engine behind
    tab completion in many popular editors and shells such as IPython.
    Jedi version 0.14.1 is required for tab completion as earlier versions 
    did not support annotations on compiled classes.  Tab completion
    with older versions requires use of the IPython greedy method.

  - JProxy objects now are returned from Java as the Python objects 
    that originate from. Older style proxy classes return the 
    inst or dict. New style return the proxy class instance.
    Thus proxy classes can be stored on generic Java containers
    and retrieved as Python objects.

- **0.7.0 - 2019**

  - Doc strings are generated for classes and methods.

  - Complete rewrite of the core module code to deal unattached threads,
    improved hardening, and member management.  Massive number of internal 
    bugs were identified during the rewrite and corrected.
    See the :doc:`ChangeLog-0.7` for details of all changes.

  - API breakage:

     - Java strings conversion behavior has changed.  The previous behavior was
       switchable, but only the default convert to Python was working.
       Converting to automatically lead to problems in which is was impossible
       to work with classes like StringBuilder in Java. To convert a Java
       string use ``str()``. Therefore, string conversion is currently selected
       by a switch at the start of the JVM.  The default shall be False
       starting in JPype 0.8.  New code is encouraged to use the future default
       of False.  For the transition period the default will be True with a
       warning if not policy was selected to encourage developers to pick the
       string conversion policy that best applies to their application.

     - Java exceptions are now derived from Python exception. The old wrapper
       types have been removed. Catch the exception with the actual Java
       exception type rather than ``JException``.
       
     - Undocumented exceptions issued from within JPype have been mapped to the
       corresponding Python exception types such as ``TypeError`` and
       ``ValueError`` appropriately.  Code catching exceptions from previous
       versions should be checked to make sure all exception paths are being
       handled.

     - Undocumented property import of Java bean pattern get/set accessors was
       removed as the default. It is available with ``import jpype.beans``, but
       its use is discouraged.

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
