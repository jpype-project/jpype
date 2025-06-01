Changelog
=========

This changelog *only* contains changes from the *first* pypi release (0.5.4.3) onwards.

Latest Changes:
- **1.6.0 - 2025-05-31**
- **1.6.0 - 2025-01-20**

  - Java components have been converted to maven style module.
  
  - JArray is now registered as a Sequence.

  - Fixed conversion of float16 for subnormal numbers.

  - Fixed segmentation fault on null String.

  - Fixed bugs with java.util.List concat and repeat methods.

  - Enhancement to JProxy to handle wrapping an existing Python object with a Java
    interface.  

  - Fixed bug in which using an interface the derived from Map with JProxy failed.

  - Fixed a bug in which JPype did not respect a JConversion between two Java classes.

  - Enhancement in convertToDirectBuffer to support wrapping bytes and readonly
    memoryviews as readonly java ByteBuffers.

- **1.5.2 - 2025-01-20**

  - Roll back agent change due to misbehaving JVM installs.

  - Correct issues with non-ascii path for jdbc connections and forName.

- **1.5.1 - 2024-11-09**

  - Future proofing for Python 3.14

  - Support for Python 3.13

  - Allow access to default methods implemented in interfaces when using ``@JImplements``.

  - Added support for typing ``JArray`` (Java type only), e.g. ``JArray[java.lang.Object]`` ``"JArray[java.lang.Object]"``

  - Fixed uncaught exception while setting traceback causing issues in Python 3.11/3.12.

  - Use PEP-518 and PEP-660 configuration for the package, allowing editable and
    configurable builds using modern Python packaging tooling.
    Where before ``python setup.py --enable-tracing develop``, now can be done with
    ``pip install --editable ./ --config-setting="--install-option=--enable-tracing"``.
    The old setup.py usage remains, but is discouraged, and the arguments are now passed
    after the command (previously they were specified before).

  - Use PEP-518 configuration for the package, allowing
    configurable builds using more up-to-date Python packaging tooling.
    For editable installs, ``python setup.py --enable-tracing develop``
    must now be done with ``python setup.py develop --enable-tracing``.

  - Update for tests for numpy 2.0.

  - Support of np.float16 conversion with arrays.

  - Fixed a problem that caused ``dir(jpype.JPackage("mypackage"))`` to fail if
    the class path contained non-ascii characters. See issue #1194.

  - Fixed ``BufferOverflowException`` in ``JUnpickler`` when decoding
    multiple Java objects.


- **1.5.0 - 2023-04-03**

  - Support for Python 3.12

  - Switched ``__eq__`` and ``__ne__`` operator to use ``equals`` rather than
    ``compareTo`` for comparable objects to avoid exception when comparing
    object of different types.

  - Fixed segmentation fault when comparing Java Comparable to primitives.

  - Java exceptions that occur in inequality comparisons now map to Python
    TypeError.

- **1.4.2_dev0 - 2022-10-26**

  - Fixed crash when calling subscript on JArray.

  - Fixed direct byte buffers not reporting nbytes correctly when cast to
    memoryview.

  - Expand the defintion for Functional interface to include classes without
    FunctionInterface annotation.

  - Add additional matching level for derived types to resolve ambiguities when
    a derived type is used in place of base class when determining the method
    overload.  This will resolve some previous ambiguities between methods.  

- **1.4.1 - 2022-10-26**
  
  - Fixed issue with startJVM changing locale settings.

  - Changes to support Python 3.11

  - Fix truncation of strings on null when using convert strings.

  - Replaced distutil with packaging


- **1.4.0 - 2022-05-14**

  - Support for all different buffer type conversions.

    - Improved buffer transfers to numpy as guaranteed to match Java types.
      However, exact dtype for conversions is os/numpy version dependent.

    - Support for byte order channels on buffer transfers.

    - Byte size for buffers now fixed to Java definitions.
      
    - When directly accessing Java arrays using memory view, Python requires a
      cast from buffers.  Required because Python does not support memory view
      alterations on non-native sizes.

  - Fix crash when comparing JChar.

     - Order handling for numerical operations with JChar fixed.

  - Improved matching for Java functors based on parameter count.

  - Dropped support for Python 3.5 and 3.6

  - dbapi2 handles drivers that don't support autocommit.

  - Fixed issue when Java classes with dunder methods such as  ``__del__`` 
    caused conflicts in Python type system.   Java method which match dunder 
    patterns are longer translated to Python.

  - Fix issue with numpy arrays with no dimensions resulting in crash.

  - Support for user defined conversions for java.lang.Class and array types.

  - Fixed issue with ssize_t on Windows for Python 3.10.


