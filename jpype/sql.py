from _jpype import JClass
from . import _jinit
from . import types as _jtypes
import typing
import _jpype
import time
import threading

# This a generic implementation of PEP-249
__all__ = ['BINARY', 'Binary', 'Connection', 'Cursor', 'DATE', 'DATETIME',
           'DBAPITypeObject', 'DECIMAL', 'DataError', 'DatabaseError', 'Date',
           'DateFromTicks', 'Error', 'FLOAT', 'IntegrityError', 'InterfaceError',
           'InternalError', 'NUMBER', 'NotSupportedError', 'OperationalError',
           'ProgrammingError', 'ROWID', 'STRING', 'TEXT', 'TIME', 'Time',
           'TimeFromTicks', 'Timestamp', 'TimestampFromTicks', 'Warning',
           '__builtins__', '__cached__', '__doc__', '__file__', '__loader__',
           '__name__', '__package__', '__spec__', 'apilevel', 'connect',
           'paramstyle', 'threadsafely']

apilevel = "2.0"
threadsafety = 1
paramstyle = 'qmark'

# For compatiblity with sqlite (not implemented)
PARSE_DECLTYPES = None

_SQLException = None
_SQLTimeoutException = None
_conversionTable = {}


def connect(url, driver=None, **kwargs):
    if driver:
        JClass('java.lang.Class').forName(driver).newInstance()
    connection = JClass('java.sql.DriverManager').getConnection(url)
    return Connection(connection)


class Warning(Exception):
    pass


class Error(Exception):
    pass


class InterfaceError(Error):
    pass


class DatabaseError(Error):
    pass


class DataError(DatabaseError):
    pass


class OperationalError(DatabaseError):
    pass


class IntegrityError(DatabaseError):
    pass


class InternalError(DatabaseError):
    pass


class ProgrammingError(DatabaseError):
    pass


class NotSupportedError(DatabaseError):
    pass


class Connection:
    def __init__(self, jconnection):
        self._jconnection = jconnection
        self._thread = threading.get_ident()

    def __del__(self):
        if not _jpype.isStarted():
            return
        if self._jconnection.isClosed():
            self.close()
        # FIXME hande the case in which the JVM was terminated before __del__

    def close(self):
        """ Close the connection now (rather than whenever .__del__() is called).

        The connection will be unusable from this point forward; an Error (or
        subclass) exception will be raised if any operation is attempted with
        the connection. The same applies to all cursor objects trying to use
        the connection. Note that closing a connection without committing the
        changes first will cause an implicit rollback to be performed.  """
        self._validate()
        self._jconnection.close()

    def commit(self):
        """Commit any pending transaction to the database.
        """
        self._validate()
        self._jconnection.commit()

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
        pass

    def cursor(self):
        """ Return a new Cursor Object using the connection. """
        self._validate()
        return Cursor(self)

    def _validate(self):
        if self._jconnection.isClosed() or threading.get_ident() != self._thread:
            raise ProgrammingError

    def __call__(self):
        self._validate()


