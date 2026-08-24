##################
JPype DBAPI2 Guide
##################

`Introduction`
==============

One common use of JPype is to provide access to databases using JDBC.  The JDBC
API is well established, very capable, and supports most databases.
JPype can be used to access JDBC both directly or through the use of the Python
DBAPI2 as specified in PEP-0249_.  Unfortunately, the Python API leaves a lot of
behaviors undefined.

The JPype dbapi2 module provides our implementation of this Python API.
Normally the Python API has to deal with two different type systems, Python
and SQL.  When using JDBC, we have the added complexity that Java types are
used to communicate with the driver.  We have introduced concepts appropriate
to handle this additional complexity.


`Module Interface`
==================

`Constructors`
--------------

Access to the database is made available through connection
objects. The module provides the following constructor for connections:

.. _connect:

.. autofunction:: jpype.dbapi2.connect

Globals
-------

JPype dbapi2 defines several globals that define the module behavior.
These values are constants.

.. _apilevel:

`apilevel`_
    The apilevel for the module is "``2.0``".  


.. _threadsafety:

`threadsafety`_
    The threadsafety level is 2 meaning "Threads may share the module and
    connections".  But the actual threading level depends on the driver
    implementation that JDBC is connected to.  Connections for many databases
    are synchronized so they can be shared, but threads must execute statements
    in series.  Connections in the module are implemented in Python and 
    have per object resources that cannot be shared.  Attempting to use a
    connection with a thread other than the thread that created it will
    raise an ``Error``.

    Sharing in the above context means that two threads may use a resource
    without wrapping it using a mutex semaphore to implement resource locking.
    Note that you cannot always make external resources thread safe by managing
    access using a mutex: the resource may rely on global variables or other
    external sources that are beyond your control.


.. _paramstyle:

`paramstyle`_
    The parameter style for JPype dbapi2 module is ``qmark``

    ============ ==============================================================
    paramstyle   Meaning
    ============ ==============================================================
    ``qmark``    Question mark style, e.g. ``...WHERE name=?``
    ============ ==============================================================


.. _isolation level constants:

`Isolation level constants`_
    (extension) Values for `connection.isolation_level <connection.isolation_level_>`_,
    matching ``java.sql.Connection.TRANSACTION_*``.  Not every level is
    supported by every database; setting an unsupported level raises
    ``NotSupportedError``.

    - ``TRANSACTION_NONE`` (0)
    - ``TRANSACTION_READ_UNCOMMITTED`` (1)
    - ``TRANSACTION_READ_COMMITTED`` (2)
    - ``TRANSACTION_REPEATABLE_READ`` (4)
    - ``TRANSACTION_SERIALIZABLE`` (8)


Exceptions
----------

The dbapi2 module exposes error information using the following
exceptions:

.. autoclass:: jpype.dbapi2.Warning
.. autoclass:: jpype.dbapi2.Error
.. autoclass:: jpype.dbapi2.InterfaceError
.. autoclass:: jpype.dbapi2.DatabaseError
.. autoclass:: jpype.dbapi2.DataError
.. autoclass:: jpype.dbapi2.OperationalError
.. autoclass:: jpype.dbapi2.IntegrityError
.. autoclass:: jpype.dbapi2.InternalError
.. autoclass:: jpype.dbapi2.ProgrammingError
.. autoclass:: jpype.dbapi2.NotSupportedError

Python exceptions are more fine grain than JDBC exceptions.  Wherever possible
we have redirected the Java exception to the nearest Python exception.  However,
there are cases in which the Java exception may appear.  Those exceptions
inherit from :py:class:`jpype.dbapi2.Error`.  This is the exception inheritance layout::

    Exception
    |__Warning
    |__Error
       |__InterfaceError
       |__java.sql.SQLError
       |  |__java.sql.BatchUpdateException
       |  |__java.sql.RowSetWarning
       |  |__java.sql.SerialException
       |  |__java.sql.SQLClientInfoException
       |  |__java.sql.SQLNonTransientException
       |  |__java.sql.SQLRecoverableException
       |  |__java.sql.SQLTransientException
       |  |__java.sql.SQLWarning
       |  |__java.sql.SyncFactoryException
       |  |__java.sql.SyncProviderException
       |
       |__DatabaseError
          |__DataError
          |__OperationalError
          |__IntegrityError
          |__InternalError
          |__ProgrammingError
          |__NotSupportedError



