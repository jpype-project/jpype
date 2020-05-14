from _jpype import JClass
from . import _jinit
from . import _jcustomizer
from . import types as _jtypes
import typing
import _jpype
import time
import threading
import decimal

# This a generic implementation of PEP-249
__all__ = ['Binary', 'Connection', 'Cursor', 'DBAPITypeObject', 'DataError',
           'DatabaseError', 'Date', 'DateFromTicks', 'Error', 'IntegrityError',
           'InterfaceError', 'InternalError', 'NotSupportedError',
           'OperationalError', 'ProgrammingError', 'Time', 'TimeFromTicks',
           'Timestamp', 'TimestampFromTicks', 'Warning', '__builtins__',
           '__cached__', '__doc__', '__file__', '__loader__', '__name__',
           '__package__', '__spec__', 'apilevel', 'connect', 'paramstyle',
           'threadsafety']

apilevel = "2.0"
threadsafety = 1
paramstyle = 'qmark'

# For compatiblity with sqlite (not implemented)
PARSE_DECLTYPES = 1
PARSE_COLNAMES = 2

_SQLException = None
_SQLTimeoutException = None
_registry = {}


###############################################################################
# Types

class JDBCType:
    def __init__(self, name, code, native, getter=None, setter=None):
        """ (internal) Create a new JDBC type. """
        if isinstance(name, (str, type(None))):
            self._name = name
            self._values = [name]
        else:
            self._name = name[0]
            self._values = name
        self._code = code
        self._native = native
        self._getter = getter
        self._setter = setter
        self._adapters = {}
        self.converter = None
        if code is not None:
            _registry[code] = self

    def _initialize(self, ps, rs):
        """ Called after the JVM starts initialize Java resources """
        self._type = _jpype.JClass(self._native)
        if self._getter is not None:
            self._rsget = getattr(rs, self._getter)
        else:
            self._rsget = getattr(rs, "getObject")
        if self._setter is not None:
            self._psset = getattr(ps, self._setter)
        else:
            self._psset = getattr(ps, "setObject")

    def fetch(self, rs, column):
        """ A method to retrieve a specific JDBC type.

        To use a getter add the fetch method to the JDBC type matching the
        column type to be pulled.  For example, to set the getter for FLOAT to
        use the OBJECT getter, use  ``cx.getter[FLOAT] = OBJECT.fetch``.

        Not all getters are available on all database drivers.  Consult the
        database driver documentation for details.
        """
        try:
            return self._rsget(rs, column)
        except _SQLException as ex:
            raise InterfaceError("Unable to get '%s' using '%s'" % (self._name, self._getter)) from ex

    def set(self, ps, column, value):
        """ A method used to set a parameter to a query.

        To use a setter place the set method in the setter map corresponding.
        For example, if the database supports Blob types, the default handler
        for BLOB can be changed from OBJECT to BLOB with 
        ``cx.setter[BLOB] = BLOB.set``.

        Not all setters are available on all database drivers.  Consult the
        database driver documentation for details.
        """
        # Apply the adapter
        adp = self._adapters.get(type(value), None)
        if adp is not None:
            value = adp(value)
        if self._native._canConvertToJava(value) in _accepted:
            return self._psset(ps, column, value)
        try:
            return ps.setObject(column, value)
        except TypeError as ex:
            raise InterfaceError("Unable to convert '%s' into '%s'" % (type(value).__name__, self._name)) from ex

    def __str__(self):
        return self._name

    def __repr__(self):
        return self._name

    def __eq__(self, other):
        return other in self._values

    def __hash__(self):
        return hash(self._name)

    @property
    def adapters(self):
        """ Adapters are used to take a Python type and convert to the required
        Java native type corresponding JDBC type.
        """
        return self._adapters


