import _jpype
from . import _jinit
from . import _jcustomizer
from . import types as _jtypes
import typing
import _jpype
import time
import threading
import datetime

# TODO
#  - Callable procedures
#  - Isolation levels
#  - Default adaptors
#  - A complete testbench
#  - Testbench with more than one DB
#  - Documentation

# This a generic implementation of PEP-249
__all__ = ['ARRAY', 'ASCII_STREAM', 'BIGINT', 'BINARY', 'BINARY_STREAM', 'BIT',
           'BLOB', 'BOOLEAN', 'Binary', 'CHAR', 'CHARACTER_STREAM', 'CLOB',
           'Connection', 'Cursor', 'DATE', 'DATETIME', 'DECIMAL', 'DOUBLE',
           'DataError', 'DatabaseError', 'Date', 'DateFromTicks', 'Error',
           'FLOAT', 'GETTERS_BY_NAME', 'GETTERS_BY_TYPE', 'INTEGER',
           'IntegrityError', 'InterfaceError', 'InternalError', 'JDBCType',
           'LONGNVARCHAR', 'LONGVARBINARY', 'LONGVARCHAR', 'NCHAR',
           'NCHARACTER_STREAM', 'NCLOB', 'NULL', 'NUMBER', 'NUMERIC', 'NVARCHAR',
           'NotSupportedError', 'OBJECT', 'OTHER', 'OperationalError',
           'ProgrammingError', 'REAL', 'REF', 'RESULTSET', 'ROWID',
           'SETTERS_BY_META', 'SETTERS_BY_TYPE', 'SMALLINT', 'SQLXML', 'STRING',
           'TEXT', 'TIME', 'TIMESTAMP', 'TIMESTAMP_WITH_TIMEZONE',
           'TIME_WITH_TIMEZONE', 'TINYINT', 'Time', 'TimeFromTicks', 'Timestamp',
           'TimestampFromTicks', 'URL', 'VARBINARY', 'VARCHAR', 'Warning',
           'apilevel', 'connect', 'paramstyle', 'threadsafety']

apilevel = "2.0"
threadsafety = 2
paramstyle = 'qmark'

_SQLException = None
_SQLTimeoutException = None
_registry = {}
_types = []


def _nop(x):
    return x

###############################################################################
# Types


class JDBCType(object):
    def __init__(self, name, code=None, getter=None, setter=None):
        """ (internal) Create a new JDBC type. """
        if isinstance(name, (str, type(None))):
            self._name = name
            self._values = [name]
        else:
            self._name = name[0]
            self._values = name
        self._code = code
        self._getter = getter
        self._setter = setter
        if code is not None:
            _registry[code] = self
        if name is not None:
            _registry[self._name.upper()] = self
        _types.append(self)
        if _jpype.isStarted():
            java = _jpype._JPackage("java")
            self._initialize(java.sql.CallableStatement, java.sql.PreparedStatement, java.sql.ResultSet)

    def _initialize(self, cs, ps, rs):
        """ Called after the JVM starts initialize Java resources """
        if self._getter is None:
            self._getter = "getObject"
        self._rsget = getattr(rs, self._getter)
        self._csget = getattr(cs, self._getter, None)
        if self._setter is None:
            self._setter = "setObject"
        self._psset = getattr(ps, self._setter)

    def get(self, rs, column, st):
        """ A method to retrieve a specific JDBC type.

        To use a getter add the fetch method to the JDBC type matching the
        column type to be pulled.  For example, to set the getter for FLOAT to
        use the OBJECT getter, use  ``cx.getter[FLOAT] = OBJECT.get``.

        Not all getters are available on all database drivers.  Consult the
        database driver documentation for details.
        """
        try:
            if st:
                return self._csget(rs, column)
            return self._rsget(rs, column)
        except _SQLException as ex:
            raise InterfaceError("Unable to get '%s' using '%s'" % (self._name, self._getter)) from ex

    def set(self, ps, column, value):
        """ A method used to set a parameter to a query.

        To use a setter place the set method in the setter dict corresponding.
        For example, if the database supports Blob types, the default handler
        for BLOB can be changed from OBJECT to BLOB with
        ``cx.setter[BLOB] = BLOB.set``.

        Not all setters are available on all database drivers.  Consult the
        database driver documentation for details.
        """
        # Set the column with the specialized method
        try:
            if self._psset._matches(ps, column, value):
                return self._psset(ps, column, value)
            # Otherwise, try to set with the generic method
            return ps.setObject(column, value)
        except TypeError as ex:
            raise InterfaceError("Unable to convert '%s' into '%s'" % (type(value).__name__, self._name)) from ex
        except OverflowError as ex:
            raise InterfaceError("Unable to convert '%s' into '%s' calling '%s'" % (type(value).__name__, self._name, self._setter)) from ex

    def __repr__(self):
        if self._name is None:
            return ""
        return self._name

    def __eq__(self, other):
        return other in self._values

    def __hash__(self):
        return hash(self._name)


class _JDBCTypePrimitive(JDBCType):
    def get(self, rs, column, st):
        try:
            if st:
                rc = self._csget(rs, column)
            else:
                rc = self._rsget(rs, column)
            if rc == 0 and rs.wasNull():
                return None
            return rc
        except _SQLException as ex:
            raise InterfaceError("Unable to get '%s' using '%s'" % (self._name, self._getter)) from ex