Type Access
===========

JPype dbapi2 provides two different maps which serve to convert data
between Python and SQL types.  When setting parameters and fetching 
results, Java types are used.  The connection provides two maps for converting
the types of parameters.  An `adapter <adapters_>`_ is used to translate from a Python
type into a Java type when setting a parameter.  Once a result is produced,
a `converter <converters_>`_ can be used to translate the Java type back into a Python type.

There are two lookup functions that select the behavior to decide how a column or
parameter should be treated.  These are `getters`_ and `setters`_.

.. _adapters:

adapters_
---------

Whenever a Python type is passed to a statement, it must first be converted
to the appropriate Java type.  This can be accomplished in a few ways.  The
user can manually convert to the correct type by constructing a Java object or
applying the JPype casting operator.  Some Java types have built-in implicit
conversions from the corresponding type.  For all other conversions, an
adapter must be supplied.

An adapter is defined as a type to convert from and a conversion function which
takes a single argument that returns a Java object.  For example, a
``decimal.Decimal`` has no JDBC setter of its own, so it is adapted into a
``java.math.BigDecimal`` by a default adapter before a setter is selected.

The adapter maps are stored in the connection.  The adapter map can be
supplied when calling `connect`_, or added to the map later
through the `adapters <connection.adapters_>`_ property.


.. _setters:

setters_
--------

A setter transfers the Java type into a SQL parameter.  There are multiple
types that an individual parameter may accept.  The type of setter is determined
by the JDBC type.  Each individual JDBC type can have its own setter.  Not
every database supports the same setter.  There is a default setter that
should work for most purposes.  Setters can also be set individually using
the ``types`` argument to the ``.execute*()`` methods.  The setter is a
configurable function that uses the database metadata to select an
appropriate type.

Setters can be supplied as a map to `connect`_ or by accessing
the `setter <connection.setters_>`_ property on a Connection.

.. autofunction:: jpype.dbapi2.SETTERS_BY_META
.. autofunction:: jpype.dbapi2.SETTERS_BY_TYPE

.. _converters:

converters_
-----------

When a result is fetched from the database it is returned as a Java type.  This Java
type then has a converter applied.  Converters are stored in a map holding the 
type as key and a converter function that takes one argument and returns the desired type.
The default converter map will convert all types to Python.  This can be 
disabled by setting the converters to ``None``.

The converter map can be passed in to the `connect`_ function, or set on the
Connection using the `converters <connection.converters_>`_ property.  It
can be supplied as a list or a map to the ``.fetch*()`` methods.

.. _getters:

getters_
--------

JDBC provides more than one way to access data returned from a result.
In the native JDBC, each executed statement returns a result set which 
acts as a cursor for the statement.  It is possible to access each 
column using a different get method.  The default map will attempt
to fetch according to the most general type.  The getter is a configurable
function that uses the metadata to find the most appropriate type.

.. autofunction:: jpype.dbapi2.GETTERS_BY_TYPE
.. autofunction:: jpype.dbapi2.GETTERS_BY_NAME

.. _Connection:

`Connection Objects`_
=====================

A Connection object can be created by using the `connect`_ function.  Once a
connection is established the resulting Connection contains the following.

.. autoclass:: jpype.dbapi2.Connection
  :members:

.. _Cursor:

`Cursor Objects`_
=================

These objects represent a database cursor, which is used to manage the
context of a fetch operation. Cursors created from the same connection
are not isolated, *i.e.*, any changes done to the database by a cursor
are immediately visible by the other cursors.  Cursors created from
different connections may or may not be isolated, depending on how the
transaction support is implemented (see also the connection's
`rollback <connection.rollback_>`_ and `commit <connection.commit_>`_ methods).

.. autoclass:: jpype.dbapi2.Cursor
  :members:


Cursors can act as an iterator.  So to get the contents of a table one
could use code like:

.. code-block:: python

   with connection.cursor() as cur:
       cur.execute("select * from table")
       for row in cur:
          print(row)

