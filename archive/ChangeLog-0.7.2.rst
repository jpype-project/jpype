:orphan:

Buffers and NumPy removal
=========================

NumPy was used primarily for supplying Python buffers on slicing.  Numpy has
always been problematic for JPype.  As an optional extra is may or may not be
built into the distribution, but if it is compiled in it is required.
Therefore, is itn't really on "extra".  Therefore, removing it would make
distribution for binary versions of JPype much easier.

NumPy returns on slicing is but one part of the three uses of Python buffers in
JPype.  Thus to properly remove it we need to rework to remove it we need to
review all three of the paths.  These paths are

 - Conversion of Python buffers to Java arrays on setArrayRange. 
 - Conversion of Java arrays to slices (and then from slices to buffers so that
   they can be transferred to NumPy.)
 - Connecting bytearray to Java direct byte buffers.

We reviewed and revised each of the paths accordingly.

On conversion of Python buffers, the implementation was dated from Python 2.6
era where there was no formal support for buffers.  Thus the buffer
implementation never consulted the buffer type to see what type of object was
being transferred nor how the memory was oriented.  This entire section had to
be replaced and was given the same conversion rules as NumPy.  Any conversion is
possible (including lossy ones like float to bool).  The rules to trigger the
conversion is by slicing just as with NumPy.  By replicating the rules of NumPy
we hide the fact that NumPy is no longer used and increase typesafety.  There
was a number of cases where in the past it would reinterpret cast the memory
that will now function properly.  The old behavior was a useless side effect of
the implementation and was unstable with machine architecture so not likely used
be a user.

The getArrayRange portion has to be split into two pieces.  Under the previous
implementation the type of the return changes from Python list to NumPy array
depending on the compile option.  Thus the test suite tested different
behaviors for each.  In removing NumPy we replace the Java array slice return
with a Java array.  Thus the type is always consistent.  The Java array that is
returned is still backed by the same array as before and has the start, end, and
step set appropriately.  This does create one change in behavior as the slice
now has left assignment (just like NumPy) and before it was a copy.  It is
difficult exercise this but to do so we have to copy a slice to a new variable
then use a second array dereference to assign an element.  Because we converted 
to either list or NumPy, the second dereference could not affect the original.

There is no way to avoid this behavior change without adding a large transaction 
cost.  Which is why NumPy has the exact same behavior as our replacement
implementation.  We could in principle make the slice read only rather than
allowing for double slicing, but that would also be an API change.  

There is one other consequence of producing a view of the original having to do
with passing back to Java.  As Java does not recognize the concept of an array
view we must force it back to a Java type at the JNI level.  We will force copy
the array slice into a Java array at that time.  Thus replicates the same
functionality.  This induces one special edge case as `java.lang.Object` and
`java.lang.Serializable` which are both implemented by arrays must be aware of
the slice copy.

Before we trigger the conversion to NumPy or list by calling the slice without
limits operator `[:]` on the array.  Under the new implementation this is
effectively a no-op.  Thus we haven't broken or forced any changes in the API. 

The third case of direct byte buffers also revealed problems.  Again the type of
the buffer was not being check resulting in weird reinterpret cast like
behaviors.  Additionally, the memory buffer interface was making a very bad
assumption about the referencing of the buffer by assuming that referencing the
object that holds the buffer is the same as referencing the buffer itself.  It
was working only because the buffer was being leaked entirely and was likely
possible to break under situations as the leaked buffer essentially locked the
object forever.

To implement all of this properly unfortunately requires making the Python
wrapper of Java arrays a direct type.  This is possible in JPype 0.8 series
where we converted all classes to CPython roots, thus our only choice is to
backport the JPype speed patch into JPype 0.7.

API change summary
------------------

 - The type of a slice is no longer polymorphic but is always a Java array now.
 - The unbounded slice is now a no op.
 - Buffer types now trigger conversion routines rather than reinterpret casting 
   the memory.
 - Direct buffers now guard the memory layout such that they work only with
   mutable bytearray like types.
 - Assignment of elements though double slicing of an array now affects the
   original rather than just doing nothing effective like before.


JPype Speed Patch
=================

Speed has always been an issue for JPype.  While the interface of JPype is great
the underlying implementation is wanting.  Part of this was choices made early
in the development that favors ease of implementation over speed.  This forced a
very thin interface using Python capsules and then the majority of the code in
a pure Python module.  

The speed issue stems from two main paths.  The first being the method
resolution phase in which we need to consider each method overload argument by
argument which means many queries applied to each object.  The second is the
object construction penality when we return a Java object back to Python.  The
object bottleneck is both on the cost of the wrappers we produce, but
additionaly all of the objects that are constructed to communicate with a Python
function.  Every int, list, string and tuple we use in the process is another
object to construct.  Thus returning one object triggers dozens of object to be
constructed.