# From https://www.cis.upenn.edu/~bcpierce/courses/629/jdkdocs/guide/jdbc/getstart/mapping.doc.html
# DATALINK = JDBCType('DATALINK',70)
# DISTINCT= JDBCType('DISTINCT',2001)
# REF_CURSOR = JDBCType('REF_CURSOR',2012)
# STRUCT = JDBCType('STRUCT',2002)
ARRAY = JDBCType('ARRAY', 2003, 'getArray', 'setArray')
BIGINT = _JDBCTypePrimitive('BIGINT', -5, 'getLong', 'setLong')
BIT = JDBCType('BIT', -7, 'getBoolean', 'setBoolean')
BLOB = JDBCType('BLOB', 2004, 'getBlob', 'setBlob')
BOOLEAN = _JDBCTypePrimitive('BOOLEAN', 16, 'getBoolean', 'setBoolean')
CHAR = JDBCType('CHAR', 1, 'getString', 'setString')
CLOB = JDBCType('CLOB', 2005, 'getClob', 'setClob')
DATE = JDBCType('DATE', 91, 'getDate', 'setDate')
DOUBLE = _JDBCTypePrimitive('DOUBLE', 8, 'getDouble', 'setDouble')
INTEGER = _JDBCTypePrimitive('INTEGER', 4, 'getInt', 'setInt')
OBJECT = JDBCType('OBJECT', 2000)
LONGNVARCHAR = JDBCType('LONGNVARCHAR', -16, 'getString', 'setString')
LONGVARBINARY = JDBCType('LONGVARBINARY', -4, 'getBytes', 'setBytes')
LONGVARCHAR = JDBCType('LONGVARCHAR', -1, 'getString', 'setString')
NCHAR = JDBCType('NCHAR', -15, 'getString', 'setString')
NCLOB = JDBCType('NCLOB', 2011, 'getNClob', 'setNClob')
NULL = JDBCType('NULL', 0)
NUMERIC = JDBCType('NUMERIC', 2, 'getBigDecimal', 'setBigDecimal')
NVARCHAR = JDBCType('NVARCHAR', -9, 'getClob', 'setClob')
OTHER = JDBCType('OTHER', 1111)
REAL = _JDBCTypePrimitive('REAL', 7, 'getFloat', 'setFloat')
REF = JDBCType('REF', 2006, 'getRef', 'setRef')
ROWID = JDBCType('ROWID', -8, 'getRowId', 'setRowId')
RESULTSET = JDBCType('RESULTSET', -10, 'getObject', 'setObject')
SMALLINT = _JDBCTypePrimitive('SMALLINT', 5, 'getShort', 'setShort')
SQLXML = JDBCType('SQLXML', 2009, 'getSQLXML', 'setSQLXML')
TIME = JDBCType('TIME', 92, 'getTime', 'setTime')
TIME_WITH_TIMEZONE = JDBCType('TIME_WITH_TIMEZONE', 2013, 'getTime', 'setTime')
TIMESTAMP = JDBCType('TIMESTAMP', 93, 'getTimestamp', 'setTimestamp')
TIMESTAMP_WITH_TIMEZONE = JDBCType('TIMESTAMP_WITH_TIMEZONE', 2014, 'getTimestamp', 'setTimestamp')
TINYINT = _JDBCTypePrimitive('TINYINT', -6, 'getShort', 'setShort')
VARBINARY = JDBCType('VARBINARY', -3, 'getBytes', 'setBytes')
VARCHAR = JDBCType('VARCHAR', 12, 'getString', 'setString')

# Aliases required by DBAPI2
STRING = JDBCType(['STRING', 'CHAR', 'NCHAR', 'NVARCHAR', 'VARCHAR', 'OTHER'], None,
                  'getString', 'setString')
TEXT = JDBCType(['TEXT', 'CLOB', 'LONGVARCHAR', 'LONGNVARCHAR', 'NCLOB', 'SQLXML'], None,
                'getString', 'setString')
BINARY = JDBCType(['BINARY', 'BLOB', 'LONGVARBINARY', 'VARBINARY'], -2,
                  'getBytes', 'setBytes')
NUMBER = JDBCType(['NUMBER', 'BOOLEAN', 'BIGINT', 'BIT', 'INTEGER', 'SMALLINT', 'TINYINT'], None,
                  'getObject', 'setObject')
FLOAT = _JDBCTypePrimitive(['FLOAT', 'REAL', 'DOUBLE'], 6,
                           'getDouble', 'setDouble')
DECIMAL = JDBCType(['DECIMAL', 'NUMERIC'], 3,
                   'getBigDecimal', 'setBigDecimal')
DATETIME = TIMESTAMP

# Special types
ASCII_STREAM = JDBCType(None, None, 'getAsciiStream', 'setAsciiStream')
BINARY_STREAM = JDBCType(None, None, 'getBinaryStream', 'setBinaryStream')
CHARACTER_STREAM = JDBCType(None, None, 'getCharacterStream', 'setCharacterStream')
NCHARACTER_STREAM = JDBCType(None, None, 'getNCharacterStream', 'setNCharacterStream')
URL = JDBCType(None, None, 'getURL', 'setURL')

# The converters are defined in a customizer


def _asPython(x):
    return x._py()


# This maps the types reported by the columns to the type used for the getter
# and converter
_default_map = {ARRAY: OBJECT, OBJECT: OBJECT, NULL: OBJECT,
                REF: OBJECT, ROWID: OBJECT, RESULTSET: OBJECT,
                TIME_WITH_TIMEZONE: OBJECT, TIMESTAMP_WITH_TIMEZONE: OBJECT,
                NVARCHAR: STRING, CHAR: STRING,
                NCHAR: STRING, VARCHAR: STRING, BINARY: BINARY,
                BLOB: BINARY, LONGVARBINARY: BINARY, VARBINARY: BINARY,
                NUMBER: NUMBER, BOOLEAN: BOOLEAN, BIGINT: BIGINT,
                BIT: BIT, INTEGER: INTEGER, SMALLINT: SMALLINT,
                TINYINT: TINYINT, FLOAT: FLOAT, REAL: REAL,
                DECIMAL: DECIMAL, NUMERIC: NUMERIC,
                DATE: DATE, TIMESTAMP: TIMESTAMP, TIME: TIME,
                CLOB: STRING, NCLOB: STRING, STRING: STRING,
                TEXT: STRING, SQLXML: STRING, LONGVARCHAR: STRING,
                LONGNVARCHAR: STRING, DOUBLE: DOUBLE, OTHER: OBJECT
                }

_default_setters = {}

_default_converters = {}

_default_adapters = {}