# From https://www.cis.upenn.edu/~bcpierce/courses/629/jdkdocs/guide/jdbc/getstart/mapping.doc.html
# DATALINK = JDBCType('DATALINK',70)
# DISTINCT= JDBCType('DISTINCT',2001)
# REF_CURSOR = JDBCType('REF_CURSOR',2012)
# STRUCT = JDBCType('STRUCT',2002)
ARRAY = JDBCType('ARRAY', 2003, 'java.sql.Array', 'getArray', 'setArray')
BIGINT = JDBCType('BIGINT', -5, _jtypes.JLong, 'getLong', 'setLong')
BIT = JDBCType('BIT', -7, _jtypes.JBoolean, 'getBoolean', 'setBoolean')
BLOB = JDBCType('BLOB', 2004, 'java.sql.Blob', 'getBlob', 'setBlob')
BOOLEAN = JDBCType('BOOLEAN', 16, _jtypes.JBoolean, 'getBoolean', 'setBoolean')
CHAR = JDBCType('CHAR', 1, 'java.lang.String', 'getString', 'setString')
CLOB = JDBCType('CLOB', 2005, 'java.sql.Clob', 'getClob', 'setClob')
DATE = JDBCType('DATE', 91, 'java.sql.Date', 'getDate', 'setDate')
DOUBLE = JDBCType('DOUBLE', 8, _jtypes.JDouble, 'getDouble', 'setDouble')
INTEGER = JDBCType('INTEGER', 4, _jtypes.JInt, 'getInt', 'setInt')
OBJECT = JDBCType('OBJECT', 2000, 'java.lang.Object')
LONGNVARCHAR = JDBCType('LONGNVARCHAR', -16, 'java.lang.String', 'getString', 'setString')
LONGVARBINARY = JDBCType('LONGVARBINARY', -4, 'byte[]', 'getBytes', 'setBytes')
LONGVARCHAR = JDBCType('LONGVARCHAR', -1, 'java.lang.String', 'getString', 'setString')
NCHAR = JDBCType('NCHAR', -15, 'java.lang.String', 'getString', 'setString')
NCLOB = JDBCType('NCLOB', 2011, 'java.sql.NClob', 'getNClob', 'setNClob')
NULL = JDBCType('NULL', 0, 'java.lang.Object')
NUMERIC = JDBCType('NUMERIC', 2, 'java.math.BigDecimal', 'getBigDecimal', 'setBigDecimal')
NVARCHAR = JDBCType('NVARCHAR', -9, 'java.sql.Clob', 'getClob', 'setClob')
OTHER = JDBCType('OTHER', 1111, 'java.lang.Object')
REAL = JDBCType('REAL', 7, _jtypes.JFloat, 'getFloat', 'setFloat')
REF = JDBCType('REF', 2006, 'java.sql.Ref', 'getRef', 'setRef')
ROWID = JDBCType('ROWID', -8, 'java.sql.RowId', 'getRowId', 'setRowId')
RESULTSET = JDBCType('RESULTSET', -10, 'java.sql.ResultSet', 'getObject', 'setObject')
SMALLINT = JDBCType('SMALLINT', 5, _jtypes.JShort, 'getShort', 'setShort')
SQLXML = JDBCType('SQLXML', 2009, 'java.sql.SQLXML', 'getSQLXML', 'setSQLXML')
TIME = JDBCType('TIME', 92, 'java.sql.Time', 'getTime', 'setTime')
TIME_WITH_TIMEZONE = JDBCType('TIME_WITH_TIMEZONE', 2013, 'java.sql.Time', 'getTime', 'setTime')
TIMESTAMP = JDBCType('TIMESTAMP', 93, 'java.sql.Timestamp', 'getTimestamp', 'setTimestamp')
TIMESTAMP_WITH_TIMEZONE = JDBCType('TIMESTAMP_WITH_TIMEZONE', 2014, 'java.sql.Timestamp', 'getTimestamp', 'setTimestamp')
TINYINT = JDBCType('TINYINT', -6, _jtypes.JShort, 'getShort', 'setShort')
VARBINARY = JDBCType('VARBINARY', -3, 'byte[]', 'getBytes', 'setBytes')
VARCHAR = JDBCType('VARCHAR', 12, 'java.lang.String', 'getString', 'setString')

# Aliases required by DBAPI2
STRING = JDBCType(['STRING', 'CHAR', 'NCHAR', 'NVARCHAR', 'VARCHAR', 'OTHER'], None,
                  'java.lang.String', 'getString', 'setString')
TEXT = JDBCType(['TEXT', 'CLOB', 'LONGVARCHAR', 'LONGNVARCHAR', 'NCLOB', 'SQLXML'], None,
                'java.lang.String', 'getString', 'setString')
BINARY = JDBCType(['BINARY', 'BLOB', 'LONGVARBINARY', 'VARBINARY'], -2,
                  'byte[]', 'getBytes', 'setBytes')
NUMBER = JDBCType(['NUMBER', 'BOOLEAN', 'BIGINT', 'BIT', 'INTEGER', 'SMALLINT', 'TINYINT'], None,
                  'java.lang.Number', 'getObject', 'setObject')
FLOAT = JDBCType(['FLOAT', 'REAL', 'DOUBLE'], 6,
                 _jtypes.JDouble, 'getDouble', 'setDouble')
DECIMAL = JDBCType(['DECIMAL', 'NUMERIC'], 3,
                   'java.math.BigDecimal', 'getBigDecimal', 'setBigDecimal')