`SQL Type Constructors`
=======================

Many databases need to have the input in a particular format for
binding to an operation's input parameters.  For example, if an input
is destined for a ``DATE`` column, then it must be bound to the
database in a particular string format.  Similar problems exist for
"Row ID" columns or large binary items (e.g. blobs or ``RAW``
columns).  This presents problems for Python since the parameters to
the ``.execute*()`` method are untyped.  When the database module sees
a Python string object, it doesn't know if it should be bound as a
simple ``CHAR`` column, as a raw ``BINARY`` item, or as a ``DATE``.

This is less of a problem in JPype dbapi2 than in a typical 
dbapi driver as we have strong typing backing the connection,
but we are still required to supply methods to construct individual
SQL types.  These functions are:

.. autofunction::  jpype.dbapi2.Date
.. autofunction::  jpype.dbapi2.Time
.. autofunction::  jpype.dbapi2.Timestamp
.. autofunction::  jpype.dbapi2.DateFromTicks
.. autofunction::  jpype.dbapi2.TimeFromTicks
.. autofunction::  jpype.dbapi2.TimestampFromTicks
.. autofunction::  jpype.dbapi2.Binary

For the most part these constructors are largely redundant because 
adapters can provide the same functionality and Java types
can be used directly to communicate type information.

.. `JDBC Types`

`JDBC Types`_
=============

In the Python DBAPI2, the SQL type system is normally reduced to a subset
of the SQL types by mapping multiple types together. For example, ``STRING``
covers types ``STRING``, ``CHAR``, ``NCHAR``, ``NVARCHAR``, ``VARCHAR``,
and ``OTHER``.  JPype dbapi2 supports both the recommended Python types and
the fine grain JDBC types.  Each type is represented by an object
of type ``JDBCType``.

.. autoclass:: jpype.dbapi2.JDBCType
   :members:

The following types are defined with the corresponding Python grouping, the
default setter, getter, and Python type.  For types that support more than
one kind of getter, the special getter can be requested explicitly by
passing it in the ``types`` argument to a ``.fetch*()`` method (not every
JDBC driver implements every getter, so this is driver-dependent).  For
example, the default configuration maps ``BLOB`` to the same getter as
``BINARY`` (returning ``bytes``); on a driver that implements
``getBlob()``, calling ``cur.fetchone(types=[BLOB])`` instead returns the
column as a Java ``Blob`` object.

======== ======================== =================== ============== ================= ===============
Group    JDBC Type                Default Getter      Default Setter PyTypes           Special Getter
======== ======================== =================== ============== ================= ===============
DATE     DATE                     getDate             setDate        datetime.date
DATETIME TIMESTAMP                getTimestamp        setTimestamp   datetime.datetime
TIME     TIME                     getTime             setTime        datetime.time
-------- ------------------------ ------------------- -------------- ----------------- ---------------
DECIMAL  DECIMAL                  getBigDecimal       setBigDecimal  decimal.Decimal
DECIMAL  NUMERIC                  getBigDecimal       setBigDecimal  decimal.Decimal
-------- ------------------------ ------------------- -------------- ----------------- ---------------
FLOAT    FLOAT                    getDouble           setDouble      float
FLOAT    DOUBLE                   getDouble           setDouble      float
FLOAT    REAL                     getFloat            setFloat       float
-------- ------------------------ ------------------- -------------- ----------------- ---------------
NUMBER   BOOLEAN                  getBoolean          setBoolean     bool
NUMBER   BIT                      getBoolean          setBoolean     bool
NUMBER   TINYINT  (0..255)        getShort            setShort       int
NUMBER   SMALLINT (-2^15..2^15)   getShort            setShort       int
NUMBER   INTEGER  (-2^31..2^31)   getInt              setInt         int
NUMBER   BIGINT   (-2^63..2^63)   getLong             setLong        int
-------- ------------------------ ------------------- -------------- ----------------- ---------------
BINARY   BINARY                   getBytes            setBytes       bytes
BINARY   BLOB                     getBytes            setBytes       bytes             getBlob
BINARY   LONGVARBINARY            getBytes            setBytes       bytes
BINARY   VARBINARY                getBytes            setBytes       bytes
-------- ------------------------ ------------------- -------------- ----------------- ---------------
TEXT     CLOB                     getString           setString      str               getClob
TEXT     LONGNVARCHAR             getString           setString      str
TEXT     LONGVARCHAR              getString           setString      str
TEXT     NCLOB                    getString           setString      str               getNClob
TEXT     SQLXML                   getString           setString      str               getSQLXML
-------- ------------------------ ------------------- -------------- ----------------- ---------------
STRING   NVARCHAR                 getString           setString      str
STRING   CHAR                     getString           setString      str
STRING   NCHAR                    getString           setString      str
STRING   VARCHAR                  getString           setString      str
-------- ------------------------ ------------------- -------------- ----------------- ---------------
--       ARRAY                    getObject           --             --                getArray
--       OBJECT                   getObject           --             --                getObject
--       NULL                     getObject           --             --                getObject
--       REF                      getObject           --             --                getRef
--       ROWID                    getObject           --             --                getRowId
--       RESULTSET                getObject           --             --                getObject
--       TIME_WITH_TIMEZONE       getObject           --             --                getTime
--       TIMESTAMP_WITH_TIMEZONE  getObject           --             --                getTimestamp
-------- ------------------------ ------------------- -------------- ----------------- ---------------
*        ASCII_STREAM             getAsciiStream      --             --                --
*        BINARY_STREAM            getBinaryStream     --             --                --
*        CHARACTER_STREAM         getCharacterStream  --             --                --
*        NCHARACTER_STREAM        getNCharacterStream --             --                --
*        URL                      getURL              --             --                --
======== ======================== =================== ============== ================= ===============