# Setters take (connection, meta, col, type) -> JDBCTYPE
def SETTERS_BY_META(cx, meta, col, ptype):
    """ Option for setters to use the metadata of the parameters.

    On some databases this option is useless as they do not track parameter
    types.  This method can be cached for faster performance when lots of
    parameters.  Usually types can only be determined accurately on inserts
    into defined columns.
    """
    return _default_map[_registry[meta.getParameterType(col + 1)]]


SETTERS_BY_META._cachable = True


def SETTERS_BY_TYPE(cx, meta, col, ptype):
    """ Option for setters to use the type of the object passed.

    This option looks at the type of the parameter being passed
    from Python after adapters have been applied to determine the
    best setter.
    """
    return _default_setters.get(ptype, None)


# Getters take (connection, meta, col) -> JDBCTYPE
def GETTERS_BY_TYPE(cx, meta, idx):
    """ Option for getters to determine column type by the JDBC type.

    This option is the default option that uses the type code supplied in
    the meta data.  On some databases it is better to use the name.
    If the type code is OTHER, it will attempt to find a type by name.
    New types can be created with JDBCType for database specific types.
    """
    tp = _registry[meta.getColumnType(idx + 1)]
    if tp == OTHER:
        # Other may be found by name
        name = str(meta.getColumnTypeName(idx + 1)).upper()
        return _registry.get(name, tp)
    return _default_map[tp]


def GETTERS_BY_NAME(cx, meta, idx):
    """ Option to getters to determine column type by the column name.

    This option uses the column name to select the type.  It looks up
    the column type name, converts it to uppercase, and then searches
    for a matchine type.  It falls back to the type code meta information if
    the typename can not be found in the registery.  New types can be created
    using JDBCType for database specific types such as ``JSON``.
    """
    name = str(meta.getColumnTypeName(idx + 1)).upper()
    tp = _registry.get(name, None)
    if tp is None:
        tp = _registry[meta.getColumnType(idx + 1)]
    return _default_map.get(tp, tp)


###############################################################################
# Exceptions


class Warning(Exception):
    """Exception raised for important warnings like data truncations while
    inserting, etc. """
    pass


class Error(Exception):
    """Exception that is the base class of all other error exceptions. You can use
    this to catch all errors with one single except statement. Warnings are not
    considered errors and thus should not use this class as base.
    """
    pass


class InterfaceError(Error, TypeError):
    """ Exception raised for errors that are related to the database interface
    rather than the database itself."""
    pass


class DatabaseError(Error):
    """ Exception raised for errors that are related to the database."""
    pass


class DataError(DatabaseError):
    """ Exception raised for errors that are due to problems with the processed
    data like division by zero, numeric value out of range, etc."""
    pass


class OperationalError(DatabaseError):
    """ Exception raised for errors that are related to the database's operation
    and not necessarily under the control of the programmer, e.g. an unexpected
    disconnect occurs, the data source name is not found, a transaction could not
    be processed, a memory allocation error occurred during processing, etc."""
    pass


class IntegrityError(DatabaseError):
    """ Exception raised when the relational integrity of the database is affected,
    e.g. a foreign key check fails."""
    pass


class InternalError(DatabaseError):
    """ Exception raised when the database encounters an internal error, e.g. the
    cursor is not valid anymore, the transaction is out of sync, etc."""
    pass


class ProgrammingError(DatabaseError):
    """ Exception raised for programming errors, e.g. table not found or already
    exists, syntax error in the SQL statement, wrong number of parameters
    specified, etc."""
    pass


class NotSupportedError(DatabaseError):
    """ Exception raised in case a method or database API was used which is not
    supported by the database, e.g. requesting a .rollback() on a connection that
    does not support transaction or has transactions turned off.
    """
    pass


class _UnsupportedTypeError(InterfaceError, TypeError):
    pass


###############################################################################
# Connection

_default = object()


def connect(dsn, *, driver=None, driver_args=None,
            adapters=_default, converters=_default,
            getters=GETTERS_BY_TYPE, setters=SETTERS_BY_TYPE, **kwargs):
    """ Create a connection to a database.

    Arguments to the driver depend on the database type.

    Args:
       dsn (str): The database connection string for JDBC.
       driver (str, optional): A JDBC driver to load.
       driver_args: Arguments to the driver.  This may either be a dict,
          java.util.Properties.  If not supplied, kwargs are used as as the
          parameters for the JDBC connection.
       *kwargs: Arguments to the driver if not supplied as
          driver_args.

    Raises:
       Error if the connection cannot be established.

    Returns:
       A new connection if successful.
    """
    Properties = _jpype.JClass("java.util.Properties")
    if driver:
        _jpype.JClass('java.lang.Class').forName(driver).newInstance()
    DM = _jpype.JClass('java.sql.DriverManager')

    # User is supplying Java properties
    if isinstance(driver_args, Properties):
        connection = DM.getConnection(dsn, driver_args)

    # User is supplying a mapping that can be converted Properties
    elif isinstance(driver_args, typing.Mapping):
        info = Properties()
        for k, v in driver_args.items():
            info.setProperty(k, v)
        connection = DM.getConnection(dsn, info)

    # User supplied nothing
    elif driver_args is None:
        connection = DM.getConnection(dsn)

    # Otherwise use the kwargs
    else:
        info = Properties()
        for k, v in kwargs.items():
            info.setProperty(k, v)
        connection = DM.getConnection(url, info)

    return Connection(connection, adapters, converters, setters, getters)