- **1.3.0 - 2021-05-19**

  - Fixes for memory issues found when upgrading to Python 3.10 beta.

  - Add additional diagnositics for importing of non-public class.

  - Fixed issue with classes with unsatified dependencies leading to a crash
    on windows.
  
  - Fixed a bug with arrays created using the short cut.  The wrong type
    was being returned.

- **1.2.1 - 2021-01-02**

  - Missing stub files added.

  - Python 3.9 issues are resolved on Windows.

  - JPype scans jar files and rebuilding missing directories to allow imports
    from stripped and obfuscated jar files.

- **1.2.0 - 2020-11-29**

  - Added builds for Python 3.9.  Python 3.9 on Windows is currently failing
    due to issue in Python.

  - Fixed bug when importing from multi-release jars.  The directory was
    being truncated to only those classes in the overlay.

  - addClassPath can add jar files after the JVM is started.  The default
    loader for JPype class is ``org.jpype.classloader.DynamicClassLoader``.

  - Build support of z/OS added.

  - Bug causing ambiguity between primitives and variadic arguments in method
    resolution was corrected.

  - Boolean was inadvertently left out of method resolution.  ``boolean``
    now properly matched with boxed types.

  - Support for PyInstaller was added.

- **1.1.2 - 2020-10-23**

  - Linux binaries are now stripped for size.

  - Add importlib.util to address instability in Python importlib boot process.
    Certain versions of Python such as 3.9 appear to not properly load this
    module resulting in unexpected errors during startJVM.

- **1.1.1 - 2020-10-21**

  - Fixed packaging problem on linux.  

- **1.1.0 - 2020-10-13**
  
  - Correct bug resulting in reporting ambiguous overloads when resolving
    methods with variadic arguments.

  - Ctrl+C behavior is switchable with interrupt flag to startJVM.
    If True, process will halt on Ctrl-C.  If False, the process
    will transfer control to Python rather than halting.  If
    not specified JPype will assume false if Python is  started as an
    interactive shell.

  - Fixed crash with Ctrl+C when multiple exceptions were generated.

  - Removed extraneous exception when calling Ctrl+C before Java code is 
    executed for methods and fields.

  - Fixed memory leak with string cache.

  - Fixed crash when manually creating wrappers for anonymous classes.

  - Fixed reference count problem in stackframes used for exceptions.

  - Errors report `*static*` when the matching with a static method
    so that it is clear when a member method was called statically.

  - java.lang.String slices function like Python string slice.

  - Java packages now operate as normal Python modules.  Removed restrictions
    regarding setattr.  All package instances for the same package name are
    shared so that functionality added to one instance is shared wiht all
    instances.

- **1.0.2 - 2020-07-27**

  - The wrapper for Throwable was getting the wrapper for Object rather than
    the expected wrapper resulting in odd conversions from Python classes.

  - Typos within the import system resulting in "jname" not found corrected.

  - ^C propogates to a KeyboardInterrupt properly.

  - Added cache to the method dispatch to bypass resolution of overloads.
    This reduces the cost of method resolution significantly especially if
    the same overload is hit repeatedly such as during loop operations.

  - Improved speed on transfer of lists, tuples, buffers to arrays of Java
    primitives by a factor of 4 to 100 depending on the data type.  The
    conversion uses optimized path for memory buffers, rather than the 
    Sequence API.  When a Python buffer is encountered only the
    first element is checked for conversion as Python buffers are homogeneous. 

  - Corrected symbol problem with Python 3.5.3.  PySlice_Unpack was introduced
    in a later patch release and should not have been used.

  - **shutdown** The behavior log entry for changes on shutdown were lost in
    the 1.0 release.  JPype now calls the JVM shutdown routine which tries to
    gracefully exit when shutdown is called.  This results in several changes
    in behavior.  Non daemon threads can now hold open the JVM until they have
    completed.  Proxy calls will hold the shutdown until the call is completed
    but will receive an interrupt message. Files now close properly and will
    flush if the threads properly handle the exception.  Resource clean up
    hooks and finalizers are executed.  AtExit hooks in the JVM are called as
    spawned threads.  Automatic attachment of threads by use of the JVM from
    Python are done as daemon but can be reattached as user threads on demand.
    Buggy code that fails to properly handle thread clean up will likely hang
    on shutdown.  Additional documentation is located in the user guide.

  - A bug was reported with numpy.linalg.inv resulting in crashes.  This was
    traced to an interaction with threading between the JVM and some compilations
    of numpy.  The workaround appears to be calling numpy.linalg.inv prior to 
    starting the JVM.

