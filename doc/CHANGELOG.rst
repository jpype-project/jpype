Changelog
=========

This changelog *only* contains changes from the *first* pypi release (0.5.4.3) onwards.

0.5.7
-----
* No JDK/JRE is required to build anymore due to provided jni.h. To override
  this, one needs to set a JAVA_HOME pointing to a JDK during setup.
* better support for various platforms and compilers (MinGW, Cygwin, Windows) 

0.5.6
-----
*Note*: In this release we returned to the three point number versioning scheme.

* Fix #63: 'property' object has no attribute 'isBeanMutator'
* Fix #70: python setup.py develop does now work as expected
* Fix #79, Fix #85: missing declaration of 'uint'
* Fix #80: opt out NumPy code dependency by '--disable-numpy' parameter to setup.
  To opt out with pip append --install-option="--disable-numpy".
* Use JVMFinder method of @tcalmant to locate a Java runtime

0.5.5.4
-------
* Fix: compile issue, if numpy is not available (NPY_BOOL n/a). Closes #77

0.5.5.3
-------
* Optional support for NumPy arrays in handling of Java arrays. Both set and get
  slice operators are supported. Speed improvement of factor 10 for setting and
  factor 6 for getting. The returned arrays are typed with the matching NumPy type.
* Fix: add missing wrapper type 'JShort'
* Fix: Conversion check for unsigned types did not work in array setters (tautological compare)  

0.5.5.2
-------
* Fix: array setter memory leak (ISSUE: #64)

0.5.5.1
-------
* Fix: setup.py now runs under MacOSX with Python 2.6 (referred to missing subprocess function)

0.5.5
-----

*Note* that this release is *not* compatible with Python 2.5 anymore!

* Added AHL changes

  * replaced Python set type usage with new 2.6.x and higher
  * fixed broken Python slicing semantics on JArray objects
  * fixed a memory leak in the JVM when passing Python lists to JArray constructors
  * prevent ctrl+c seg faulting
  * corrected new[]/delete pairs to stop valgrind complaining
  * ship basic PyMemoryView implementation (based on numpy's) for Python 2.6 compatibility

* Fast sliced access for primitive datatype arrays (factor of 10)
* Use setter for Java bean property assignment even if not having a
  getter by @baztian
* Fix public methods not being accessible if a Java bean property with
  the same name exists by @baztian (*Warning*: In rare cases this
  change is incompatibile to previous releases. If you are accessing a
  bean property without using the get/set method and the bean has a
  public method with the property's name you have to change the code
  to use the get/set methods.)
* Make jpype.JException catch exceptions from subclasses by @baztian
* Make more complex overloaded Java methods accessible (fixes https://sourceforge.net/p/jpype/bugs/69/) by @baztian and anonymous
* Some minor improvements inferring unnecessary copies in extension code
* Some JNI cleanups related to memory
* Fix memory leak in array setters
* Fix memory leak in typemanager
* Add userguide from sourceforge project by @baztian

0.5.4.5 (2013-08-25 12:12)
--------------------------

* Added support for OSX 10.9 Mavericks by @rmangino (#16)

0.5.4.4 (2013-08-10 19:30)
--------------------------

* Rewritten Java Home directory Search by @marsam (#13, #12 and #7)
* Stylistic cleanups of setup.py

0.5.4.3 (2013-07-27 14:00)
--------------------------

Initial pypi release with most fixes for easier installation