We have addressed these problems in five ways

 - Improve resolution of methods by removing the two phase approach to resolving
   a type match cutting the resolution time in half.
 - Caching the types in C so that they don't have to go back to Python to execute a
   method to check the Python cache and construct if necessary.
 - Converting all of the base types to CPython so that they can directly access
   the C++ guts without going back through entry points.
 - Remove all Python new and init method from the Java class tree so that we
   don't leave C during the construction phase.  Thus avoiding having to
   construct Python objects for each argument used during object construction.
 - Adding a Java slot to so that we can directly access Java resources both
   increasing the speed of the queries and saving us an additional object
   during construction.

All of these are being implemented for JPype 0.8 series.  For now we are
backporting the last four to the JPype 0.7 series.  The first is not possible to
backport as it requires larger structural changes.

Lets briefly discuss each of the items


Method resolution 
-----------------

During method resolution the previous implementation had a two phase approach.
First is tried to match the arguments to the Java types in canConvertToJava.  In
this each Java argument had to walk through each possible type conversion and
decide if the conversion is possible.  Once the we have the method overload
resovled we then perform the conversion by calling convertToJava.  That would
then walk through each possible type conversion to find the right one and apply
it.  Thus we did tha work twice.

To prevent this from happening we need to reserve memory for each argument we
are trying to convert.  When we walk through the list and find a conversion
rather than just returning true, we place a pointer to the conversion routine.
That way when we call convertToJava we don't have to walk the list a second time
but instead go straight to the pointer to get the routine to execute. 

This change has two additional consequences. First the primary source of bugs in
the type conversion was a mismatch between canConvertToJava and convertToJava
thus we are removing that problem entirely.  The second and more important to
the user is that the type system is now open.  By installing a routine we can
now add a user rule.  Therefore if we need `java.sql.TimeStamp` to accept a
Python time object we just need to add this to the type conversion table at the
Python level.  This is implemented in the ClassHints patch.  About half of our
customizer code was to try to achieve this on a per method level.  Thus this
elimiates a lot of our current Python customizer code.  The remaining customizer
code is to rename Java methods to Python methods and that will remain.


Caching of Python wrappers
--------------------------

In the previous implementation there was a text keyed dictionary that was
consulted to get type wrappers.  To access it C++ called to a Python function
that decided when to return a cached type and when to create a new one.  This
meant dozens of object constructed just to find the wrapper.  To solve this we
simply move the cache and add it to the JClass directly.  We have to back
reference the Python class so it can't go away while the JVM is running.

There is one section of code that also uses the wrapper dict in the customizers
which needs to decided does a wrapper already exist for the customizer.  We have
replaced these calls with methods on the module.



Conversion of the Base classes
------------------------------

JPype has a number of base classes (object, primitive, exception, string, array)
which hold the methods for the class.  If they are implemented as pure Python
than every access from C++ to these elements needs to create objects accordingly
when then are passed back through the module entry points to get back to C++.

We can avoid this by implementing each of these in CPython first at the module
layer and then extending them in the exposed module so that they have the same
outward appearance as before.  

We made one refinement during the conversion by implementing all of the CPython
classes using the Heep type API which has the distinct advantage that unlike
static types, it can be changed at runtime.  Thus from Python we can add
behavior to the heap types simply with by using `type.__setattr_`.  This was
a bit of a challenge as the documentation on heap types is much more sparse than
for static types.  However, after going through the process I would recommend
that all new CPython modules should use heap types rather than static as API is
much better and the result much more flexable and stable.  The only downside
being the memory footprint increases from 400 bytes to 900 bytes.  There are a
few rough spots in the heap type API in that certain actions like setting the
Buffer have to be added to the type after creation, but otherwise it is a big
improvement.  Now if all of the documentation would just drop the old static API
in favor of heap types it would be great.


Constructor simplifications
---------------------------

In order to benefit from moving all of the base classes to C, we have to make
sure that derived classes do not transfer control back to Python.  Currently
this happens due to the factory nature of our classes.  The entry point for
JObject is shared between the construction of objects from Python and a return
from Java.  Thus we have to either separate the factory behavior by pushing
those types out of the type tree or pushing the factory behavior into the C
layer.

We have chosen to split the factories and use overrides of the type system in
the meta class to apply `isinstance` and `issubtype` behavior.  We can further
restrict the type system if we need to by adding verifications that the
`__new__` and `__init__` methods must point the original base class
implementations if need.  Howver, we have not taken this step as of yet.
The split approach effectively removes these heavy elements from type creation.
The concequence of this is that means all of the rest of logic needs to be in
CPython implementation.  These can be rather cumbersome at times.