class Connection(object):
    """ Connection provides access to a JDBC database.

    Connections are managed and can be as part of a Python with statement.
    Connections close automatically when they are garbage collected, at the
    end of a with statement scope, or when manually closed.  Once a connection
    is closed all operations using the database will raise an Error.
    """
    Error = Error
    Warning = Warning
    InterfaceError = InterfaceError
    DatabaseError = DatabaseError
    InternalError = InternalError
    OperationalError = OperationalError
    ProgrammingError = ProgrammingError
    IntegrityError = IntegrityError
    DataError = DataError
    NotSupportedError = NotSupportedError

    def __init__(self, jconnection, adapters, converters, setters, getters):
        self._jcx = jconnection
        # Required by PEP 249
        # https://www.python.org/dev/peps/pep-0249/#commit
        self._jcx.setAutoCommit(False)

        # Handle defaults
        if adapters is _default:
            adapters = dict(_default_adapters)
        if adapters is None:
            adapters = {}
        if converters is _default:
            converters = dict(_default_converters)
        if converters is None:
            converters = {}

        self._closed = False
        self._batch = jconnection.getMetaData().supportsBatchUpdates()
        self._adapters = adapters
        self._converters = converters
        self._getters = getters
        self._setters = setters

    @property
    def adapters(self):
        """ Adapters are used to convert Python types into JDBC types.

        Adaptors are stored in a mapping from incoming type to an adapter
        function.  Adapters set on a connection apply only to that connection.
        Adapters can be overriden when calling the ``.execute*()`` method.

        Adapters can also be set on the JDBC types directly.
        """
        return self._adapters

    @adapters.setter
    def adapters(self, v):
        if v is None:
            v = {}
        if not isinstance(v, typing.Mapping):
            raise _UnsupportedTypeError("Mapping is required")
        self._adapters = v

    @property
    def converters(self):
        """ Converters are applied when retrieving a JDBC type from the database.

        """
        return self._converters

    @converters.setter
    def converters(self, v):
        if v is None:
            v = {}
        if not isinstance(v, typing.Mapping):
            raise _UnsupportedTypeError("Mapping is required")
        self._converters = v

    @property
    def getters(self):
        """ Getters are used to retrieve JDBC types from the database following
        a ``.fetch*()``.

        Getters should be a function taking (connection, meta, col) -> JDBCTYPE
        """
        return self._getters

# Setters take (connection, meta, col, type) -> JDBCTYPE
    @getters.setter
    def getters(self, v):
        self._getters = v

    @property
    def setters(self):
        """ Setter are used to set parameters to ``.execute*()`` methods.

        Setters should be a function taking (connection, meta, col, type) -> JDBCTYPE
        """
        return self._setters

    @setters.setter
    def setters(self, v):
        self._setters = v

    def __setattr__(self, name, value):
        if isinstance(vars(type(self)).get(name, None), property):
            return object.__setattr__(self, name, value)
        if not name.startswith("_"):
            raise AttributeError("'%s' cannot be set" % name)
        object.__setattr__(self, name, value)

    def _close(self):
        if self._closed or not _jpype.isStarted():
            return
        if not self._jcx.isClosed():
            self._jcx.close()
        self._closed = True

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        self._close()

    def __del__(self):
        try:
            self._close()
        except Exception:  # pragma: no cover
            pass

    def close(self):
        """ Close the connection immediately (rather than whenever .__del__() is called).

        The connection will be unusable from this point forward; an Error (or
        subclass) exception will be raised if any operation is attempted with
        the connection. The same applies to all cursor objects trying to use
        the connection. Note that closing a connection without committing the
        changes first will cause an implicit rollback to be performed.  """
        self._validate()
        self._close()

    def commit(self):
        """Commit any pending transaction to the database.

        Calling commit on a cooonection that does not support the operation
        will raise NotSupportedError.
        """
        self._validate()
        if self._jcx.getAutoCommit():
            raise NotSupportedError("Autocommit is enabled")
        try:
            self._jcx.commit()
        except Exception as ex:  # pragma: no cover
            raise OperationalError(ex.message()) from ex

    def rollback(self):
        """Rollback the transaction.

        This method is optional since not all databases provide transaction
        support.  Calling rollback on a cooonection that does not support will
        raise NotSupportedError.

        In case a database does provide transactions this method causes the
        database to roll back to the start of any pending transaction. Closing
        a connection without committing the changes first will cause an
        implicit rollback to be performed.
        """
        self._validate()
        if self._jcx.getAutoCommit():
            raise NotSupportedError("Autocommit is enabled", self.autocommit)
        try:
            self._jcx.rollback()
        except Exception as ex:  # pragma: no cover
            raise OperationalError(ex.message()) from ex

    def cursor(self):
        """ Return a new Cursor Object using the connection. """
        self._validate()
        return Cursor(self)

    def _validate(self):
        if self._closed or self._jcx.isClosed():
            raise ProgrammingError

    @property
    def connection(self):
        """ Get the JDBC connection that is backing this Python
        Connection object.

        This can be used to retrieve additional metadata and other features
        that are outside of the scope of the DBAPI driver.
        """
        return self._jcx

    @property
    def autocommit(self):
        """ bool: Property controlling autocommit behavior.

        By default connects are not autocommit.  Setting autocommit
        will result in commit and rollback producing a ProgrammingError.
        """
        self._validate()
        return self._jcx.getAutoCommit()

    @autocommit.setter
    def autocommit(self, enabled):
        self._validate()
        self._jcx.setAutoCommit(enabled)

    @property
    def typeinfo(self):
        """ list: The list of types that are supported by this driver.

        This is useful to find out the capabilities of the driver.
        """
        self._validate()
        out = {}
        metadata = self._jcx.getMetaData()
        with metadata.getTypeInfo() as resultSet:
            while (resultSet.next()):
                try:
                    key = str(resultSet.getString("TYPE_NAME"))
                    out[key] = _registry[resultSet.getInt("DATA_TYPE")]
                except KeyError as ex:  # pragma: no cover
                    raise DatabaseError("Unknown data type '%s'" % key) from ex
        return out


###############################################################################
# Cursor