Some of these types never correspond to a SQL type but are used only to specify
getters and setters for a particular parameter or column.

``TIME_WITH_TIMEZONE``/``TIMESTAMP_WITH_TIMEZONE`` conversion
---------------------------------------------------------------

``getObject()`` on a ``TIME``/``TIMESTAMP WITH TIME ZONE`` column returns
whichever Java type the driver uses to represent it, which varies by
vendor.  Two of those representations convert to a timezone-aware
``datetime.time``/``datetime.datetime`` automatically, by default:

- ``java.time.OffsetTime`` and ``java.time.OffsetDateTime`` -- the
  JDK-standard (JDBC 4.2) representations, which modern drivers
  increasingly return.  HSQLDB returns ``OffsetDateTime`` for
  ``TIMESTAMP WITH TIME ZONE``; both H2 and HSQLDB return ``OffsetTime``
  for ``TIME WITH TIME ZONE``.

A driver that instead returns its own vendor-specific class (H2's
``TIMESTAMP WITH TIME ZONE`` returns ``org.h2.api.TimestampWithTimeZone``,
not ``OffsetDateTime``) is not covered by a built-in converter -- adding
one for every vendor's private class isn't something the module should
carry -- and the raw Java object is returned instead.  Register a
converter for that specific class the same way as any other type mapping:

.. code-block:: python

   import jpype

   H2TimestampTZ = jpype.JClass("org.h2.api.TimestampWithTimeZone")

   def to_py(v):
       offset = datetime.timezone(datetime.timedelta(seconds=v.getTimeZoneOffsetSeconds()))
       midnight = datetime.datetime(v.getYear(), v.getMonth(), v.getDay())
       micros = int(v.getNanosSinceMidnight()) // 1000
       return (midnight + datetime.timedelta(microseconds=micros)).replace(tzinfo=offset)

   cx.converters[H2TimestampTZ] = to_py

Timestamp precision is driver-dependent
------------------------------------------

Python's ``datetime`` and Java's ``java.sql.Timestamp`` both support
sub-millisecond (microsecond/nanosecond) precision, but not every JDBC
driver actually preserves it.  sqlite-jdbc, for example, only round-trips
``TIMESTAMP`` values to millisecond resolution -- a value inserted with
microsecond precision comes back truncated to the nearest millisecond.
This is a property of the driver's storage representation, not something
``dbapi2`` does or can correct: the same value round-trips with full
microsecond precision through H2 and HSQLDB, and calling the driver's raw
``setTimestamp()``/``getTimestamp()`` directly (bypassing ``dbapi2``
entirely) shows the identical truncation on sqlite.  If timestamp
precision looks wrong, check whether it's driver-specific before assuming
it's a ``dbapi2`` bug.