- **1.0.1 - 2020-07-16**

  - Workarounds for Python 3.8.4 release.  Python altered logic regarding the
    use of ``__setattr__`` for object and type, preventing it from being used
    to alter derived classes.  Also the checking for errors was delegated from
    the ``__setattr__`` method so exception types on some sanity checks 
    needed to be updated accordingly.

- **1.0.0 - 2020-07-12**

  - ``JChar`` is supported as a return type, thus rather than returning a
    string where a ``JChar`` is expected.  For compatiblity ``JChar`` is
    derived from ``str`` and implements implicit conversion to an ``int`` when
    used in numeric operations. Therefore, it passes the return, argument, and
    field contracts.  But that means it is no longer considered a numerical
    type to Python and thus ``isinstance(c, int)`` is False.  This is
    consistent with the Java type conversion rules.

  - Introduced Python operator for Java casting.  In Java to cast
    to a type you would use ``(Type) obj``, but Python does not support
    anything similar.  Therefore, we are enlisting the rarely used 
    ``matmul`` operator as to allow an easy way to cast an object
    to a Java type.  When a cast to a Java type is required, use
    ``Type@obj`` or ``(Type)@obj``.  

  - Introduced array notation to create Java arrays.  In earlier versions,
    JArray factory was required to make a new array type.  But this is
    tedious to read.  In Java the notation would be ``Type[]`` to declare
    a type or ``new Type[sz]`` to make a new array.  Python does not 
    directly support this notation, but it does allow for unspecifed 
    array sizes using a slice.  All Java class types support
    ``Type[sz]`` to create an array of a fixed size and ``Type[:]`` to 
    create an array type which can be intiated later.   This call be applied
    to multiple dimensions to create fixed sized arrays ``Type[s1][s2][s3]``
    to declare multidimension array types ``Type[:][:][:]`` or to 
    create a new multi dimensional array with unspecified dimensions
    ``Type[sz][:][:]``.  Applying a slice with limits to a class is
    unsupported.

  - Java classes annotated with ``@FunctionalInterface`` can be 
    converted from any Python object that implements ``__call__``. 
    This allows functions, lambdas, and class constructors to be used
    whereever Java accepts a lambda.

  - Support for Protocol on type conversions.  Attribute based
    conversions deprecated in favor of Protocol.  Internal API
    for stubbing.

  - Deprecated class and functions were removed.  ``JIterator``,
    use of ``JException`` as a factory,  ``get_default_jvm_path``,
    ``jpype.reflect`` module.

  - Default for starting JVM is now to return Java strings rather
    than convert.

  - Python deprecated ``__int__`` so implicit conversions between
    float and integer types will produce a ``TypeError``.

  - Use of ``JException`` is discouraged.  To catch all exceptions
    or test if an object is a Java exception type, 
    use ``java.lang.Throwable``.

  - Chained Java exception causes are now reflected in the Python stackframes.

  - Use of ``JString`` is discouraged.  To create a Java string or
    test if an object is a Java string type, use ``java.lang.String``.

  - Updated the repr methods on Java classes.

  - ``java.util.List`` completes the contract for ``collections.abc.Sequence``
    and ``collections.abc.MutableSequence``.

  - ``java.util.Collection`` completes the contract for ``collections.abc.Collection``.
  
  - Java classes are closed and will raise ``TypeError`` if extended in Python.

  - Handles Control-C gracefully.  Previous versions crash whenever
    Java handles the Control-C signal as they would shutdown Java
    during a call.  Now JPype will produce a ``InterruptedException``
    when returning from Java.  Control-C will not break out of large
    Java procedures as currently implemented as Java does not have
    a specific provision for this.

- **0.7.5 - 2020-05-10**

  - Updated docs.

  - Fix corrupt conda release.

- **0.7.4 - 4-28-2020**

  - Corrected a resource leak in arrays that affects array initialization, and variable
    argument methods.  

  - Upgraded diagnostic tracing and JNI checks to prevent future resource leaks.