class Cursor(object):
    """ Cursors are used to execute queries and retrieve results.

    Part PreparedStatement, part ResultSet,  Cursors are a mixture of
    both.  The native resultSet can be accessed with ``resultSet``.

    Cursors are managed and can be as part of a Python with statement.
    Cursors close automatically when they are garbage collected, at the
    end of a with statement scope, or when manually closed.  Once a cursor
    is closed all operations using the database will raise an Error.
    """

    def __init__(self, connection):
        if not isinstance(connection, Connection):
            raise TypeError
        self._connection = connection
        self._jcx = connection._jcx
        self._resultSet = None
        self._statement = None
        self._rowcount = -1
        self._arraysize = 1
        self._description = None
        self._closed = False
        self._resultGetters = None
        self._thread = threading.get_ident()
        self._last = None

    def _close(self):
        if self._closed or not _jpype.isStarted():
            return
        self._finish()
        self._closed = True

    def _setParams(self, params):
        cx = self._connection
        meta = self._statement.getParameterMetaData()
        count = meta.getParameterCount()
        types = self._parameterTypes
        if types is None:
            types = [None] * count
        if isinstance(params, str):
            raise _UnsupportedTypeError("parameters must be a sequence of values")
        if isinstance(params, typing.Sequence):
            if count != len(params):
                raise ProgrammingError("incorrect number of parameters (%d!=%d)"
                                       % (count, len(params)))
            for i in range(len(params)):
                p = params[i]

                # Find and apply the adapter
                a = cx._adapters.get(type(p), None)
                if a is not None:
                    p = a(p)

                # Find the setter
                if types[i] is None:
                    s = cx._setters(cx, meta, i, type(p))
                    types[i] = s
                if s is None:
                    raise _UnsupportedTypeError("no setter found for '%s'" % type(p).__name__)
                s.set(self._statement, i + 1, p)
            # Cache
            if hasattr(cx._setters, "_cachable"):
                self._parameterTypes = types
        elif isinstance(params, typing.Mapping):
            raise _UnsupportedTypeError("mapping parameters not supported")
        elif isinstance(params, typing.Iterable):
            for i, p in enumerate(params):
                if i >= count:
                    raise ProgrammingError("incorrect number of parameters (%d!=%d)" % (count, i + 1))

                # Find and apply the adapter
                a = cx._adapters.get(type(p), None)
                if a is not None:
                    p = a(p)

                # Find the setter
                if types[i] is None:
                    s = cx._setters(cx, meta, i, type(p))
                    types[i] = s
                if s is None:
                    raise _UnsupportedTypeError("no setter found for '%s'" % type(p).__name__)
                s.set(self._statement, i + 1, p)
            if count != i + 1:
                raise ProgrammingError("incorrect number of parameters (%d!=%d)" % (count, i + 1))
            # Cache
            if hasattr(cx._setters, "_cachable"):
                self._parameterTypes = types
        else:
            raise _UnsupportedTypeError("'%s' parameters not supported" % (type(params).__name__))  # pragma: no cover

    def _onResultSet(self, rs):
        meta = rs.getMetaData()
        self._resultSet = rs
        self._resultSetMeta = meta
        self._resultSetCount = meta.getColumnCount()
        self._columnTypes = None

    def _fetchRow(self, converters):
        cx = self._connection
        count = self._resultSetCount
        meta = self._resultSetMeta

        byPosition = False
        if converters is _default:
            converters = cx._converters
        if isinstance(converters, typing.Sequence):
            if len(converters) != count:
                raise ProgrammingError("converter list size incorrect")
            byPosition = True

        # Get all the column types
        if self._columnTypes is None:
            gk = cx._getters
            # We need the type information for the columns
            self._columnTypes = [gk(cx, meta, i) for i in range(count)]
        if len(self._columnTypes) != count:
            raise ProgrammingError("incorrect number of columns")
        try:
            row = []
            for idx in range(count):
                tp = self._columnTypes[idx]
                # Fetch the value
                value = tp.get(self._resultSet, idx + 1, False)
                if value is None or converters is None:
                    row.append(value)
                elif byPosition:
                    # find the column converter by type
                    converter = converters[idx]
                    row.append(converter(value))
                else:
                    # find the column converter by type
                    converter = cx._converters.get(type(value), _nop)
                    row.append(converter(value))
            return row
        except TypeError as ex:
            raise _UnsupportedTypeError(str(ex)) from ex

    def _validate(self):
        """ Called before any method that requires the statement to be open. """
        if self._closed or self._jcx.isClosed():
            raise ProgrammingError("Cursor is closed")
        if threading.get_ident() != self._thread:
            raise ProgrammingError("Threading error")

    def _check_executed(self):
        """ Called before any method that requires the resultSet to be open. """
        if self._closed or self._jcx.isClosed() or threading.get_ident() != self._thread:
            raise ProgrammingError("Cursor is closed")
        if self._resultSet is None:
            raise ProgrammingError("execute() first")

    def _finish(self):
        if self._resultSet is not None:
            self._resultSet.close()
            self._resultSet = None
        if self._statement is not None:
            self._statement.close()
            self._statement = None
        self._rowcount = -1
        self._description = None
        self._last = None

    @property
    def resultSet(self):
        """ Get the Java result set if available.

        The object will be closed on the next call to ``.execute*()``.
        """
        return self._resultSet

    @property
    def parameters(self):
        """ (extension) Parameters is read-only attribute is a sequence of
        6-item sequences.

        Each of these sequences contains information describing one result
        column:

        - type_name
        - jdbc_type
        - parameter_mode (1=in, 2=in/out, 4=out)
        - precision
        - scale
        - null_ok

        This can only be used after execute or callproc.
        """
        desc = []
        if not self._statement:
            raise ProgrammingError("No statement")
        meta = self._statement.getParameterMetaData()
        for i in range(1, meta.getParameterCount() + 1):
            desc.append((str(meta.getParameterTypeName(i)),
                         _registry[meta.getParameterType(i)],
                         meta.getParameterMode(i),
                         meta.getPrecision(i),
                         meta.getScale(i),
                         meta.isNullable(i),))
        return desc

    @property
    def description(self):
        """ Description is read-only attribute is a sequence of 7-item
        sequences.

        Each of these sequences contains information describing one result
        column:

        - name
        - type_code
        - display_size
        - internal_size
        - precision
        - scale
        - null_ok

        This can only be used if the last query produced a result set.
        """
        if self._description is not None:
            return self._description
        desc = []
        if not self._resultSet:
            return None
        meta = self._resultSet.getMetaData()
        for i in range(1, meta.getColumnCount() + 1):
            size = meta.getColumnDisplaySize(i)
            desc.append((str(meta.getColumnName(i)),
                         str(meta.getColumnTypeName(i)),
                         size,
                         size,
                         meta.getPrecision(i),
                         meta.getScale(i),
                         meta.isNullable(i),))
        self._description = desc
        return desc

    @property
    def rowcount(self):
        """ This read-only attribute specifies the number of rows that the last
        .execute*() affected (for DML statements like UPDATE or INSERT).

        The attribute is -1 in case no .execute*() has been performed on the
        cursor or the rowcount of the last operation is cannot be determined by
        the interface.  JDBC does not support getting the number of rows
        returned from SELECT, so for most drivers rowcount will be -1 after a
        SELECT statement.
        """
        return self._rowcount

    def close(self):
        """ Close the cursor now (rather than whenever __del__ is called).

        The cursor will be unusable from this point forward; an Error (or
        subclass) exception will be raised if any operation is attempted with
        the cursor.
        """
        self._validate()
        self._close()

    def callproc(self, procname, parameters=(), *, types=None):
        """ Call a stored procedure.

        (Not all JDBC drivers support this method)

        Call a stored database procedure with the given name. The sequence of
        parameters must contain one entry for each argument that the procedure
        expects. The result of the call is returned as modified copy of the
        input sequence. Input parameters are left untouched, output and
        input/output parameters replaced with possibly new values.

        For type output and input/output arguments, it is best to use
        types keyword argument to select the appropriate getters for the 
        returned arguments.  Converters are applied to output parameters.

        The procedure may also provide a result set as output. This must then
        be made available through the standard .fetch*() methods. 
        """
        try:
            self._validate()
            self._finish()
            if not isinstance(procname, str):
                raise _UnsupportedTypeError("procname must be str, not '%s'" % type(procname).__name__)
            if not isinstance(parameters, typing.Sequence):
                raise _UnsupportedTypeError("parameters must be sequence, not '%s'" % type(procname).__name__)
            query = "{CALL %s(%s)}" % (procname, ",".join("?" * len(parameters)))
            try:
                self._statement = self._jcx.prepareCall(query)
            except _SQLException as ex:
                raise ProgrammingError(ex.message()) from ex

            # This is a special one as we need to deal with in and out arguments
            out = list(parameters)
            cx = self._connection
            meta = self._statement.getParameterMetaData()
            count = meta.getParameterCount()
            if types is None:
                types = [None] * count
            else:
                if len(types) != count:
                    raise ProgrammingError("expected '%d' types, got '%d'" % (count, len(types)))
            for i in range(count):
                # Lookup the JDBC Type
                p = parameters[i]
                a = cx._adapters.get(type(p), None)
                if a is not None:
                    p = a(p)

                if types[i] is None:
                    types[i] = cx._setters(cx, meta, i, type(p))
                jdbcType = types[i]
                if jdbcType is None:
                    raise _UnsupportedTypeError("no setter found for '%s'" % type(p).__name__)

                mode = meta.getParameterMode(i + 1)
                if mode == 1:
                    jdbcType.set(self._statement, i + 1, p)
                    types[i] = None
                if mode == 2:
                    jdbcType.set(self._statement, i + 1, p)
                    self.registerOutParameter(i + 1, jdbcType._code)
                if mode == 4:
                    self.registerOutParameter(i + 1, jdbcType._code)

            if self._statement.execute():
                self._onResultSet(self._statement.getResultSet())
            self._rowcount = self._statement.getUpdateCount()

            for i, t in enumerate(types):
                if t is None:
                    continue

                # Find the converter to apply to the column
                value = t.get(self._statement, i + 1, True)
                # FIXME how do we get a converv
                converter = cx._converters[type(value)]
                out[i] = converter(value)

            return out

        # Restore the defaults
        except Exception as ex:
            self._converters = self._connection._converters
            self._getters = self._connection._getters
            raise ex
        finally:
            self._adapters = self._connection._adapters
            self._setters = self._connection._setters

    def execute(self, operation, parameters=None, *, types=None, keys=False):
        """
        Prepare and execute a database operation (query or command).

        Parameters may be provided as sequence and will be bound to variables
        in the operation. Variables are specified in a qmark notation.  JDBC
        does not support mapping style parameters.

        After executing a statement, the rowcount will be updated.  If the
        statement has no result set then the rowcount will be -1.  A statement
        can produce multiple result sets.  Use ``.nextset()`` to traverse the
        sets.

        Parameters:
           operation (str): A statement to be executed.
           parameters (list, optional): A list of parameters for the statement.
              The number of parameters much match the number required by the
              statement or an Error will be raised.
           keys (bool, optional): Specify if the keys should be available to 
              retrieve. (Default False) 

        Returns:
           This cursor.
        """
        self._last = None
        self._parameterTypes = types
        self._validate()
        self._finish()
        if parameters is None:
            parameters = ()
        if not isinstance(parameters, (typing.Sequence, typing.Iterable, typing.Iterator)):
            raise _UnsupportedTypeError("parameters are of unsupported type '%s'" % type(parameters).__name__)
        # complete the previous operation
        try:
            if keys:
                self._statement = self._jcx.prepareStatement(operation, 1)
            else:
                self._statement = self._jcx.prepareStatement(operation)
        except TypeError as ex:
            raise _UnsupportedTypeError(str(ex))
        except _SQLException as ex:
            raise ProgrammingError(ex.message()) from ex
        self._executeone(parameters)
        return self

    def _executeone(self, params):
        self._setParams(params)
        if self._statement.execute():
            self._onResultSet(self._statement.getResultSet())
        self._rowcount = self._statement.getUpdateCount()
        return self._rowcount

    def executemany(self, operation, seq_of_parameters, *, types=None, keys=False):
        """
        Prepare a database operation (query or command) and then execute it
        against all parameter sequences or mappings found in the sequence
        seq_of_parameters.

        Modules are free to implement this method using multiple calls to the
        .execute() method or by using array operations to have the database
        process the sequence as a whole in one call.

        Use of this method for an operation which produces one or more result
        sets constitutes undefined behavior, and the implementation is
        permitted (but not required) to raise an exception when it detects that
        a result set has been created by an invocation of the operation.

        The same comments as for .execute() also apply accordingly to this
        method.

        Args:
           operation (str): A statement to be executed.
           seq_of_parameters (list, optional): A list of lists of parameters
               for the statement.  The number of parameters much match the number
               required by the statement or an Error will be raised.
           keys (bool, optional): Specify if the keys should be available to 
              retrieve. (Default False) For drivers that do not support
              batch updates only that last key will be returned.

        Returns:
           This cursor.
        """
        self._last = None
        self._parameterTypes = types
        self._validate()
        if seq_of_parameters is None:
            seq_of_parameters = ()
        # complete the previous operation
        self._finish()
        try:
            if keys:
                self._statement = self._jcx.prepareStatement(operation, 1)
            else:
                self._statement = self._jcx.prepareStatement(operation)
        except TypeError as ex:
            raise _UnsupportedTypeError(str(ex))
        except _SQLException as ex:
            raise ProgrammingError("Failed to prepare '%s'" % operation) from ex
        if self._connection._batch:
            return self._executeBatch(seq_of_parameters)
        else:  # pragma: no cover
            return self._executeRepeat(seq_of_parameters)

    def _executeBatch(self, seq_of_parameters):
        if isinstance(seq_of_parameters, typing.Iterable):
            for params in seq_of_parameters:
                self._setParams(params)
                self._statement.addBatch()
        else:
            raise _UnsupportedTypeError("'%s' is not supported" % type(seq_of_parameters).__name__)
        try:
            counts = self._statement.executeBatch()
        except _SQLException as ex:  # pragma: no cover
            raise ProgrammingError(ex.message()) from ex
        self._rowcount = sum(counts)
        if self._rowcount < 0:  # pragma: no cover
            self._rowcount = -1
        return self

    def _executeRepeat(self, seq_of_parameters):  # pragma: no cover
        counts = []
        if isinstance(seq_of_parameters, typing.Iterable):
            for params in seq_of_parameters:
                counts.append(self._executeone(params))
        elif isinstance(seq_of_parameters, typing.Iterator):
            while True:
                try:
                    params = next(seq_of_parameters)
                    counts.append(self._executeone(params))
                except StopIteration:
                    break
        else:
            raise _UnsupportedTypeError("'%s' is not supported" % str(type(seq_of_parameters)))
        self._rowcount = sum(counts)
        if self._rowcount < 0:
            self._rowcount = -1
        return self

    def fetchone(self, *, types=None, converters=_default):
        """
        Fetch the next row of a query result set, returning a single
        sequence, or None when no more data is available.

        An Error (or subclass) exception is raised if the previous call to
        .execute*() did not produce any result set or no call was issued yet.
        """
        self._check_executed()
        if not self._resultSet.next():
            return None
        if types is not None:
            self._columnTypes = types
        return self._fetchRow(converters)

    def fetchmany(self, size=None, *, types=None, converters=_default):
        """ Fetch multiple results.

        Fetch the next set of rows of a query result, returning a sequence of
        sequences (e.g. a list of tuples). An empty sequence is returned when
        no more rows are available.

        The number of rows to fetch per call is specified by the parameter. If it
        is not given, the cursor's arraysize determines the number of rows to be
        fetched. The method should try to fetch as many rows as indicated by the
        size parameter. If this is not possible due to the specified number of rows
        not being available, fewer rows may be returned.

        An Error (or subclass) exception is raised if the previous call to
        ``.execute*()`` did not produce any result set or no call was issued yet.

        Note there are performance considerations involved with the size parameter.
        For optimal performance, it is usually best to use the .arraysize
        attribute. If the size parameter is used, then it is best for it to retain
        the same value from one ``.fetchmany()`` call to the next.
        """
        self._check_executed()
        if size is None:
            size = self._arraysize
        # Set a fetch size
        self._resultSet.setFetchSize(size)
        rows = []
        if types is not None:
            self._columnTypes = types
        for i in range(size):
            if not self._resultSet.next():
                break
            row = self._fetchRow(converters)
            rows.append(row)
        # Restore the default fetch size
        self._resultSet.setFetchSize(0)
        return rows

    def fetchall(self, *, types=None, converters=_default):
        """ Fetch all (remaining) rows of a query result, returning them as
        a sequence of sequences (e.g. a list of tuples). Note that the cursor's
        arraysize attribute can affect the performance of this operation.

        An Error (or subclass) exception is raised if the previous call to
        ``.execute*()`` did not produce any result set or no call was issued yet.
        """
        self._check_executed()
        # Set a fetch size
        rows = []
        if types is not None:
            self._columnTypes = types
        while self._resultSet.next():
            row = self._fetchRow(converters)
            rows.append(row)
        return rows

    def __iter__(self):
        """ (extension) Iterate through a cursor one record at a time.
        """
        self._check_executed()
        # Set a fetch size
        while self._resultSet.next():
            yield self._fetchRow(_default)

    def nextset(self):
        """ Get the next result set in this cursor.

        Not all databases support multiple result sets.

        This method will make the cursor skip to the next available set, discarding
        any remaining rows from the current set.

        If there are no more sets, the method returns None. Otherwise, it returns a
        true value and subsequent calls to the ``.fetch*()`` methods will return rows
        from the next result set.

        An Error (or subclass) exception is raised if the previous call to
        ``.execute*()`` did not produce any result set or no call was issued yet.
        """
        self._resultSet.close()
        if self._statement.getMoreResults():  # pragma: no cover
            self._onResultSet(_statement.getResultSet())
            return True
        else:
            self._rowcount = self._statement.getUpdateCount()
            return None

    @property
    def arraysize(self):
        """
        Specify the number of rows to fetch with ``.fetchmany()``.

        This read/write attribute specifies the number of rows to fetch
        at a time with ``.fetchmany()``. It defaults to 1 meaning to fetch a single row
        at a time.
        """
        return self._arraysize

    @arraysize.setter
    def arraysize(self, sz):
        self._arraysize = sz

    @property
    def lastrowid(self):
        """ Get the id of the last row inserted.

        This is not supported on all JDBC drivers. The ``.execute*()`` must have
        been executed with keys set to True.

        Returns:
           None if there is no rowid, the rowid if only one row was inserted,
           or a list of row ids if multiple rows were inserted.
        """
        if self._last is not None:
            return self._last
        with self._statement.getGeneratedKeys() as rs:
            if rs.isClosed():
                return self._last
            last = []
            while rs.next():
                last.append(rs.getLong(1))
            if len(last) == 0:
                return None
            if len(last) == 1:
                self._last = last[0]
                return last[0]
            self._last = last
            return last

    def setinputsizes(self, sizes):
        """ This can be used before a call to .execute*() to
        predefine memory areas for the operation's parameters.

        sizes is specified as a sequence — one item for each input parameter. The
        item should be a Type Object that corresponds to the input that will be
        used, or it should be an integer specifying the maximum length of a string
        parameter. If the item is None, then no predefined memory area will be
        reserved for that column (this is useful to avoid predefined areas for
        large inputs).

        This method would be used before the .execute*() method is invoked.

        (not implemented)
        """
        pass

    def setoutputsize(self, size, column=None):
        """
        Set a column buffer size for fetches of large columns (e.g. LONGs, BLOBs, etc.).

        The column is specified as an index into the result sequence. Not
        specifying the column will set the default size for all large columns
        in the cursor.

        (not implemented)
        """
        pass

    def __del__(self):
        try:
            self._close()
        except Exception:  # pragma: no cover
            pass

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        self._close()

