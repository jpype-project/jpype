
Python Collections (for Java)
================================

This is the Java-side counterpart to :doc:`collections_py`: the
``python.collections`` front end for Python's ``collections`` module. Every
type named below is real, shipping code in the ``python.collections``
package.

.. contents::
   :local:
   :depth: 1


Getting started
------------------

``PyCollections`` (not ``Collections``, to avoid colliding with
``java.util.Collections``) is the entry point, following the same
``<Module>.using(context)`` convention as :doc:`types_java` and
:doc:`quickguide_java`'s standard-library section:

.. code-block:: java

    import python.collections.PyCollections;
    import python.collections.PyDeque;

    PyCollections collections = PyCollections.using(context);
    PyDeque d = collections.deque();

Each factory method returns a normal Java interface backed by the real
Python object, so its methods behave exactly as they would in Python.


The PyDict vs PyMapping type-shape distinction
--------------------------------------------------

``PyOrderedDict``, ``PyDefaultDict``, and ``PyCounter`` are all real
``dict`` subclasses in Python, so they extend ``python.lang.PyDict`` here
too and reuse its entire ``Map`` surface -- only each type's Python-specific
extras are added on top. ``PyChainMap`` is a ``collections.abc.MutableMapping``
but *not* a real ``dict`` subclass, so it extends the weaker
``python.lang.PyMapping`` instead: same reused ``Map`` surface, but an
accurate claim about its underlying Python type. ``PyDeque`` is not a
``list`` subclass and does not support slicing, so it stands on its own -- a
Python-flavored, named subset of ``java.util.Deque``'s shape.


PyDeque
---------

.. code-block:: java

    PyDeque d = collections.deque(
            context.listFromObjects(context.$int(1), context.$int(2)), 2);  // maxlen=2

    d.append(context.$int(3));               // maxlen reached: discards from the left

    int size = d.size();                       // 2
    String popped = d.popleft().toString();     // "2"


PyCounter
-----------

A real ``dict`` subclass, so it's also a ``java.util.Map``.

.. code-block:: java

    PyCounter c = collections.counter(
            context.listFromObjects(context.str("a"), context.str("a"), context.str("b")));

    String countOfA = c.get(context.str("a")).toString();   // "2"
    int total = c.total();                                    // 3


PyOrderedDict / PyDefaultDict
--------------------------------

Both real ``dict`` subclasses. ``PyDefaultDict`` fills in missing keys by
calling its factory.

.. code-block:: java

    PyOrderedDict od = collections.orderedDict();
    od.put(context.str("a"), context.$int(1));

    PyDefaultDict dd = collections.defaultDict(context.eval("int"));
    dd.put(context.str("a"), context.$int(5));
    PyObject missing = dd.get(context.str("missing"));         // null: get() doesn't
                                                                  // trigger the factory


PyChainMap
------------

A ``MutableMapping``, not a ``dict`` subclass -- extends ``PyMapping``
rather than ``PyDict``. Writes and deletes always land on the first map in
the chain.

.. code-block:: java

    PyDict first = context.dict();
    first.put(context.str("a"), context.$int(1));
    PyDict second = context.dict();
    second.put(context.str("b"), context.$int(2));

    PyChainMap cm = collections.chainMap(java.util.Arrays.asList(first, second));

    cm.put(context.str("c"), context.$int(3));
    boolean onFirst = first.containsKey(context.str("c"));      // true
    boolean onSecond = second.containsKey(context.str("c"));    // false


Where to next
---------------

- :doc:`datetime_java` -- ``python.datetime``, the sibling standard-library
  front end for dates and times.
- :doc:`limitations_java` -- what's not typed yet.