DATETIME = TIMESTAMP

# Special types
ASCII_STREAM = JDBCType(None, None, 'java.io.OutputStream', 'getAsciiStream', 'setAsciiStream')
BINARY_STREAM = JDBCType(None, None, 'java.io.OutputStream', 'getBinaryStream', 'setBinaryStream')
CHARACTER_STREAM = JDBCType(None, None, 'java.io.OutputStream', 'getCharacterStream', 'setCharacterStream')
NCHARACTER_STREAM = JDBCType(None, None, 'java.io.OutputStream', 'getNCharacterStream', 'setNCharacterStream')
URL = JDBCType(None, None, 'java.net.URL', 'getURL', 'setURL')

# =======================================================================================================
# Group    JDBC Type                Default Getter      Default Setter PyTypes           Special Getter |
# -------- ------------------------ ------------------- -------------- ----------------- ---------------|
# DATE     DATE                     getDate             setDate        datetime.datetime                |
# DATETIME TIMESTAMP                getTimestamp        setTimestamp   datetime.datetime                |
# TIME     TIME                     getTime             setTime        datetime.datetime                |
# -------- ------------------------ ------------------- -------------- ----------------- ---------------|
# DECIMAL  DECIMAL                  getBigDecimal       setBigDecimal  decimal.Decimal                  |
# DECIMAL  NUMERIC                  getBigDecimal       setBigDecimal  decimal.Decimal                  |
# -------- ------------------------ ------------------- -------------- ----------------- ---------------|
# FLOAT    FLOAT                    getDouble           setDouble      float                            |
# FLOAT    DOUBLE                   getDouble           getDouble      float                            |
# FLOAT    REAL                     getFloat            setFloat       float                            |
# -------- ------------------------ ------------------- -------------- ----------------- ------------   |
# NUMBER   BOOLEAN                  getBoolean          setBoolean     bool                             |
# NUMBER   BIT                      getBoolean          setBoolean     bool                             |
# NUMBER   TINYINT  (0..255)        getShort            setShort       int                              |
# NUMBER   SMALLINT (-2^15..2^15)   getShort            getShort       int                              |
# NUMBER   INTEGER  (-2^31..2^31)   getInt              getInt         int                              |
# NUMBER   BIGINT   (-2^63..2^63)   getLong             getLong        int                              |
# -------- ------------------------ ------------------- -------------- ----------------- ------------   |
# BINARY   BINARY                   getBytes            setBytes       bytes                            |
# BINARY   BLOB                     getBytes            setBytes       byte              getBlob        |
# BINARY   LONGVARBINARY            getBytes            setBytes       bytes                            |
# BINARY   VARBINARY                getBytes            setBytes       bytes                            |
# -------- ------------------------ ------------------- -------------- ----------------- ------------   |
# TEXT     CLOB                     getString           setString      str               getClob        |
# TEXT     LONGNVARCHAR             getString           setString      str                              |
# TEXT     LONGVARCHAR              getString           setString      str                              |
# TEXT     NCLOB                    getString           setString      str               getNClob       |
# TEXT     SQLXML                   getString           setString      str               getSQLXML      |
# -------- ------------------------ ------------------- -------------- ----------------- ------------   |
# STRING   NVARCHAR                 getString           setString      str                              |
# STRING   CHAR                     getString           setString      str                              |
# STRING   NCHAR                    getString           setString      str                              |
# STRING   VARCHAR                  getString           setString      str                              |
# -------- ------------------------ ------------------- -------------- ----------------- ------------   |
#          ARRAY                    getObject                                            getArray       |
#          OBJECT                   getObject                                            getObject      |
#          NULL                     getObject                                            getObject      |
#          REF                      getObject                                            getRef         |
#          ROWID                    getObject                                            getRowId       |
#          RESULTSET                getObject                                            getObject      |
#          TIME_WITH_TIMEZONE       getObject                                            getTime        |
#          TIMESTAMP_WITH_TIMEZONE  getObject                                            getTimeStamp   |
# -------- ------------------------ ------------------- -------------- ----------------- ---------------|
#    *     ASCII_STREAM             getAsciiStream                                                      |
#    *     BINARY_STREAM            getBinaryStream                                                     |
#    *     CHARACTER_STREAM         getCharacterStream                                                  |
#    *     NCHARACTER_STREAM        getNCharacterStream                                                 |
#    *     URL                      getURL                                                              |
# ======================================================================================================


def _fromDate(x):
    raise RuntimeError("not supported")


def _fromTime(x):
    raise RuntimeError("not supported")


def _fromTimestamp(x):
    raise RuntimeError("not supported")


def _fromBig(x):
    raise RuntimeError("not supported")