###############################################################################
# Factories


def Date(year, month, day):
    """ This function constructs an object holding a date value. """
    return _jpype.JClass('java.sql.Date')(year - 1900, month - 1, day)


def Time(hour, minute, second):
    """ This function constructs an object holding a time value. """
    return _jpype.JClass('java.sql.Time')(hour, minute, second)


def Timestamp(year, month, day, hour, minute, second, nano=0):
    """ This function constructs an object holding a time stamp value. """
    return _jpype.JClass('java.sql.Timestamp')(year - 1900, month - 1, day, hour, minute, second, nano)


def DateFromTicks(ticks):
    """
    This function constructs an object holding a date value from the given
    ticks value (number of seconds since the epoch; see the documentation of
    the standard Python time module for details).
    """
    return Date(*time.localtime(ticks)[:3])


def TimeFromTicks(ticks):
    """

    This function constructs an object holding a time value from the given
    ticks value (number of seconds since the epoch; see the documentation of
    the standard Python time module for details).
    """
    return Time(*time.localtime(ticks)[3:6])


def TimestampFromTicks(ticks):
    """
    This function constructs an object holding a time stamp value from the
    given ticks value (number of seconds since the epoch; see the documentation
    of the standard Python time module for details).
    """
    return Timestamp(*time.localtime(ticks)[:6])