Re-using a fetched Array/Blob/Clob as a parameter
----------------------------------------------------

A value fetched from an ``ARRAY``/``BLOB``/``CLOB``/``NCLOB``/``SQLXML``/
``REF``/``ROWID`` column comes back as whatever vendor-specific concrete
class the driver uses to implement the corresponding ``java.sql``
interface (e.g. H2 returns ``org.h2.jdbc.JdbcBlob`` for a fetched
``BLOB``), never the bare interface itself.  ``SETTERS_BY_TYPE`` matches
against that interface with ``issubclass()`` when the value's exact class
isn't found directly, so passing such a value back in as a parameter --
copying a LOB from one table into another, for example -- works the same
way any other parameter does:

.. code-block:: python

   f = cur.execute("select v from source").fetchone(types=[dbapi2.BLOB])
   cur.execute("insert into dest(v) values(?)", [f[0]])

Inserting a plain Python ``list``/array as an ``ARRAY`` column needs one
extra step: JDBC has no implicit conversion from a native array to
``java.sql.Array``, so build one with ``Connection.createArrayOf()``
first (available as ``cx.connection.createArrayOf(sql_type_name, values)``):

.. code-block:: python

   jarr = cx.connection.createArrayOf("INTEGER", [1, 2, 3])
   cur.execute("insert into t values (?)", [jarr])

Other
-----

The default getter will attempt to look for the column type by name if the type is OTHER.
This allows for user defined types to be added if supported by the database.

User defined types
------------------

A user can declare a new type using ``JDBCType``.  The arguments are the name of 
new type which must match a SQL typename.  Use ``typeinfo`` on the connection to 
get the list of available types.

It may be necessary to define a custom getter function when defining a new type
so that the custom return type accurately reflects the column type.

.. code-block:: python

   class JSONType(dbapi2.JDBCType):
      def get(self, *args):
          rc = dbapi2.JDBCType.get(self, *args)
          # Custom return converter here
          return rc
   JSON = JSONType("JSON")


Interactions with prepared statements
-------------------------------------

Certain calls can be problematic for dbapi2 depending on the driver.  In
particular, SQL calls which invalidate the state of the connection will issue
an exception when the connection is used.  For example, when using HSQLDB, the
statement ``cur.execute('shutdown')`` will invalidate and close the connection,
causing an exception to be raised.

This exception is due to a conflict between dbapi2, Java, and HSQLDB
specifications.  Dbapi2 requires that statements be executed as prepared
statements, Java requires that closing a statement yields no action if the
connection is already closed, and HSQLDB sets the ``isValid`` to false but not
``isClosed``.  Thus executing a shutdown through dbapi2 would be expected to
close the prepared statement on an invalid connection resulting in an error.

We can address these sorts of driver specific behaviors by applying a customizer
to the Java class to add additional behaviors.

.. code-block:: python

        @jpype.JImplementationFor("java.sql.PreparedStatement")
        class MyStatement(object):
            @jpype.JOverride(sticky=True, rename='_close')
            def close(self):
                if not self.getConnection().isValid(100):
                     return
                return self._close()

Alternatively we can access the ``java.sql.Connection`` directly and call the
shutdown routine using an unprepared statement.  Though that would require
accessing private fields.


`Working with pandas`
======================

Since ``jpype.dbapi2`` conforms to PEP 249, results can be loaded directly
into a `pandas <https://pandas.pydata.org/>`_ ``DataFrame`` the same way as
any other DB-API 2.0 module.  Two patterns work:

Manually, from a cursor
------------------------

Build the ``DataFrame`` from ``.fetchall()`` and the column names in
``.description``.  This always works and gives full control over
``dbapi2``-specific features such as ``types=`` and custom converters.

.. code-block:: python

   import jpype.dbapi2 as dbapi2
   import pandas as pd

   with dbapi2.connect("jdbc:sqlite::memory:") as cx, cx.cursor() as cur:
       cur.execute("create table t (id integer, name varchar(20))")
       cur.executemany("insert into t values (?, ?)", [(1, "a"), (2, "b")])
       cur.execute("select * from t")
       columns = [d[0] for d in cur.description]
       df = pd.DataFrame(cur.fetchall(), columns=columns)