It is always a slippery slope when pushing code from Python back to CPython.
Some thing are needed as they are on the critical path while others are
called only occasionally and thus represent no cost to leave in Python.  On the
other hand some things are easy to implement in CPython because the have
direct access rather than having to go through a module entry point.  We have
gone with the approach that all critical path and all code the eliminates the
need for an entry point should be pushed back to C.


Java Slots
-------------

In order to get any reasonable speed with Python, the majority of the code
needs to be in C.  But additionally there needs to be the use of slots which are
hard coded locations to search for a particular piece of information.  This
presents a challenge when wrapping Java as we need a slot for the Java value
structure which must appear on Python object, long, float, exception, and type.
These different types each have their own memory layout and thus we can't just
add the slot at the base as once something is added to the base object it can
no longer be used in multiple inheritance.  Thus we require a different
approach.

Looking through the CPython source, we find they have the same quandary with
respect to `__dict__` and `__weakref__` slots.  Those slots do not appear one
the base object but get added along the way.  The method they use is to add
those slots to the back of the object be increasing the basesize of the object
and then referencing them with two offset slots in the type object.  If the type
is variable length the slot offset are negative thus referencing from the end of
the object, or positive if the object is a fixed layout.

Thus we tried a few formulations to see what would work best.


Broken tree approach
~~~~~~~~~~~~~~~~~~~~

The problem with just directly adding the slots in the tree is that the Java
inheritance tree forces the order of the Python tree we have to apply.  If we
add a slot to `java.lang.Object` we have to keep the slot on
`java.lang.Throwable` but that is not possible because Throwable requires it to
be derived from Python `Exception`. Thus if we are going to add a slot to the
base we would have to break the tree into pieces on the Python side.  This is
possible due to Python inheritance hacking with some effort.

But this approach had significant down sides. When we go to access the slot
we have to first figure out if the slot is present and if not then fall back to
looking in the dictionary.  But one of the most common cases is one in which the
item has no slot at all.  Thus if we have to both look for the slot and then hit
the dictionary, this is worse than just going to the dictionary in the first
place. Thus this defeats the point of a slot in many cases.


Python dict approach
~~~~~~~~~~~~~~~~~~~~~~~~~

We attempted the same trick by increasing the basesize to account for our extra
slot.  This leaves to difficulties.  First, the slot has no offset so we need to
find it each time by implying its location.  Second, the extra objects have to
be "invisible" during the type construction phase, or Python will conclude the
memory layout of the object is in conflict.  We can fool the type system by
subtracting the extra space from the type during the declaration phase and then
adding it back after the base types are created.  

This approach failed because the "invisible" part is checked each and every time
a new type is added to the system.  Thus every dynamic type we add checks the
base types for consistency and at some point the type system will find the
inconsistency and cause a failure.  Therefore, this system can never be robust.

Dict and weakref appear to be very special cases within the Python system and as
there is no general facility to replicate them working within the system does
not appear to be viable.


Memory hacking approach
~~~~~~~~~~~~~~~~~~~~~~~

The last system we attempted to mess with the memory layout of the object during
the creation phase to append our memory after Pythons.  To do this we need to
override the memory allocator to allocate the requested memory plus our extra.
We can then access this appended memory by computing the correct size of the
object and thus our slot is on the end.

We can test if the slot is present by looking to see if both `tp_alloc` and
`tp_finalize` point to our Java slot handlers.  This means we are still
effectively a slot as we can test and access with O(1).

The downside of this approach is there are certain cases in which the type of an
object can be changed during the destruction phase which means that our slot can
point to the wrong space if the basesize is changed out from under us.  To guard
against this we need to close our type system by imposing a ClassMeta which
closes off mixin types that do not inherit from one of the special case base
classes we have defined.  

The API implications should be small.  There was never a functional case where 
extending a Java object within Python actually made sense as the Python portion 
is just lost when passed to Java and unlike Proxies there is no way to retrieve
it.  Further the extending a Java object within Python does not bind the
lifespan of the objects so any code that used this is likely already buggy.  We
will properly support this option with `@JExtends` at a latter point.

With this limitiation in mind, this appears to be the best implementation
 - It adds the slot to all of the required types.
 - The slot is immediately accessable using just two fields (basesize, itemsize)
 - The slot can be tested for easily (check tp_alloc, tp_finalize)
 - It closes the type system by forcing a meta class that guards against
   inappropraite class constuction.

