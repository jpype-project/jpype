Java Generics Support
=====================

JPype provides experimental support for Java generic types through the ``.typed()`` method.

Basic Usage
-----------

To create a generic type, use the ``.typed()`` method on a Java class with one or more type parameters:

.. code-block:: python

    import jpype
    jpype.startJVM()
    
    ArrayList = jpype.JClass('java.util.ArrayList')
    String = jpype.JClass('java.lang.String')
    
    # Create a generic ArrayList<String> type
    StringList = ArrayList.typed(String)
    
    print(StringList.__name__)  # java.util.ArrayList<java.lang.String>

Multiple Type Parameters
-------------------------

For classes with multiple type parameters (like Map), pass multiple arguments:

.. code-block:: python

    HashMap = jpype.JClass('java.util.HashMap')
    Integer = jpype.JClass('java.lang.Integer')
    
    # Create HashMap<String, Integer>
    StringIntMap = HashMap.typed(String, Integer)

Type Caching
------------

Generic types are cached, so calling ``.typed()`` with the same parameters returns the same type object:

.. code-block:: python

    StringList1 = ArrayList.typed(String)
    StringList2 = ArrayList.typed(String)
    
    assert StringList1 is StringList2  # Same cached type

Inheritance
-----------

Generic types inherit from their base class:

.. code-block:: python

    StringList = ArrayList.typed(String)
    
    assert issubclass(StringList, ArrayList)  # True

Type Information
----------------

Generic types store their type parameters in the ``_generics`` attribute:

.. code-block:: python

    StringList = ArrayList.typed(String)
    
    print(StringList._generics)  # (String,)

Limitations
-----------

**Current Status**: This is an experimental feature with the following limitations:

1. **Static Method Conflicts**: Generic types currently don't handle static methods correctly. This will be addressed when the main branch catches up with static method improvements.

2. **Runtime Type Checking**: The generic type information is purely for documentation and type hints. Java's type erasure means runtime type checking is not enforced.

3. **No Type Validation**: The ``.typed()`` method accepts any Java class, even if it doesn't make sense for the generic type.

Array Creation
--------------

Array creation using ``[]`` subscript notation continues to work as before and does not conflict with generics:

.. code-block:: python

    # Array creation (still works)
    arr = String[10]  # Creates String array of size 10
    
    # Generic type creation (new feature)
    StringList = ArrayList.typed(String)  # Creates ArrayList<String> type

Future Improvements
-------------------

- Fix static method handling in generic types
- Add runtime type validation based on type parameters
- Support for wildcard types (``? extends T``, ``? super T``)
- Integration with Python type hints (``ArrayList[String]`` syntax)