_default_getters = {ARRAY: OBJECT.fetch, OBJECT: OBJECT.fetch, NULL: OBJECT.fetch,
                    REF: OBJECT.fetch, ROWID: OBJECT.fetch, RESULTSET: OBJECT.fetch,
                    TIME_WITH_TIMEZONE: OBJECT.fetch, TIMESTAMP_WITH_TIMEZONE: OBJECT.fetch,
                    NVARCHAR: STRING.fetch, CHAR: STRING.fetch,
                    NCHAR: STRING.fetch, VARCHAR: STRING.fetch, BINARY: BINARY.fetch,
                    BLOB: BINARY.fetch, LONGVARBINARY: BINARY.fetch, VARBINARY: BINARY.fetch,
                    NUMBER: NUMBER.fetch, BOOLEAN: BOOLEAN.fetch, BIGINT: BIGINT.fetch,
                    BIT: BIT.fetch, INTEGER: INTEGER.fetch, SMALLINT: SMALLINT.fetch,
                    TINYINT: TINYINT.fetch, FLOAT: FLOAT.fetch, REAL: REAL.fetch,
                    DECIMAL: DECIMAL.fetch, NUMERIC: NUMERIC.fetch,
                    DATE: DATE.fetch, TIMESTAMP: TIMESTAMP.fetch, TIME: TIME.fetch,
                    }

_default_converters = {
    CLOB: str, LONGNVARCHAR: str, LONGVARCHAR: str, NCLOB: str,
    SQLXML: str, NVARCHAR: str, CHAR: str, NCHAR: str,
    VARCHAR: str, FLOAT: float, DOUBLE: float, REAL: float,
    BOOLEAN: int, BIT: int, TINYINT: int, SMALLINT: int,
    INTEGER: int, BIGINT: int, DATE: _fromDate, TIMESTAMP: _fromTime,
    TIME: _fromTimestamp, DECIMAL: _fromBig, NUMERIC: _fromBig,
}

_default_adapters = {}

_default_setters = {ARRAY: OBJECT.set, OBJECT: OBJECT.set, NULL: OBJECT.set,
                    REF: OBJECT.set, ROWID: OBJECT.set, RESULTSET: OBJECT.set,
                    TIME_WITH_TIMEZONE: OBJECT.set, TIMESTAMP_WITH_TIMEZONE: OBJECT.set,
                    NVARCHAR: STRING.set, CHAR: STRING.set,
                    NCHAR: STRING.set, VARCHAR: STRING.set, BINARY: BINARY.set,
                    BLOB: BINARY.set, LONGVARBINARY: BINARY.set, VARBINARY: BINARY.set,
                    NUMBER: NUMBER.set, BOOLEAN: BOOLEAN.set, BIGINT: BIGINT.set,
                    BIT: BIT.set, INTEGER: INTEGER.set, SMALLINT: SMALLINT.set,
                    TINYINT: TINYINT.set, FLOAT: FLOAT.set, REAL: REAL.set,
                    DECIMAL: DECIMAL.set, NUMERIC: NUMERIC.set,
                    DATE: DATE.set, TIMESTAMP: TIMESTAMP.set, TIME: TIME.set,
                    }

VARCHAR.adapters[memoryview] = lambda x: _jpype.JArray(_jtypes.JByte)(x)

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


###############################################################################
# Connection

def connect(url, driver=None, driver_args=None, detect_types=None,
            adapters=None, converters=None, getters=None, setters=None,
            **kwargs):
    """ Create a connection to a database.

    Arguments to the driver depend on the database type.

    Args:
       url (str): The database connection string for JDBC.
       driver (str, optional): A JDBC driver to load.
       driver_args: Arguments to the driver.  This may either be a map,
          java.util.Properties.  If not supplied, kwargs are used as as the
          parameters for the JDBC connection.
       adapters (map, optional): A map from types to JDBC types.  It is
          generally better to add the adapter to JDBC type directly.  Connection
          adapters are only used if a type specific adapter is not found.
       converters (map, optional): A map from JDBC types to converter functions
          which convert JDBC types to result types.  Use an empty map to allow
          Java types to be returned.
       getters (map, optional): A map of JDBC types to functions that retrieve
          data from a result set.
       setters (map, optional): A map of JDBC types to functions that set
          parameters on queries.
       *kwargs: Arguments to the driver if not supplied as
          driver_args.

    Raises:
       Error if the connection cannot be established.

    Returns:
       A new connection if successful.
    """
    Properties = JClass("java.util.Properties")
    if driver:
        JClass('java.lang.Class').forName(driver).newInstance()
    DM = JClass('java.sql.DriverManager')

    # User is supplying Java properties
    if isinstance(driver_args, Properties):
        connection = DM.getConnection(url, driver_args)

    # User is supplying a mapping that can be converted Properties
    elif isinstance(driver_args, typing.Mapping):
        info = Properties()
        for k, v in driver_args.items():
            info.setProperty(k, v)
        connection = DM.getConnection(url, info)

    # User supplied nothing
    elif driver_args is None:
        connection = DM.getConnection(url)

    # Otherwise use the kwargs
    else:
        info = Properties()
        for k, v in kwargs.items():
            info.setProperty(k, v)
        connection = DM.getConnection(url, info)
    return Connection(connection, adapters, converters, getters, setters)