We could in principle add the slot to the "front" of the Python object but that
could cause additional issues as we would require also require overriding the
deallocation slot to disappear our memory from the free.  The Python GC module
has already reserved the memory in the front of the object so the back is 
the next best option.


Speed patch implications
------------------------

Other than improving the speed, the speed patch has a lot of below the hood
changes.  So long as the user was not accessing private members there is no API
change, but everything below that is gone.  All private symbols like
`__javaclass__` and `__javavalue__` as well as all exposed private members
vanish from the interface.  There is no longer a distinction between class
wrappers and `java.lang.Class` instances for purposes of the casting system.
The wrapper is an extension of a Python type and has the class methods and 
fields, and the instance is an extension of a Python object without these.
Both hold Java slots to the same object.  Therefore a lot of complexity of the
private portions is effectively removed from the user view.  Every path now has
the same resolution, check the Java slot first and if not assume it is Python.

Two private methods now appear on the wrapper (though I may be able to hide them
from the user view.)  These are the test entry points `_canConvertToJava` and 
`_convertToJava`.  Thus the speed patch should be transparent all user code that
does not access our private members.  That said some code like the Database
wrappers using JPype have roots in some code that did make access to the private
tables.  I have sent corrections when we upgraded to 0.7 series thus making them
conforming enough not to touch the private members.  But that does mean some
modules may be making such accesses out in the wild.

The good new is after the speed patch pretty much everything that is supposed to
be under the hood is now out of the reach of the user.  Thus the chance of
future API breakage is much lower.


Below the hood changes
======================
 - We have tried to prevent backend changes from reaching the API, though this
   is not always entirely the case.  The majority of the cases not already noted
   appear to be in the range of implementation side effects.  The old
   implementation had bugs that create undefined behaviors like reinterpet
   casting buffers and the like.  It is not possible to both fix the bugs in the
   backend and make preserve buggy behavours on the front end.  We have limited
   our changes to only those for which we can see no desirable use existing.
   Calling a list slice assignment on a float from a numpy array of ints and
   getting a pile of gibberish was as far as we can tell not useful to the user.
 - setResource is dropped in favor of caching the module dictionary once at
   start of the JVM.  We have a lot of resources we will need and the
   setResources method was cumbersome.
 - There is a lot of thrashing for the Python module style between C and C++
   style.  The determining blow was that C++ exception warning showed up when
   the proper linkage was given.  Thus the perferred style flipped from C++
   style to C.  Thus the naming style change accordingly.
 - In addition to the style change there is also an attempt to isolate symbols
   between the different classes.  The older style with a formal header that
   declares all the symbols at the top encouraged access to the functions and
   increased the complexity.  Moving to a C style and making everything static
   forces the classes to be much more independent.
 - With the change to C style there is natural split in CPython class files
   between the structure declaration, static methods that implement Python API
   functions, the declaration of the type, and the exposed C++ style API used by
   the rest of the module. 
 - There is some thrashing on how much of the C++ wrapper style Python API to
   keep.  The rewrapping of the API was mainly so support differences between
   Python 2.7 and 3.x.  So we dropped where we could.  Only the JPPyObject which
   acts as memory handling device over pure Python style (because Python style
   is not exception safe) is strongly needed.
 - There is some spacing thrashing between different editors and the continuing
   debate of why C was written to have the pointer stick to to the variable
   rather than the type.  When writing `Object* foo` it implies that the star
   is stuck to type rather than the variable where C reads it as the opposite.
   Hence there is the endless churn between what is correct `Object *foo` and
   what we would say in which `Object*` is actually a type.  As Python favors
   the former and we currently have the latter that means at some point we
   should just have formatter force consistency.
 - We introduced Py_IsInstanceSingle.  Is is a missing Python API function first
   for fast type check method of a single inherited type using the mro.  Then
   something is singlely inherited we can bypass the whole pile of redirects and
   list searchs and just compare if the mro list matches at the same point
   counted from the end.  As all of our base types are single inherited this
   saves time and dodges the more expensive calls that would trigger due to the
   PyJPClassMeta overrides.
 - We introduced Py_GetAttrDescriptor.  This was previously implemented in
   Python and was very slow.  Python has very good implementations for GetAttr
   but unfortunately it has the behavior that it always dereferences
   descriptors.  Thus it is useless in the case that we need to get the
   descriptor from the type tree.  We have reimplemented the Python type search
   without the descriptor dereferencing behavior.  It is not nearly as complete
   as the Python method as we only need to consult the dictionary and not handle
   every edge case.  That of course means that we are much faster.
 - As with every rewrite there is the need to cleanup.  Anything that wasn't
   reached by the test bench or was identified as being called only from one
   location was removed or reencorperated back in into the function that call
   it.

