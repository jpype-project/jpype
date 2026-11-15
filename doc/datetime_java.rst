
Python Dates and Times (for Java)
=====================================

The ``python.datetime`` front end for Python's ``datetime`` module -- there
is no matching Python-side chapter for this one; it's cross-linked from
:doc:`types_java` rather than paired with a ``_py`` counterpart. Every type
named below is real, shipping code in the ``python.datetime`` package.

.. contents::
   :local:
   :depth: 1


Getting started
------------------

``DateTime`` is the entry point, following the same
``<Module>.using(context)`` convention as :doc:`collections_java`:

.. code-block:: java

    import python.datetime.DateTime;
    import python.datetime.PyDate;
    import python.datetime.PyDateTime;

    DateTime datetime = DateTime.using(context);
    PyDate d = datetime.date(2024, 1, 2);
    PyDateTime dt = datetime.now();

``datetime.datetime`` is itself a subclass of ``datetime.date`` in Python,
so ``PyDateTime`` extends ``PyDate`` here too, inheriting its calendar
accessors. ``datetime.timedelta`` represents a fixed span rather than a
calendar point, so ``PyTimeDelta`` stands on its own.

Every type also offers a promotion default method to the corresponding
``java.time`` type -- ``PyDate#toLocalDate()``,
``PyDateTime#toLocalDateTime()``, ``PyDateTime#toInstant()`` (aware
instances only), and ``PyTimeDelta#toDuration()`` -- computed entirely on
the Java side from already-fetched accessor values, with no further round
trip into Python.


Naive vs. aware datetimes
----------------------------

Like Python itself, a ``PyDateTime`` may be *naive* (no timezone
information) or *aware* (carries a UTC offset). Check with ``isAware()``;
``utcOffsetSeconds()`` is ``null`` for naive instances.

.. code-block:: java

    PyDateTime naive = datetime.dateTime(2024, 1, 2, 3, 4, 5, 6789);
    boolean isAware = naive.isAware();                    // false
    Integer offset = naive.utcOffsetSeconds();             // null

    PyDateTime aware = datetime.dateTimeFromEpochSeconds(1704164645.006789);
    // aware.isAware() is true; aware.toInstant() is safe to call


The java.time naming-collision hazard
------------------------------------------

Each factory method that accepts a ``java.time`` convenience type (e.g.
``dateFromLocalDate(LocalDate)``) is deliberately given a distinct name
from its primitive-argument counterpart (``date(int, int, int)``), rather
than an overload of it. JPype's proxy dispatch for ``WrapperService``-backed
interfaces routes purely by method name, not by Java overload signature -- a
``default`` method sharing a registered method's name is intercepted and
routed to that method's underlying Python callable with whatever arguments
were actually passed, without ever running the ``default`` method's own
body. Overloading ``date(int, int, int)``/``date(LocalDate)`` under one name
would silently forward a raw ``LocalDate`` object to a Python callable that
expects three integers instead. See :doc:`spi` for the mechanism.

.. code-block:: java

    import java.time.LocalDate;
    import java.time.Instant;
    import java.time.Duration;

    PyDate fromLocal = datetime.dateFromLocalDate(LocalDate.of(2024, 1, 2));
    PyDateTime fromInstant = datetime.dateTimeFromInstant(Instant.now());
    PyTimeDelta fromDuration = datetime.timeDeltaFromDuration(Duration.ofHours(3));


Where to next
---------------

- :doc:`collections_java` -- the sibling standard-library front end for
  containers.
- :doc:`types_java` -- the core ``python.lang`` type hierarchy these types
  build on.
- :doc:`limitations_java` -- what's not typed yet.
