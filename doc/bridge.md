# Type Mapping
One of the challenging tasks for the Java to Python bridge is to select the
correct set of interfaces to implement that presents the available behaviors 
for each Python class as Java interfaces.

This is represented in two ways.

 - A list of interfaces that describes the functional behavior of
   the object in Java.

 - A dictionary that provides a mapping from Java function names
   to Python callables.

With these two objects we can create a JProxy that captures 
the behaviors of the Python object in Java.

We will break the wrappers into two types.

 - Protocol wrappers are entirely behavioral which look at the 
   dunder functions of the class an present that behavior to 
   user.

 - Concrete mappings are specializations to each of the 
   Python builtin classes.  We would like to support
   additional late specialized wrappers by the user.
   (See Specialized Wrappers)


## Probing

Probes will take place in two stages.  First we need to collect 
a list protocols that are supported for the object.  These will be
provided as a bit mapped field.  

There are three ways we can probe for concrete types.  We can consult the mro
to see what is the second type slot.  For objects that haven't reordered
their mro this will be successful.  We can consult the `tp_flags` for those
types that Python has already accelerated support to check for.
We can perform a list of isinstance searches.  The last has the significant
downside that it will end up O(n).


Concrete Types:

- PyByteArray
- PyBytes - inherits from byte  `Py_TPFLAGS_BYTES_SUBCLASS`
- PyComplex
- PyDict - inherits from dict  `Py_TPFLAGS_DICT_SUBCLASS`
- PyEnumerate
- PyLong - inherits from int  `Py_TPFLAGS_LONG_SUBCLASS`
- PyExceptionBase - inherits from exception  `Py_TPFLAGS_BASE_EXC_SUBCLASS`
- PyList - inherits from list `Py_TPFLAGS_LIST_SUBCLASS`
- PyMemoryView
- PyObject - Base class and special case when an object has len(mro)==1
- PyRange
- PySet
- PySlice
- PyString - inherits from str  `Py_TPFLAGS_UNICODE_SUBCLASS`
- PyTuple - inherits from list  `Py_TPFLAGS_TUPLE_SUBCLASS`
- PyType - inherits from type  `Py_TPFLAGS_TYPE_SUBCLASS`
- PyZip


Protocols include:

- PyASync - Abstract interface for classes with the async behavior.
  Identify with `tp_as_async` slot.

- PyAttributes - Virtual map interface to the attributes of an object.
  Python has two types of map object in getattr/setattr and getitem/setitem.
  As we can't have the same function mapping for two behaviors we 
  will split into a seperate wrapper for this type.

- PyBuffer - Abstract interface for objects which look like memory buffers.
  Identify with `tp_as_buffer`

- PyCallable - Abstraction for every way that an object can be called.  
  This will have methods to simplify keyword arguments in Java syntax.
  Identify with `tp_call` slot.

- PyComparable - Class with the `tp_richcompare` slot.

- PyGenerator - An abstraction for an object which looks both like a iterable and a iterator.
  Generators are troublesome as Java requires that every use of an iterable starts
  back from the start of the collection.  We will need create an illusion of this.
  Identify with `tp_iter` and `tp_next`

- PyIterable - Abstract interface that can be used as a Java Iterable.
  Identify with `tp_iter` slot.

- PyIter - Abstract interface that can be converted to a Java Iterator.  
  PyIter is not well compatible with the Java representation because
  it requires a lot of state information to be wrapped, thus we 
  will need both a Python like and a Java like instance.  The 
  concrete type PyIterator is compatible with the Java concept.
  Identify with `tp_next`  (the issue is many Python iter also
  look like generators, so may be hard to distiguish.)

- PySequence - Abstract interface for ordered list of items.
  Identify with `Py_TPFLAGS_SEQUENCE`

- PyMapping - Abstract interface that looks like a Java Map.
  Identify with `Py_TPFLAGS_MAPPING`

- PyNumber - Abstraction for class that supports some subset of numerical operations.
  This one will be a bit hit or miss because number slots can mean
  number like, matrix like, array like, or some other use of 
  operators.