def Binary(data):
    """
    This function constructs an object capable of holding a binary (long)
    string value.
    """
    return _jtypes.JArray(_jtypes.JByte)(data)


#  SQL NULL values are represented by the Python None singleton on input and output.
_accepted = set(["exact", "implicit"])


def _populateTypes():
    global _SQLException, _SQLTimeoutException
    _SQLException = _jpype.JClass("java.sql.SQLException")
    _SQLTimeoutException = _jpype.JClass("java.sql.SQLTimeoutException")
    cs = _jpype.JClass("java.sql.CallableStatement")
    ps = _jpype.JClass("java.sql.PreparedStatement")
    rs = _jpype.JClass("java.sql.ResultSet")
    for v in _types:
        v._initialize(cs, ps, rs)

    java = _jpype._JPackage("java")

    byteArray = _jpype.JArray(_jtypes.JByte)
    _default_setters[java.lang.String] = STRING
    _default_setters[java.sql.Date] = DATE
    _default_setters[java.sql.Time] = TIME
    _default_setters[java.sql.Timestamp] = TIMESTAMP
    _default_setters[byteArray] = BINARY
    _default_setters[java.math.BigDecimal] = DECIMAL
    _default_setters[_jtypes.JFloat] = REAL
    _default_setters[_jtypes.JDouble] = DOUBLE
    _default_setters[_jtypes.JBoolean] = BOOLEAN
    _default_setters[_jtypes.JShort] = INTEGER
    _default_setters[_jtypes.JInt] = INTEGER
    _default_setters[_jtypes.JLong] = BIGINT
    _default_setters[bool] = BOOLEAN
    _default_setters[int] = BIGINT
    _default_setters[float] = DOUBLE
    _default_setters[str] = STRING
    _default_setters[memoryview] = BINARY
    _default_setters[bytes] = BINARY
    _default_setters[bytearray] = BINARY
    _default_setters[type(None)] = OBJECT
    _default_setters[java.sql.Clob] = CLOB
    _default_setters[java.sql.Blob] = BLOB
    _default_setters[datetime.datetime] = TIMESTAMP
    _default_setters[datetime.date] = DATE
    _default_setters[datetime.time] = TIME

    _default_converters[java.lang.String] = str
    _default_converters[java.sql.Date] = _asPython
    _default_converters[java.sql.Time] = _asPython
    _default_converters[java.sql.Timestamp] = _asPython
    _default_converters[java.math.BigDecimal] = _asPython
    _default_converters[byteArray] = bytes
    _default_converters[type(None)] = _nop
    # Adaptors can be installed after the JVM is started
    # JByteArray = _jpype.JArray(_jtypes.JByte)
    # VARCHAR.adapters[memoryview] = JByteArray


_jcustomizer.getClassHints("java.sql.SQLException").registerClassBase(Error)
_jinit.registerJVMInitializer(_populateTypes)