class Connection:
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

    def __init__(self, jconnection, adapters, converters, getters, setters):
        self._jconnection = jconnection
        # Required by PEP 249
        # https://www.python.org/dev/peps/pep-0249/#commit
        self._jconnection.setAutoCommit(False)
        self._closed = False
        self._batch = jconnection.getMetaData().supportsBatchUpdates()
        if adapters is None:
            self._adapters = _default_adapters
        if converters is None:
            self._converters = _default_converters
        if getters is None:
            self._getters = _default_getters
        if setters is None:
            self._setters = _default_setters

    @property
    def adapters(self):
        return self._adapters

    @adapters.setter
    def adapters(self, v):
        self._adapters = v

    @property
    def converters(self):
        return self._converters

    @converters.setter
    def adapters(self, v):
        self._converters = v

    @property
    def getters(self):
        return self._getters

    @getters.setter
    def getters(self, v):
        self._getters = v

    @property
    def setters(self):
        return self._setters

    @setters.setter
    def setters(self, v):
        self._adaptors = v

    def __setattr__(self, name, value):
        if not name.startswith("_"):
            raise AttributeError("'%s' cannot be set" % name)
        object.__setattr__(self, name, value)

    def _close(self):
        if self._closed or not _jpype.isStarted():
            return
        if not self._jconnection.isClosed():
            self._jconnection.close()
        self._closed = True

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        self._close()

    def __del__(self):
        try:
            self._close()
        except:
            pass

    def close(self):
        """ Close the connection now (rather than whenever .__del__() is called).

        The connection will be unusable from this point forward; an Error (or
        subclass) exception will be raised if any operation is attempted with
        the connection. The same applies to all cursor objects trying to use
        the connection. Note that closing a connection without committing the
        changes first will cause an implicit rollback to be performed.  """
        self._validate()
        self._close()

    def commit(self):
        """Commit any pending transaction to the database.
        """
        self._validate()
        if self._jconnection.getAutoCommit:
            raise NotSupportedError("Autocommit is enabled")
        try:
            self._jconnection.commit()
        except Exception as ex:
            self._handle(ex)

    def rollback(self):
        """Rollback the transaction.

        This method is optional since not all databases provide transaction
        support.

        In case a database does provide transactions this method causes the
        database to roll back to the start of any pending transaction. Closing
        a connection without committing the changes first will cause an
        implicit rollback to be performed.
        """
        self._validate()
        if self._jconnection.getAutoCommit:
            raise NotSupportedError("Autocommit is enabled")
        try:
            self._jconnection.rollback()
        except Exception as ex:
            self._handle(ex)

    def _handle(self, ex):
        raise ex

    def cursor(self):
        """ Return a new Cursor Object using the connection. """
        self._validate()
        return Cursor(self)

    def _validate(self):
        if self._jconnection.isClosed():
            raise ProgrammingError

    def __call__(self):
        self._validate()

    # Sqlite3 extensions
    def execute(self, sql, parameters=None):
        """ This is a nonstandard shortcut that creates an intermediate cursor
        object by calling the cursor method, then calls the cursor’s execute
        method with the parameters given.  """
        return self.cursor().execute(sql, parameters)

    def executemany(self, sql, parameters):
        """ This is a nonstandard shortcut that creates an intermediate cursor
        object by calling the cursor method, then calls the cursor’s
        executemany method with the parameters given.
        """
        return self.cursor().executemany(sql, parameters)

    @property
    def native(self):
        """ Get the JDBC connection that is backing this Python 
        Connection object.

        This can be used to retrieve additional metadata and other features
        that are outside of the scope of the DBAPI driver.
        """
        return self._jconnection

    @property
    def autocommit(self):
        """ bool: Property controlling autocommit behavior.

        By default connects are not autocommit.  Setting autocommit
        will result in commit and rollback producing a ProgrammingError.
        """
        self._validate()
        return self._jconnection.getAutoCommit()

    @autocommit.setter
    def autocommit(self, enabled):
        self._validate()
        self._jconnection.setAutoCommit(enabled)

    @property
    def type_info(self):
        """ list: The list of types that are supported by this driver.

        This is useful to find out the capabilities of the driver.
        """
        self._validate()
        out = {}
        metadata = self._jconnection.getMetaData()
        with metadata.getTypeInfo() as resultSet:
            while (resultSet.next()):
                try:
                    out[str(resultSet.getString("TYPE_NAME"))] = _registry[resultSet.getInt("DATA_TYPE")]
                except KeyError as ex:
                    raise RuntimeError from ex
        return out