- **0.7.3 - 4-17-2020**

  - **Replaced type management system**, memory management for internal
    classes is now completely in Java to allow enhancements for
    buffer support and revised type conversion system.

  - Python module ``jpype.reflect`` will be removed in the next release.  
    
  - ``jpype.startJVM`` option ``convertStrings`` default will become False
    in the next release.

  - Undocumented feature of using a Python type in ``JObject(obj, type=tp)`` 
    is deprecated to support casting to Python wrapper types in Java in a 
    future release.

  - Dropped support for Cygwin platform.

  - ``JFloat`` properly follows Java rules for conversion from ``JDouble``.
    Floats outside of range map to inf and -inf.

  - ``java.lang.Number`` converts automatically from Python and Java numbers.
    Java primitive types will cast to their proper box type when passed
    to methods and fields taking Number.

  - ``java.lang.Object`` and ``java.lang.Number`` box signed, sized numpy types
    (int8, int16, int32, int64, float32, float64) to the Java boxed type
    with the same size automatically.  Architecture dependent numpy
    types map to Long or Double like other Python types.

  - Explicit casting using primitives such as JInt will not produce an
    ``OverflowError``.  Implicit casting from Python types such as int or float
    will.

  - Returns for number type primitives will retain their return type
    information.  These are derived from Python ``int`` and ``float`` types
    thus no change in behavior unless chaining from a Java methods
    which is not allowed in Java without a cast.
    ``JBoolean`` and ``JChar`` still produce Python types only.

  - Add support for direct conversion of multi-dimensional primitive arrays
    with ``JArray.of(array, [dtype=type])``

  - ``java.nio.Buffer`` derived objects can convert to memoryview if they
    are direct.  They can be converted to NumPy arrays with
    ``numpy.asarray(memoryview(obj))``.

  - Proxies created with ``@JImplements`` properly implement ``toString``, 
    ``hashCode``, and ``equals``.

  - Proxies pass Python exceptions properly rather converting to
    ``java.lang.RuntimeException``

  - ``JProxy.unwrap()`` will return the original instance object for proxies
    created with JProxy.  Otherwise will return the proxy.

  - JProxy instances created with the ``convert=True`` argument will automatic
    unwrap when passed from Java to Python.

  - JProxy only creates one copy of the invocation handler per
    garbage collection rather than once per use.  Thus proxy objects
    placed in memory containers will have the same object id so long
    as Java holds on to it.

  - jpype.imports and JPackage verify existance of packages and classes.
    Imports from Java packages support wildcards.  

  - Bug with JPackage that imported private and protected classes
    inappropriately has been corrected.  Protected classes can still be
    imported using JClass.

  - Undocumented feature of using a Python type in ``JObject(obj, type=tp)`` 
    is deprecated to support casting to Python wrapper types in Java in a 

  - ``@JImplements`` with keyword argument ``deferred`` can be started 
    prior to starting the JVM.  Methods are checked at first object
    creation.

  - Fix bug that was causing ``java.lang.Comparable``, ``byte[]``,
    and ``char[]`` to be unhashable.

  - Fix bug causing segfault when throwing Exceptions which lack a
    default constructor.

  - Fixed segfault when methods called by proxy have incorrect number of
    arguments.

  - Fixed stack overflow crash on iterating ImmutableList

  - ``java.util.Map`` conforms to Python ``collections.abc.Mapping`` API.

  - ``java.lang.ArrayIndexOutOfBoundsException`` can be caught with
    ``IndexError`` for consistency with Python exception usage.

  - ``java.lang.NullPointerException`` can be caught with ``ValueError``
    for consistency with Python exception usage.

  - **Replaced type conversion system**, type conversions test conversion
    once per type improving speed and increasing flexiblity.

  - User defined implicit conversions can be created with ``@JConversion``
    decorator on Python function taking Java class and Python object.
    Converter function must produce a Java class instance.

  - ``pathlib.Path`` can be implicitly converted into ``java.lang.File``
    and ``java.lang.Path``.  

  - ``datetime.datatime`` can implicitly convert to ``java.time.Instant``.

  - ``dict`` and ``collections.abc.Mapping`` can convert to ``java.util.Map``
    if all element are convertable to Java.  Otherwise, ``TypeError`` is
    raised.

  - ``list`` and ``collections.abc.Sequence`` can convert to ``java.util.Collection``
    if all elements are convertable to Java.  Otherwise, ``TypeError`` is
    raised.

- **0.7.2 - 2-28-2020**

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

  - Java arrays support memoryview API and no longer required NumPy
    to transfer buffer contents.

  - Numpy is no longer an optional extra.  Memory transfer to NumPy
    is available without compiling for numpy support.

  - JInterface is now a meta class.  Use ``isinstance(cls, JInterface)``
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
