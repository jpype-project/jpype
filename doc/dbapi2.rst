##################
JPype DBAPI2 Guide
##################

`Introduction`
==============

One common use of JPype is to provide access to databases using JDBC.  The JDBC
API is well established, very capable, and supports most databases.
JPype can be used to access JDBC both directly or through the use of the Python
DBAPI2 as layed (see PEP-0249_).  Unfortunately, the Python API leaves a lot of
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
inherit from `:py:class:jpype.dbapi2.Error`.  This is the exception inheritance layout::

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
conversions from the corresponding type.  For all other conversions, an adapter.
An adapter is defined as a type to convert from and a conversion function which 
takes a single argument that returns a Java object.

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
function which processes the database metadata into a type.

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

JDBA provides more than one way to access data returned from a result.
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
the `.execute*()` method are untyped.  When the database module sees
a Python string object, it doesn't know if it should be bound as a
simple `CHAR` column, as a raw `BINARY` item, or as a `DATE`.

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
covers types `STRING`, `CHAR`, `NCHAR` , `NVARCHAR` , `VARCHAR`, 
and `OTHER`.  JPype dbapi2 supports both the recommended Python types and
the fine grain JDBC types.  Each type is represented by an object 
of type JBDCType.

.. autoclass:: jpype.dbapi2.JDBCType
   :members:

The following types are defined with the correspond Python grouping, the
default setter, getter, and Python type.  For types that support more than
one type of getter, the special getter can be applied as the converter for
the type.  For example, the default configuration has ``getter[BLOB] = BINARY.get``,
to get the Blob type use ``getter[BLOB] = BLOB.get`` or specify it when
calling `use <cursor.use_>`_.

.. The link to cursor.use above appears to be broken. Does cursor.use exist?

======== ======================== =================== ============== ================= ===============
Group    JDBC Type                Default Getter      Default Setter PyTypes           Special Getter 
======== ======================== =================== ============== ================= ===============
DATE     DATE                     getDate             setDate        datetime.datetime                
DATETIME TIMESTAMP                getTimestamp        setTimestamp   datetime.datetime                
TIME     TIME                     getTime             setTime        datetime.datetime                
-------- ------------------------ ------------------- -------------- ----------------- ---------------
DECIMAL  DECIMAL                  getBigDecimal       setBigDecimal  decimal.Decimal                  
DECIMAL  NUMERIC                  getBigDecimal       setBigDecimal  decimal.Decimal                  
-------- ------------------------ ------------------- -------------- ----------------- ---------------
FLOAT    FLOAT                    getDouble           setDouble      float                            
FLOAT    DOUBLE                   getDouble           getDouble      float                            
FLOAT    REAL                     getFloat            setFloat       float                            
-------- ------------------------ ------------------- -------------- ----------------- ---------------
NUMBER   BOOLEAN                  getBoolean          setBoolean     bool                             
NUMBER   BIT                      getBoolean          setBoolean     bool                             
NUMBER   TINYINT  (0..255)        getShort            setShort       int                              
NUMBER   SMALLINT (-2^15..2^15)   getShort            getShort       int                              
NUMBER   INTEGER  (-2^31..2^31)   getInt              getInt         int                              
NUMBER   BIGINT   (-2^63..2^63)   getLong             getLong        int                              
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
         ARRAY                    getObject                                            getArray       
         OBJECT                   getObject                                            getObject      
         NULL                     getObject                                            getObject      
         REF                      getObject                                            getRef         
         ROWID                    getObject                                            getRowId       
         RESULTSET                getObject                                            getObject      
         TIME_WITH_TIMEZONE       getObject                                            getTime        
         TIMESTAMP_WITH_TIMEZONE  getObject                                            getTimeStamp   
-------- ------------------------ ------------------- -------------- ----------------- ---------------
   *     ASCII_STREAM             getAsciiStream                                                      
   *     BINARY_STREAM            getBinaryStream                                                     
   *     CHARACTER_STREAM         getCharacterStream                                                  
   *     ASCII_STREAM             getAsciiStream                                                      
   *     BINARY_STREAM            getBinaryStream                                                     
   *     CHARACTER_STREAM         getCharacterStream                                                  
   *     NCHARACTER_STREAM        getNCharacterStream                                                 
   *     URL                      getURL                                                              
======== ======================== =================== ============== ================= ===============

Some of these types never correspond to a SQL type but are used only to specify
getters and setters for a particular parameter or column.

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
          rc = JDBCType.get(self, *args)
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
connection is already closed, and HSQLBD sets the ``isValid`` to false but not
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
.. _connection.adapters: #jpype.dbapi2.JDBCType.adapters
.. _connection.setters: #jpype.dbapi2.JDBCType.setters
.. _connection.converters: #jpype.dbapi2.JDBCType.converters
.. _cursor.use: #jpype.dbapi2.Cursor.use
.. _cursor.description: #jpype.dbapi2.Cursor.description
.. _jdbctype.adapters: #jpype.dbapi2.Connection.adapters
