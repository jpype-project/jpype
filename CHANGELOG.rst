Changelog
=========

This changelog *only* contains changes from the *first* pypi release (0.5.4.3) onwards.

0.5.4.6 (to be released)
~~~~~~~~~~~~~~~~~~~~~~~~~~

*Note* that this release is *not* compatible with Python 2.5 anymore!

* Added AHL changes

  * replaced Python set type usage with new 2.6.x and higher
  * fixed broken Python slicing semantics on JArray objects
  * fixed a memory leak in the JVM when passing Python lists to JArray constructors
  * prevent ctrl+c seg faulting
  * corrected new[]/delete pairs to stop valgrind complaining
  * ship basic PyMemoryView implementation (based on numpy's) for Python 2.6 compatibility

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
* Fast sliced access for primitive datatype arrays (factor of 10)
* some minor improvements inferring unnecessary copies in extension code
* minor testsuite cleanups: 
    1. Simplify test modules and make all tests runnable via python -m unittest.
    No need to implement a suite() method. Tests can be run via
    jpype/test$ python -m unittest jpypetest.numeric
    jpype/test$ python -m unittest jpypetest.attr.AttributeTestCase.testCallStaticString jpypetest.numeric
    jpype/test/jpypetest$ python -m unittest numeric
    2. use unittest asserts instead of plain asserts in some modules (not yet complete)
* some JNI cleanups (to be merged)
0.5.4.5 (2013-08-25 12:12)
~~~~~~~~~~~~~~~~~~~~~~~~~~

* Added support for OSX 10.9 Mavericks by @rmangino (#16)

0.5.4.4 (2013-08-10 19:30)
~~~~~~~~~~~~~~~~~~~~~~~~~~

* Rewritten Java Home directory Search by @marsam (#13, #12 and #7)
* Stylistic cleanups of setup.py

0.5.4.3 (2013-07-27 14:00)
~~~~~~~~~~~~~~~~~~~~~~~~~~

Initial pypi release with most fixes for easier installation