Using ``pandas.read_sql``
--------------------------

``pandas.read_sql`` (and ``read_sql_query``) accept any PEP 249 ``Connection``
object directly -- a ``jpype.dbapi2.Connection`` does not need to be wrapped
in a SQLAlchemy engine first.  Parameters are passed through using the
module's ``qmark`` style (``?``):

.. code-block:: python

   import jpype.dbapi2 as dbapi2
   import pandas as pd

   with dbapi2.connect("jdbc:sqlite::memory:") as cx, cx.cursor() as cur:
       cur.execute("create table t (id integer, name varchar(20))")
       cur.executemany("insert into t values (?, ?)", [(1, "a"), (2, "b"), (3, "c")])
       df = pd.read_sql("select * from t where id > ?", cx, params=(1,))

pandas emits ``UserWarning: pandas only supports SQLAlchemy connectable
(engine/connection) or database string URI or sqlite3 DBAPI2 connection.
Other DBAPI2 objects are not tested.`` for this -- that is pandas being
conservative about which connection types its own test suite covers, not an
error; the call still executes correctly.  If the warning is undesirable,
use the manual pattern above instead.  (Wrapping the connection with
``sqlalchemy.create_engine("sqlite:///...", creator=lambda: cx)`` looks like
an obvious fix, but does not reliably work -- see `Working with SQLAlchemy
against other databases`_ below for why.)

Writing a DataFrame back to the database follows the normal ``executemany``
pattern -- there is no dbapi2-specific ``to_sql`` support, so convert the
DataFrame to row tuples first:

.. code-block:: python

   cur.executemany(
       "insert into t values (?, ?)",
       list(df.itertuples(index=False, name=None)),
   )


`Working with Polars`
======================

`Polars <https://pola.rs/>`_'s ``read_database`` also accepts a PEP 249
connection directly, with no warning and no SQLAlchemy engine required:

.. code-block:: python

   import jpype.dbapi2 as dbapi2
   import polars as pl

   with dbapi2.connect("jdbc:sqlite::memory:") as cx, cx.cursor() as cur:
       cur.execute("create table t (id integer, name varchar(20))")
       cur.executemany("insert into t values (?, ?)", [(1, "a"), (2, "b")])
       df = pl.read_database(query="select * from t", connection=cx)

There is no zero-copy Arrow path here: JDBC has no native Arrow transport,
so Polars builds its (Arrow-backed) DataFrame from the rows ``dbapi2``
fetches the ordinary DB-API way, the same as it would for any other DB-API
2.0 module. If you need genuinely zero-copy transport from the database,
that has to come from the driver/database speaking Arrow Flight SQL or
similar directly -- it is not something a JDBC bridge can add.


`Connection pooling with DBUtils`
==================================

`DBUtils <https://webwareforpython.github.io/DBUtils/>`_'s ``PooledDB``
works with ``jpype.dbapi2`` out of the box -- the module already exposes
the ``threadsafety`` attribute ``PooledDB`` checks for, so no wrapper is
needed:

.. code-block:: python

   import jpype.dbapi2 as dbapi2
   from dbutils.pooled_db import PooledDB

   pool = PooledDB(creator=dbapi2, maxconnections=5, dsn="jdbc:sqlite::memory:")
   cx = pool.connection()
   cur = cx.cursor()
   cur.execute("select 1")
   cur.fetchall()
   cx.close()  # returns the connection to the pool; does not close the JDBC connection

Repeatedly acquiring and releasing a pooled connection this way reuses the
same underlying JDBC connection rather than opening a new one each time.


`Concurrency`
=============

Recall from `threadsafety`_ above: the *module* can be used freely from
multiple threads, but an individual ``Connection`` (and its cursors) is
bound to the thread that created it.  The natural pattern for a
multi-threaded workload is therefore one connection per worker, not one
shared connection:

