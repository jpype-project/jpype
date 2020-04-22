apilevel = "2.0"
threadsafely = 0
paramstyle = 'qmark'

def connect(jclassname, url, driver_args=None, jars=None, libs=None):
    pass

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
    def close():
        """ Close the connection now (rather than whenever .__del__() is called).
        
        The connection will be unusable from this point forward; an Error (or
        subclass) exception will be raised if any operation is attempted with
        the connection. The same applies to all cursor objects trying to use
        the connection. Note that closing a connection without committing the
        changes first will cause an implicit rollback to be performed.  """
        pass

    def commit():
        """Commit any pending transaction to the database.
        """
        pass

    def rollback():
        """Rollback the transaction.
        
        This method is optional since not all databases provide transaction
        support.

        In case a database does provide transactions this method causes the
        database to roll back to the start of any pending transaction. Closing
        a connection without committing the changes first will cause an
        implicit rollback to be performed.
        """
        pass

    def cursor():
        """ Return a new Cursor Object using the connection. """
        pass

class Cursor:
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
        pass

    @property
    def rowcount(self):
        """ This read-only attribute specifies the number of rows that the last
        .execute*() produced (for DQL statements like SELECT) or affected (for
        DML statements like UPDATE or INSERT).

        The attribute is -1 in case no .execute*() has been performed on the
        cursor or the rowcount of the last operation is cannot be determined by
        the interface.
        """

        pass

    def callproc(self, procname, *args):
        """
        (This method is optional since not all databases provide stored procedures. [3])

        Call a stored database procedure with the given name. The sequence of
        parameters must contain one entry for each argument that the procedure
        expects. The result of the call is returned as modified copy of the
        input sequence. Input parameters are left untouched, output and
        input/output parameters replaced with possibly new values.

        The procedure may also provide a result set as output. This must then
        be made available through the standard .fetch*() methods.  """
        pass

    def close():
        """
        Close the cursor now (rather than whenever __del__ is called).

        The cursor will be unusable from this point forward; an Error (or
        subclass) exception will be raised if any operation is attempted with
        the cursor.  """

    def execute(self, operation, *params):
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
        pass

    def execute(self, operation, *parameters):
        """
        Prepare and execute a database operation (query or command).

        Parameters may be provided as sequence or mapping and will be bound to
        variables in the operation. Variables are specified in a
        database-specific notation (see the module's paramstyle attribute for
        details).

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

        The parameters may also be specified as list of tuples to e.g.  insert
        multiple rows in a single operation, but this kind of usage is
        deprecated: .executemany() should be used instead.

        Return values are not defined.
        """
        pass

    def executemany(self, operation, *seq_of_parameters):
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

    def fetchone(self):
        """
        Fetch the next row of a query result set, returning a single
        sequence, or None when no more data is available. [6]

        An Error (or subclass) exception is raised if the previous call to
        .execute*() did not produce any result set or no call was issued yet.
        """
        pass


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
        pass

    def fetchall():
        """ Fetch all (remaining) rows of a query result, returning them as
        a sequence of sequences (e.g. a list of tuples). Note that the cursor's
        arraysize attribute can affect the performance of this operation.

        An Error (or subclass) exception is raised if the previous call to
        .execute*() did not produce any result set or no call was issued yet.
        """
        pass

    def nextset(self):
        """(This method is optional since not all databases support
        multiple result sets. [3])

        This method will make the cursor skip to the next available set, discarding
        any remaining rows from the current set.

        If there are no more sets, the method returns None. Otherwise, it returns a
        true value and subsequent calls to the .fetch*() methods will return rows
        from the next result set.

        An Error (or subclass) exception is raised if the previous call to
        .execute*() did not produce any result set or no call was issued yet.
        """
        pass



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
        pass

    @arraysize.setter
    def arraysize(self, sz):
        pass

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

def Date(year, month, day):
    """ This function constructs an object holding a date value. """
    pass

def Time(hour, minute, second):
    """ This function constructs an object holding a time value. """
    pass

def Timestamp(year, month, day, hour, minute, second):
    """ This function constructs an object holding a time stamp value. """
    pass

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

def Binary(string):
    """
    This function constructs an object capable of holding a binary (long)
    string value.
    """

#  SQL NULL values are represented by the Python None singleton on input and output.

class DBAPITypeObject:
    def __init__(self,*values):
        self.values = values
    def __cmp__(self,other):
        if other in self.values:
             return 0
        if other < self.values:
             return 1
        else:
             return -1


STRING = DBAPITypeObject('CHAR', 'NCHAR', 'NVARCHAR', 'VARCHAR', 'OTHER')

TEXT = DBAPITypeObject('CLOB', 'LONGVARCHAR', 'LONGNVARCHAR', 'NCLOB', 'SQLXML')

BINARY = DBAPITypeObject('BINARY', 'BLOB', 'LONGVARBINARY', 'VARBINARY')

NUMBER = DBAPITypeObject('BOOLEAN', 'BIGINT', 'BIT', 'INTEGER', 'SMALLINT',
                                 'TINYINT')

FLOAT = DBAPITypeObject('FLOAT', 'REAL', 'DOUBLE')

DECIMAL = DBAPITypeObject('DECIMAL', 'NUMERIC')

DATE = DBAPITypeObject('DATE')

TIME = DBAPITypeObject('TIME')

DATETIME = DBAPITypeObject('TIMESTAMP')

ROWID = DBAPITypeObject('ROWID')
    