class Cursor:

    def __init__(self, connection):
        if not isinstance(connection, Connection):
            raise TypeError
        self._converters = {}
        self._connection = connection._jconnection
        self._resultSet = None
        self._preparedStatement = None
        self._rowcount = -1
        self._converters = {}
        self._arraysize = 1
        self._description = None
        self._closed = False
        self._thread = threading.get_ident()

    @property
    def description(self):
        """
        This read-only attribute is a sequence of 7-item sequences.

        Each of these sequences contains information describing one result column:

        - name
        - type_code
        - display_size
        - internal_size
        - precision
        - scale
        - null_ok

        The first two items (name and type_code) are mandatory, the other five
        are optional and are set to None if no meaningful values can be
        provided.  """
        if self._description is not None:
            return self._description
        desc = []
        self._fetchColumns()
        for i in range(1, self._columns + 1):
            desc = (self._resultMetaData.getColumnName(i),
                    self._resultMetaData.getColumnTypeName(i))
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

    def callproc(self, procname, *args):
        """
        (This method is optional since not all databases provide stored procedures.)

        Call a stored database procedure with the given name. The sequence of
        parameters must contain one entry for each argument that the procedure
        expects. The result of the call is returned as modified copy of the
        input sequence. Input parameters are left untouched, output and
        input/output parameters replaced with possibly new values.

        The procedure may also provide a result set as output. This must then
        be made available through the standard .fetch*() methods.  """
        pass

    def close(self):
        """
        Close the cursor now (rather than whenever __del__ is called).

        The cursor will be unusable from this point forward; an Error (or
        subclass) exception will be raised if any operation is attempted with
        the cursor.  """
        self._validate()
        self._finish()
        self._closed = True

    def _validate(self):
        if self._closed or self._connection.isClosed() or threading.get_ident() != self._thread:
            raise ProgrammingError()

    def _finish(self):
        if self._resultSet is not None:
            self._resultSet.close()
            self._resultSet = None
        self._resultColumns = None
        self._paramColumns = None
        self._rowcount = -1
        self._preparedStatement = None
        self._resultMetaData = None
        self._paramMetaData = None
        self._description = None

    def _fetchParams(self):
        paramsMetaData = self._preparedStatement.getParameterMetaData()
        self._paramColumns = []
        for i in range(1, paramsMetaData.getParameterCount() + 1):
            param = paramsMetaData.getParameterClassName(i)
            self._paramColumns.append(_conversionTable[param])

    def _setParams(self, params):
        self._fetchParams()
        if len(self._paramColumns) != len(params):
            raise Error("incorrect number of parameters")

        if isinstance(params, typing.sequence):
            for i in range(0, len(params)):
                self._preparedStatement.setObject(i + 1, params[i])
        elif isinstance(params, typing.mapping):
            # FIXME we need the column names here
            raise RuntimeError()

    def execute(self, operation, params=None):
        """
        Prepare and execute a database operation (query or command).

        Parameters may be provided as sequence or mapping and will be bound to
        variables in the operation. Variables are specified in a
        database-specific notation (see the module's paramstyle attribute for
        details). [5]

        A reference to the operation will be retained by the cursor. If the
        same operation object is passed in again, then the cursor can optimize
        its behavior. This is most effective for algorithms where the same
        operation is used, but different parameters are bound to it (many
        times).

        For maximum efficiency when reusing an operation, it is best to use the
        .setinputsizes() method to specify the parameter types and sizes ahead
        of time. It is legal for a parameter to not match the predefined
        information; the implementation should compensate, possibly with a loss
        of efficiency.

        The parameters may also be specified as list of tuples to e.g. insert
        multiple rows in a single operation, but this kind of usage is
        deprecated: .executemany() should be used instead.

        Return values are not defined.
        """
        self._validate()
        if params is None:
            params = ()
        if not isinstance(params, (typing.Sequence, typing.Mapping)):
            raise ValueError("parameters are of unsupported type")
        # complete the previous operation
        self._finish()
        try:
            self._preparedStatement = self._connection.prepareStatement(operation)
        except JClass("java.sql.SQLException") as ex:
            raise OperationalError from ex
        except TypeError as ex:
            raise ValueError from ex
        try:
            self._paramMetaData = self._preparedStatement.getParameterMetaData()
        except TypeError:
            raise Error()
        self._setParams(params)
        try:
            if self._preparedStatement.execute():
                self._resultSet = self._preparedStatement.getResultSet()
                self._resultMetaData = self._resultSet.getMetaData()
            else:
                self._rowcount = self._preparedStatement.getUpdateCount()
        except _SQLException:
            pass

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
            self._preparedStatement = self._connection.prepareStatement(operation)
        except TypeError as ex:
            raise ValueError from ex
        try:
            self._paramMetaData = self._preparedStatement.getParameterMetaData()
        except TypeError:
            raise Error()
        self._rowcount = 0

        if isinstance(seq_of_parameters, typing.Iterable):
            for params in seq_of_parameters:
                try:
                    self._execute(params)
                except _SQLException:
                    break
        elif hasattr(seq_of_parameters, '__next__'):
            while True:
                try:
                    params = next(seq_of_parameters)
                    self._execute(params)
                except _SQLException:
                    break
                except StopIteration:
                    break

        else:
            raise TypeError("'%s' is not supported" % str(type(seq_of_parameters)))

    def _execute(self, params):
        self._setParams(params)
        if self._preparedStatement.execute():
            self._resultSet = self._preparedStatement.getResultSet()
            self._resultMetaData = self._resultSet.getMetaData()
        else:
            self._rowcount += self._preparedStatement.getUpdateCount()

    def executescript(self, params):
        # FIXME not in spec
        self._validate()
        pass

    def _fetchColumns(self):
        self._validate()
        if self._resultColumns is not None:
            return self._resultColumns
        self._resultColumns = []
        for i in range(0, self._resultMetaData.getColumnCount()):
            result = self._resultMetaData.getColumnClassName(i + 1)
            self._resultColumns.append(_conversionTable[result])
        return self._resultColumns

    def _fetchRow(self):
        row = []
        for index in range(1, len(self._resultColumns)):
            row.append(self._resultColumns[i].fetch(self._resultSet))
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
            raise Error()
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
        """(This method is optional since not all databases support
        multiple result sets.)

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
        This read/write attribute specifies the number of rows to fetch
        at a time with .fetchmany(). It defaults to 1 meaning to fetch a single row
        at a time.

        Implementations must observe this value with respect to the .fetchmany()
        method, but are free to interact with the database a single row at a time.
        It may also be used in the implementation of .executemany().
        """
        return self._arraysize

    @arraysize.setter
    def arraysize(self, sz):
        self._arraysize = sz

    @property
    def lastrowid(self):
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

        Implementations are free to have this method do nothing and users are free
        to not use it.
        """
        pass

    def setoutputsize(self, size, column=None):
        """
        Set a column buffer size for fetches of
        large columns (e.g. LONGs, BLOBs, etc.). The column is specified as an
        index into the result sequence. Not specifying the column will set the
        default size for all large columns in the cursor.

        This method would be used before the .execute*() method is invoked.

        Implementations are free to have this method do nothing and users are free
        to not use it.
        """
        pass

    def __iter__(self):
        self._validate()
        if not self._resultSet:
            raise Error()
        # Set a fetch size
        self._fetchColumns()
        while self._resultSet.next():
            yield self._fetchRow()


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

##################
# I honestly have no clue what these are supposed to do.
# They don't even appear in sqlite3 dbapi2 interface??


class DBAPITypeObject:
    def __init__(self, *values):
        self.values = values

    def __cmp__(self, other):
        if other in self.values:
            return 0
        if other < self.values:
            return 1
        else:
            return -1


STRING = DBAPITypeObject('CHAR', 'NCHAR', 'NVARCHAR', 'VARCHAR', 'OTHER')

TEXT = DBAPITypeObject('CLOB', 'LONGVARCHAR',
                       'LONGNVARCHAR', 'NCLOB', 'SQLXML')

BINARY = DBAPITypeObject('BINARY', 'BLOB', 'LONGVARBINARY', 'VARBINARY')

NUMBER = DBAPITypeObject('BOOLEAN', 'BIGINT', 'BIT', 'INTEGER', 'SMALLINT',
                         'TINYINT')

FLOAT = DBAPITypeObject('FLOAT', 'REAL', 'DOUBLE')

DECIMAL = DBAPITypeObject('DECIMAL', 'NUMERIC')

DATE = DBAPITypeObject('DATE')

TIME = DBAPITypeObject('TIME')

DATETIME = DBAPITypeObject('TIMESTAMP')

ROWID = DBAPITypeObject('ROWID')


class _StringConverter:
    def get(self, rs, column):
        pass

    def set(self, st, column, value):
        pass


def _populateTypes():
    global _SQLException, _SQLTimeoutException
    _SQLException = JClass("java.sql.SQLException")
    _SQLTimeoutException = JClass("java.sql.SQLTimeoutException")
    _conversionTable["java.lang.String"] = _StringConverter()
    _conversionTable["java.lang.Object"] = _StringConverter()


_jinit.registerJVMInitializer(_populateTypes)