.. code-block:: python

   import concurrent.futures
   import jpype.dbapi2 as dbapi2

   def worker(i):
       with dbapi2.connect("jdbc:sqlite::memory:") as cx, cx.cursor() as cur:
           cur.execute("select ?", (i,))
           return cur.fetchone()

   with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
       results = list(pool.map(worker, range(50)))

A ``ThreadPoolExecutor`` reuses its worker threads across many tasks, so a
workload like this also exercises JPype's JNI thread attach/detach
repeatedly on the same OS threads rather than once each -- each worker
opening and closing its own connection, as shown above, is well within
what the module supports.


`Working with Apache Drill`
============================

Searches for "Python Apache Drill" turn up jaydebeapi or Drill's REST API
almost exclusively.  Neither is necessary: ``jpype.dbapi2`` connects to Drill
directly over JDBC, and `sqlalchemy-drill
<https://github.com/JohnOmernik/sqlalchemy-drill>`_ already ships a
``drill+jdbc://`` dialect that is built on ``jpype.dbapi2`` (not jaydebeapi)
-- it does ``from jpype import dbapi2`` and returns that module as its DBAPI.
This section is a runnable recipe for both.

Direct connection
------------------

Drill's JDBC driver is distributed as an "uber jar",
``drill-jdbc-all-<version>.jar`` (on Maven Central under
``org.apache.drill.exec:drill-jdbc-all``), which is enough to connect to a
**remote** Drillbit (``jdbc:drill:drillbit=host:port``).  It is *not* enough
on its own to run **embedded** mode (``jdbc:drill:zk=local``, which starts a
Drillbit inside your own process) -- Drill's driver rejects that outright::

    java.sql.SQLNonTransientConnectionException: Running Drill in embedded
    mode using Drill's jdbc-all JDBC driver Jar file alone is not supported.

For embedded mode you need the full Drill distribution (the tarball from
https://archive.apache.org/dist/drill/, extracted locally) and its complete
runtime classpath, not just the JDBC jar.  Once you have that, connecting is
the same ``jpype.dbapi2.connect()`` pattern used throughout this guide:

.. code-block:: python

   import jpype
   import jpype.dbapi2 as dbapi2

   DRILL_HOME = "/path/to/apache-drill-1.20.0"  # extracted distribution

   jpype.startJVM(classpath=[
       DRILL_HOME + "/conf",
       DRILL_HOME + "/jars/*",
       DRILL_HOME + "/jars/ext/*",
       DRILL_HOME + "/jars/3rdparty/*",
       DRILL_HOME + "/jars/classb/*",
   ])

   cx = dbapi2.connect("jdbc:drill:zk=local")
   with cx.cursor() as cur:
       cur.execute("SELECT * FROM cp.`employee.json` LIMIT 5")
       for row in cur.fetchall():
           print(row)
   cx.close()

Run against a real Drill cluster, only the connection string changes --
``jdbc:drill:drillbit=<host>:<port>`` (default port ``31010``) instead of
``jdbc:drill:zk=local`` -- and the jdbc-all jar alone is sufficient as the
classpath, since a remote Drillbit does the execution instead of your own
process.

**On the classpath ordering**: set the full classpath in the
``jpype.startJVM(classpath=...)`` call *before* starting the JVM.
``jpype.addClassPath()`` after the JVM has started does make new classes
available to explicit ``jpype.JClass()`` lookups, but it does not
retroactively help code that relies on ``java.util.ServiceLoader``-based
auto-discovery -- and both JDBC's own ``DriverManager`` (used internally by
``connect()``) and Drill's Hadoop-derived filesystem layer use exactly that
mechanism, and each only scans the classpath once, the first time it is
used.  In practice this means: if a driver jar is missing when
``DriverManager`` first initializes, adding it afterward will not make
``connect()`` find it, even though the class itself is now loadable.  Always
put the full classpath in ``startJVM()`` up front.

Working with SQLAlchemy
------------------------

``sqlalchemy-drill`` registers three dialects; only one of them is JPype-based:

================= ================================================== =====================
URL scheme        Implementation                                     DBAPI module
================= ================================================== =====================
``drill://``      Drill's REST API (``sqlalchemy_drill.drilldbapi``) custom, no JVM needed
``drill+jdbc://`` JDBC, via ``jpype.dbapi2``                         ``jpype.dbapi2``
``drill+odbc://`` ODBC, via ``pyodbc``                               ``pyodbc``
================= ================================================== =====================