One special case exists which happens when a Java object is passed through a
Python interface as a return.  In such a case we have no way to satify the type
relationship of being a PyObject.  Thus we will prove a PyObject wrapper that
holds the Java object. 

We will loosely separate these two types of iterfaces into two Java packages
`python.lang` and `python.protocol`.  There are also numerous private internal
classes that are used in wrapping.


## Presentation

Python objects should whereever possible conform to the nearest Java concept
if possible.  This means the method names will often need to be remapped to 
Java behaviors.  This is somewhat burdensome when Java has a different 
concept of what is returned or different argument orders.  We don't need
to be fully complete here as the user always has the option of using the 
builtin methods or using a string eval to get at unwrapped behavior.

### Name Conflicts

Python has two behaviors that share the same set of slots by have very
different meanings.  Sequence and Mapping both use the dunder functions
getitem/setitem but in one case it can only accept an index and the other any
object.  When these are wrapped they map to two difference collections on the
Java side which have extensive name conflicts.  Thus the wrapping algorithm
must ensure that these behaviors are mutually exclusive.

## Specialized Wrappers

In addition, to the predefined wrappers of Python concrete classes, we may want
to provide the user with some way to add additional specializations.  For
example, wrappers for specific classes in nump, scipy, and matplotlib may be
desireable.  

To support this we need a way for a Java module to register is wrappers and
add them interface and dictionaries produced by the probe.   There is also
a minor issue that the interpret may not be active when the Jar is loaded.

Potential implementations:

-  Initialization of a static field to call registerTypes() would be one option.
But Java is lazy in loading classes.  That means that if we probe a class
before the corresponding class is loaded we will be stuck with a bad wrapper.

- JNI onload function is guaranteed to be called when the jar is first 
encountered.  But is has the significant disadvantage that it must 
be tied to a machine architeture.  

- Java module services may be able to force a static block to implement.
This will take experimentation as the behavior of this function is 
not well documented.



## Internal implemention

### Converters

We will need to define two converters for each Python type.  One which gives
the most specific type such that if Java requests a specialized type it is
guaranteed to get it (or get a failure exception) and a second for the base
class PyObject in which Java has requested an untyped object.


### Probe Goals

The system must endever to satisfy these goals.

1) Minimize the number of probes for each object type.
   (Probing will always be an expensive operation)

2) Memory efficient
   (many types will share the same list of interfaces and dictionary, which
   means that whereever possible we will want to join like items in the table.)


To satify these goals we will use a caching strategy.

We can consolidate the storage of our entities by using three maps.

- WeakKeyDict(Type, (Interfaces[], Dict)) holds a cached copy of the results
  of every probe.  This will be consulted first to avoid unnecessary probe 
  calls.

- Dict(Interfaces[], Interfaces[]) will hold the unique tuple of 
  interfaces that were the results of the probes.  When caching we form
  the dynamic desired tuple of interfaces, then consult this map to get
  the correct instance to store in the cache.

- Dict(Interfaces[], Dict) as the Java methods are fixed based on the 
  interfaces provided, we only need one dict for each unique set of interfaces.

These will all be created at module load time.


### Limitations
As probing of classes only happens once, any monkey patching of classes will
not be reflected in the Java typing system.  Any changes to the type dictionary
will likely result in broken behaviors in the Java representation.


### Exceptions

Exceptions are a particularly challenging type to wrap.  To present an exception 
in Java, we need to deal with fact that both Python and Java choose to use
concrete types.  There is no easy way to use a proxy.  Thus Python
execptions will be represented in two pieces.  An interface based one will
be used to wrap the Python class in a proxy.  A concrete type in Java will
redirect the Java behaviors to Python proxy.   This may lead to some
confusion as the user will can encounter both copies even if we try to
keep the automatically unwrapped.  (Ie. Java captures an exception and 
places it in a log, then Python is implementing a log listener and gets
the Java version of the wrapper.)