###############################################################################
# Cursor

class Cursor:

    def __init__(self, connection):
        if not isinstance(connection, Connection):
            raise TypeError
        self._connection = connection
        self._jcx = connection._jconnection
        self._resultSet = None
        self._preparedStatement = None
        self._rowcount = -1
        self._arraysize = 1
        self._description = None
        self._closed = False
        self._thread = threading.get_ident()
        self._paramSetters = None
        self._resultFetchers = None

    def _close(self):
        if self._closed or not _jpype.isStarted():
            return
        self._finish()
        self._closed = True

    def _fetchParams(self):
        """ Look up the adapters to apply to incoming parameters. """
        cx = self.connection
        self._paramSetters = []
        meta = self._preparedStatement.getParameterMetaData()
        count = meta.getParameterCount()
        for i in range(count):
            # Lookup the JDBC Type
            jdbcType = _registry[meta.getParameterType(i + 1)]
            self._paramSetters.append(cx._setters[param])

    def _fetchColumns(self):
        """ Get the list of getters and converters that apply to
        the result set. """
        self._validate()
        if self._resultFetcher is not None:
            return
        cx = self.connection
        self._resultFetchers = []
        self._resultConverter = []
        meta = self._resultMetaData
        count = meta.getColumnCount()
        for i in range(count):
            # Lookup the JDBC Type
            jdbcType = _registry[meta.getColumnType(i + 1)]
            self._resultFetchers.append(cx._getters.get(jdbcType, OBJECT.fetch))
            self._resultConverter.append(cx._converters.get(jdbcType, _nop))
        return self._resultColumns
        # FIXME add support for position and named converters here

    def _setParams(self, params):
        if isinstance(params, typing.Sequence):
            self._fetchParams()
            if len(self._paramColumns) != len(params):
                raise ProgrammingError("incorrect number of parameters (%d!=%d)"
                                       % (len(self._paramColumns), len(params)))
            for i in range(len(params)):
                # FIXME apply cx adaptors here
                self._paramSetters[i](self._preparedStatement, i + 1, params[i])
        elif isinstance(params, typing.Mapping):
            raise TypeError("mapping parameters not supported")
        elif isinstance(params, typing.Iterable):
            self._fetchParams()
            try:
                for i in range(len(self._paramColumns)):
                    # FIXME apply cx adaptors here
                    self._paramColumns[i](self._preparedStatement, i + 1, next(params))
            except StopIteration:
                raise ProgrammingError("incorrect number of parameters (%d!=%d)"
                                       % (len(self._paramColumns), i))
        else:
            raise TypeError("'%s' parameters not supported" % (type(params).__name__))

    def _validate(self):
        """ Called before any method that requires the statement to be open. """
        if self._closed or self._jcx.isClosed() or threading.get_ident() != self._thread:
            raise ProgrammingError()

    def _finish(self):
        if self._resultSet is not None:
            self._resultSet.close()
            self._resultSet = None
        if self._preparedStatement is not None:
            self._preparedStatement.close()
            self._preparedStatement = None
        self._resultFetchers = None
        self._paramSetters = None
        self._rowcount = -1
        self._resultMetaData = None
        self._paramMetaData = None
        self._description = None

    @property
    def description(self):
        """
        Description is read-only attribute is a sequence of 7-item sequences.

        Each of these sequences contains information describing one result column:

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
        rmd = self._resultMetaData
        if rmd is None:
            return None
        for i in range(1, self._resultMetaData.getColumnCount() + 1):
            size = rmd.getColumnDisplaySize(i)
            desc.append((str(rmd.getColumnName(i)),
                         str(rmd.getColumnTypeName(i)),
                         size,
                         size,
                         rmd.getPrecision(i),
                         rmd.getScale(i),
                         rmd.isNullable(i),))
        self._description = desc
        return desc

    @property
    def rowcount(self):
        """ This read-only attribute specifies the number of rows that the last
        .execute*() produced (for DQL statements like SELECT) or affected (for
        DML statements like UPDATE or INSERT).

        The attribute is -1 in case no .execute*() has been performed on the
        cursor or the rowcount of the last operation is cannot be determined by
        the interface.
        """
        return self._rowcount

    def close(self):
        """
        Close the cursor now (rather than whenever __del__ is called).

        The cursor will be unusable from this point forward; an Error (or
        subclass) exception will be raised if any operation is attempted with
        the cursor.
        """
        self._validate()
        self._close()

    def execute(self, operation, params=None):
        """
        Prepare and execute a database operation (query or command).

        Parameters may be provided as sequence or mapping and will be bound to
        variables in the operation. Variables are specified in a qmark
        notation.
        """
        self._validate()
        if params is None:
            params = ()
        if not isinstance(params, (typing.Sequence, typing.Iterable, typing.Iterator)):
            raise TypeError("parameters are of unsupported type '%s'" % str(type(params)))
        # complete the previous operation
        self._finish()
        try:
            self._preparedStatement = self._jcx.prepareStatement(operation)
        except JClass("java.sql.SQLException") as ex:
            raise OperationalError from ex
        except TypeError as ex:
            raise ValueError from ex
        self._paramMetaData = self._preparedStatement.getParameterMetaData()
        if self._preparedStatement.execute():
            self._resultSet = self._preparedStatement.getResultSet()
            self._resultMetaData = self._resultSet.getMetaData()
        self._rowcount = self._preparedStatement.getUpdateCount()
        return self

    def _executeone(params):
        self._setParams(params)
        if self._preparedStatement.execute():
            self._resultSet = self._preparedStatement.getResultSet()
            self._resultMetaData = self._resultSet.getMetaData()
        self._rowcount = self._preparedStatement.getUpdateCount()
        return self._rowcount

    def executemany(self, operation, seq_of_parameters):
        """
        Prepare a database operation (query or command) and then execute it
        against all parameter sequences or mappings found in the sequence
        seq_of_parameters.

        Modules are free to implement this method using multiple calls to the
        .execute() method or by using array operations to have the database
        process the sequence as a whole in one call.

        Use of this method for an operation which produces one or more result sets
        constitutes undefined behavior, and the implementation is permitted (but
        not required) to raise an exception when it detects that a result set has
        been created by an invocation of the operation.

        The same comments as for .execute() also apply accordingly to this method.

        Return values are not defined.
        """
        self._validate()
        if seq_of_parameters is None:
            seq_of_parameters = ()
        # complete the previous operation
        self._finish()
        try:
            self._preparedStatement = self._jcx.prepareStatement(operation)
        except TypeError as ex:
            raise ValueError from ex
        except _SQLException as ex:
            raise ProgrammingError("Failed to prepare '%s'" % operation) from ex
        try:
            self._paramMetaData = self._preparedStatement.getParameterMetaData()
        except TypeError:
            raise Error()
        if self._connection._batch:
            return self._executeBatch(seq_of_parameters)
        else:
            return self._executeRepeat(seq_of_parameters)

    def _executeBatch(self, seq_of_parameters):
        if isinstance(seq_of_parameters, typing.Iterable):
            for params in seq_of_parameters:
                self._setParams(params)
                self._preparedStatement.addBatch()
        elif isinstance(seq_of_parameters, typing.Iterator):
            while True:
                try:
                    params = next(seq_of_parameters)
                    self._setParams(params)
                    self._preparedStatement.addBatch()
                except StopIteration:
                    break
        else:
            raise TypeError("'%s' is not supported" % str(type(seq_of_parameters)))
        try:
            counts = self._preparedStatement.executeBatch()
        except _SQLException as ex:
            raise ProgrammingError from ex
        self._rowcount = sum(counts)
        if self._rowcount < 0:
            self._rowcount = -1
        return self

    def _executeRepeat(self, seq_of_parameters):
        counts = []
        if isinstance(seq_of_parameters, typing.Iterable):
            for params in seq_of_parameters:
                counts.append(self._executeone(params))
        elif isinstance(seq_of_parameters, typing.Iterator):
            while True:
                try:
                    self._setParams(next(seq_of_parameters))
                    counts.append(self._executeone(params))
                except StopIteration:
                    break
        else:
            raise TypeError("'%s' is not supported" % str(type(seq_of_parameters)))
        try:
            counts = self._preparedStatement.executeBatch()
        except _SQLException as ex:
            raise ProgrammingError from ex
        self._rowcount = sum(counts)
        if self._rowcount < 0:
            self._rowcount = -1
        return self

    def _fetchRow(self):
        row = []
        for idx in range(len(self._resultColumns)):
            row.append(self._resultConverter(self._resultFetcher[idx](self._resultSet, idx + 1)))
        return row

    def fetchone(self):
        """
        Fetch the next row of a query result set, returning a single
        sequence, or None when no more data is available.

        An Error (or subclass) exception is raised if the previous call to
        .execute*() did not produce any result set or no call was issued yet.
        """
        self._validate()
        if not self._resultSet:
            raise Error
        if not self._resultSet.next():
            return None
        self._fetchColumns()
        return self._fetchRow()

    def fetchmany(self, size=None):
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
        .execute*() did not produce any result set or no call was issued yet.

        Note there are performance considerations involved with the size parameter.
        For optimal performance, it is usually best to use the .arraysize
        attribute. If the size parameter is used, then it is best for it to retain
        the same value from one .fetchmany() call to the next.
        """
        self._validate()
        if not self._resultSet:
            raise Error()
        if size is None:
            size = self._arraysize
        # Set a fetch size
        self._resultSet.setFetchSize(size)
        self._fetchColumns()
        rows = []
        for i in range(size):
            if not self._resultSet.next():
                break
            row = self._fetchRow()
            if row is None:
                break
            rows.append(row)
        # Restore the default fetch size
        self._resultSet.setFetchSize(0)
        return rows

    def fetchall(self):
        """ Fetch all (remaining) rows of a query result, returning them as
        a sequence of sequences (e.g. a list of tuples). Note that the cursor's
        arraysize attribute can affect the performance of this operation.

        An Error (or subclass) exception is raised if the previous call to
        .execute*() did not produce any result set or no call was issued yet.
        """
        self._validate()
        if not self._resultSet:
            raise Error()
        # Set a fetch size
        self._fetchColumns()
        rows = []
        while self._resultSet.next():
            row = self._fetchRow()
            if row is None:
                break
            rows.append(row)
        return rows

    def nextset(self):
        """ Get the next result set in this cursor.

        Not all databases support multiple result sets.

        This method will make the cursor skip to the next available set, discarding
        any remaining rows from the current set.

        If there are no more sets, the method returns None. Otherwise, it returns a
        true value and subsequent calls to the .fetch*() methods will return rows
        from the next result set.

        An Error (or subclass) exception is raised if the previous call to
        .execute*() did not produce any result set or no call was issued yet.
        """
        self._resultSet.close()
        if self._preparedStatement.getMoreResults():
            self._resultSet = self._prepareStatement.getResultSet()
            self._resultMetaData = self._resultSet.getMetaData()
            return True
        else:
            self._rowcount = self._preparedStatement.getUpdageCount()
            return None

    @property
    def arraysize(self):
        """
        Specify the number of rows to fetch with .fetchmany().

        This read/write attribute specifies the number of rows to fetch
        at a time with .fetchmany(). It defaults to 1 meaning to fetch a single row
        at a time.
        """
        return self._arraysize

    @arraysize.setter
    def arraysize(self, sz):
        self._arraysize = sz

    @property
    def lastrowid(self):
        """ Get the id of the last row inserted.

        This is not supported on all JDBC drivers.
        """
        rs = self._preparedStatement.getGeneratedKeys()
        rs.next()
        rowId = rs.getLong(1)
        rs.close()
        return rowId

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

    def __iter__(self):
        """ Extension to iterate through a cursor one record at a time.
        """
        self._validate()
        if not self._resultSet:
            raise Error()
        # Set a fetch size
        self._fetchColumns()
        while self._resultSet.next():
            yield self._fetchRow()

    def __del__(self):
        try:
            self._close()
        except:
            pass

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        self._close()

###############################################################################
# Factories


def Date(year, month, day):
    """ This function constructs an object holding a date value. """
    return JClass('java.sql.Date')(year, month, day)


def Time(hour, minute, second):
    """ This function constructs an object holding a time value. """
    return JClass('java.sql.Time')(hour, minute, second)


def Timestamp(year, month, day, hour, minute, second, nano=0):
    """ This function constructs an object holding a time stamp value. """
    return JClass('java.sql.Timestamp')(year, month, day, hour, minute, second, nano)


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


def _nop(x):
    return x


def _populateTypes():
    global _SQLException, _SQLTimeoutException
    _SQLException = JClass("java.sql.SQLException")
    _SQLTimeoutException = JClass("java.sql.SQLTimeoutException")
    ps = JClass("java.sql.PreparedStatement")
    rs = JClass("java.sql.ResultSet")
    for p, v in _registry.items():
        v._initialize(ps, rs)


_jcustomizer.getClassHints("java.sql.SQLException").registerClassBase(Error)
_jinit.registerJVMInitializer(_populateTypes)