Use ``drill+jdbc://`` to get JPype.  As with the direct connection above, the
JVM must already be running (with the same classpath) before you create the
engine -- the dialect checks ``jpype.isJVMStarted()`` and raises a clear
error if it is not:

.. code-block:: python

   import jpype
   from sqlalchemy import create_engine, text

   jpype.startJVM(classpath=[...])  # same classpath as above

   engine = create_engine("drill+jdbc://localhost:31010")
   with engine.connect() as conn:
       result = conn.execute(text("SELECT * FROM cp.`employee.json` LIMIT 5"))
       for row in result:
           print(row)

This also means ``pandas.read_sql`` works against a Drill-backed SQLAlchemy
engine exactly as described in `Working with pandas`_ above, since the
engine's underlying connection is a ``jpype.dbapi2.Connection`` the whole
way down.

.. _`Working with SQLAlchemy against other databases`:

Working with SQLAlchemy against other databases
--------------------------------------------------

``drill+jdbc://`` works because ``sqlalchemy-drill`` is a dialect written
*specifically* for ``jpype.dbapi2`` -- its ``create_connect_args()`` builds
the JDBC URL and nothing else, since ``jpype.dbapi2`` already satisfies
everything else PEP 249 requires.  There is no equivalent generic "JDBC
dialect" package that does this for arbitrary databases.

The tempting shortcut is to point one of SQLAlchemy's *existing* dialects
(e.g. the built-in ``sqlite`` one) at a ``jpype.dbapi2`` connection via
``create_engine("sqlite://", creator=lambda: dbapi2.connect(dsn))``.  This
does **not** reliably work: SQLAlchemy dialects are written against a
specific *real* DBAPI module, not just against PEP 249, and often call
driver extension methods beyond the DB-API 2.0 surface on every new
connection.  Concretely, the built-in ``sqlite`` dialect's ``on_connect``
hook unconditionally calls ``dbapi_connection.create_function(...)`` to
register SQL ``REGEXP`` support, a ``sqlite3.Connection``-specific method
that has no PEP 249 equivalent and that ``jpype.dbapi2.Connection`` does
not (and cannot portably) implement::

    AttributeError: 'Connection' object has no attribute 'create_function'

This is not a PEP 249 shortfall in ``jpype.dbapi2`` -- PEP 249 compliance
is the correct and complete target for a DB-API module, and
``jpype.dbapi2`` meets it.  The gap is one level up: SQLAlchemy's dialect
model couples each dialect to a specific real driver module's extensions,
not to the DB-API 2.0 contract alone, so an existing dialect cannot be
retargeted at a different, merely-PEP-249-compliant connection.  Tools
built on SQLAlchemy dialects -- Alembic included, since it drives schema
changes through whatever ``Connection``/dialect SQLAlchemy hands it --
work well against a database that has a ``jpype.dbapi2``-based dialect
written for it (Drill, following the pattern above).  For a database that
doesn't have one yet, writing a purpose-built dialect the way
``sqlalchemy-drill`` did is the correct path, not patching around an
existing one.


Conclusion
==========

This wraps up the JPype dbapi2 module.  Because JDBC supports many different
database drivers, not every behavior is defined on every driver.  Consult the
driver specific information to determine what is available.  

The dbapi2 does not fully cover all of the capabilities of the JDBC driver.  To
access functions that are not defined in DBAPI2, the JDBC native objects can 
be accessed on both the connection and the cursor objects.

.. _PEP-0249: https://www.python.org/dev/peps/pep-0249/

.. _connection.rollback: #jpype.dbapi2.Connection.rollback
.. _connection.commit: #jpype.dbapi2.Connection.commit
.. _connection.adapters: #jpype.dbapi2.Connection.adapters
.. _connection.setters: #jpype.dbapi2.Connection.setters
.. _connection.converters: #jpype.dbapi2.Connection.converters
.. _connection.isolation_level: #jpype.dbapi2.Connection.isolation_level
.. _cursor.description: #jpype.dbapi2.Cursor.description
