#-*- coding: iso-8859-1 -*-
# pysqlite2/test/dbapi.py: tests for DB-API compliance
#
# Copyright (C) 2004-2010 Gerhard Häring <gh@ghaering.de>
#
# This file is part of pysqlite.
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
#
# This is a modification of the CPython SQLite testbench for JPype.
# The original can be found in cpython/Lib/sqlite3/test
#
import sys
import _jpype
import jpype
from jpype.types import *
from jpype import JPackage, java
import common
import pytest
import threading
import unittest
import jpype.sql as dbapi2

from test.support import TESTFN, unlink

def getConnection():
    return "jdbc:sqlite::memory:"

class ModuleTests(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
 
    def testAPILevel(self):
        self.assertEqual(dbapi2.apilevel, "2.0",
                         "apilevel is %s, should be 2.0" % dbapi2.apilevel)

    def testThreadSafety(self):
        self.assertEqual(dbapi2.threadsafety, 1,
                         "threadsafety is %d, should be 1" % dbapi2.threadsafety)

    def testParamStyle(self):
        self.assertEqual(dbapi2.paramstyle, "qmark",
                         "paramstyle is '%s', should be 'qmark'" %
                         dbapi2.paramstyle)

    def testWarning(self):
        self.assertTrue(issubclass(dbapi2.Warning, Exception),
                     "Warning is not a subclass of Exception")

    def testError(self):
        self.assertTrue(issubclass(dbapi2.Error, Exception),
                        "Error is not a subclass of Exception")

    def testInterfaceError(self):
        self.assertTrue(issubclass(dbapi2.InterfaceError, dbapi2.Error),
                        "InterfaceError is not a subclass of Error")

    def testDatabaseError(self):
        self.assertTrue(issubclass(dbapi2.DatabaseError, dbapi2.Error),
                        "DatabaseError is not a subclass of Error")

    def testDataError(self):
        self.assertTrue(issubclass(dbapi2.DataError, dbapi2.DatabaseError),
                        "DataError is not a subclass of DatabaseError")

    def testOperationalError(self):
        self.assertTrue(issubclass(dbapi2.OperationalError, dbapi2.DatabaseError),
                        "OperationalError is not a subclass of DatabaseError")

    def testIntegrityError(self):
        self.assertTrue(issubclass(dbapi2.IntegrityError, dbapi2.DatabaseError),
                        "IntegrityError is not a subclass of DatabaseError")

    def testInternalError(self):
        self.assertTrue(issubclass(dbapi2.InternalError, dbapi2.DatabaseError),
                        "InternalError is not a subclass of DatabaseError")

    def testProgrammingError(self):
        self.assertTrue(issubclass(dbapi2.ProgrammingError, dbapi2.DatabaseError),
                        "ProgrammingError is not a subclass of DatabaseError")

    def testNotSupportedError(self):
        self.assertTrue(issubclass(dbapi2.NotSupportedError,
                                   dbapi2.DatabaseError),
                        "NotSupportedError is not a subclass of DatabaseError")

class ConnectionTests(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.cx = dbapi2.connect(getConnection())
        cu = self.cx.cursor()
        cu.execute("create table test(id integer primary key, name text)")
        cu.execute("insert into test(name) values (?)", ("foo",))

    def tearDown(self):
        self.cx.close()

    def testCommit(self):
        self.cx.commit()

    def testCommitAfterNoChanges(self):
        """
        A commit should also work when no changes were made to the database.
        """
        self.cx.commit()
        self.cx.commit()

    def testRollback(self):
        self.cx.rollback()

    def testRollbackAfterNoChanges(self):
        """
        A rollback should also work when no changes were made to the database.
        """
        self.cx.rollback()
        self.cx.rollback()

    def testCursor(self):
        cu = self.cx.cursor()

    def testFailedOpen(self):
        YOU_CANNOT_OPEN_THIS = "/foo/bar/bla/23534/mydb.db"
        with self.assertRaises(dbapi2.OperationalError):
            con = dbapi2.connect(YOU_CANNOT_OPEN_THIS)

    def testClose(self):
        self.cx.close()

    def testExceptions(self):
        # Optional DB-API extension.
        self.assertEqual(self.cx.Warning, dbapi2.Warning)
        self.assertEqual(self.cx.Error, dbapi2.Error)
        self.assertEqual(self.cx.InterfaceError, dbapi2.InterfaceError)
        self.assertEqual(self.cx.DatabaseError, dbapi2.DatabaseError)
        self.assertEqual(self.cx.DataError, dbapi2.DataError)
        self.assertEqual(self.cx.OperationalError, dbapi2.OperationalError)
        self.assertEqual(self.cx.IntegrityError, dbapi2.IntegrityError)
        self.assertEqual(self.cx.InternalError, dbapi2.InternalError)
        self.assertEqual(self.cx.ProgrammingError, dbapi2.ProgrammingError)
        self.assertEqual(self.cx.NotSupportedError, dbapi2.NotSupportedError)

    def testInTransaction(self):
        # Can't use db from setUp because we want to test initial state.
        cx = dbapi2.connect(getConnection())
        cu = cx.cursor()
        self.assertEqual(cx.in_transaction, False)
        cu.execute("create table transactiontest(id integer primary key, name text)")
        self.assertEqual(cx.in_transaction, False)
        cu.execute("insert into transactiontest(name) values (?)", ("foo",))
        self.assertEqual(cx.in_transaction, True)
        cu.execute("select name from transactiontest where name=?", ["foo"])
        row = cu.fetchone()
        self.assertEqual(cx.in_transaction, True)
        cx.commit()
        self.assertEqual(cx.in_transaction, False)
        cu.execute("select name from transactiontest where name=?", ["foo"])
        row = cu.fetchone()
        self.assertEqual(cx.in_transaction, False)

    def testInTransactionRO(self):
        with self.assertRaises(AttributeError):
            self.cx.in_transaction = True

    def testOpenWithPathLikeObject(self):
        """ Checks that we can successfully connect to a database using an object that
            is PathLike, i.e. has __fspath__(). """
        self.addCleanup(unlink, TESTFN)
        class Path:
            def __fspath__(self):
                return TESTFN
        path = Path()
        with dbapi2.connect(path) as cx:
            cx.execute('create table test(id integer)')

    def testOpenUri(self):
        if dbapi2.dbapi2_version_info < (3, 7, 7):
            with self.assertRaises(dbapi2.NotSupportedError):
                dbapi2.connect(getConnection(), uri=True)
            return
        self.addCleanup(unlink, TESTFN)
        with dbapi2.connect(TESTFN) as cx:
            cx.execute('create table test(id integer)')
        with dbapi2.connect('file:' + TESTFN, uri=True) as cx:
            cx.execute('insert into test(id) values(0)')
        with dbapi2.connect('file:' + TESTFN + '?mode=ro', uri=True) as cx:
            with self.assertRaises(dbapi2.OperationalError):
                cx.execute('insert into test(id) values(1)')

    def testSameThreadErrorOnOldVersion(self):
        with self.assertRaises(dbapi2.NotSupportedError) as cm:
            dbapi2.connect(getConnection(), check_same_thread=False)
        self.assertEqual(str(cm.exception), 'shared connections not available')


class CursorTests(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.cx = dbapi2.connect(getConnection())
        self.cu = self.cx.cursor()
        self.cu.execute(
            "create table test(id integer primary key, name text, "
            "income number, unique_test text unique)"
        )
        self.cu.execute("insert into test(name) values (?)", ("foo",))

    def tearDown(self):
        self.cu.close()
        self.cx.close()

    def testExecuteNoArgs(self):
        self.cu.execute("delete from test")

    def testExecuteIllegalSql(self):
        with self.assertRaises(dbapi2.OperationalError):
            self.cu.execute("select asdf")

    def testExecuteTooMuchSql(self):
        with self.assertRaises(dbapi2.Warning):
            self.cu.execute("select 5+4; select 4+5")

    def testExecuteTooMuchSql2(self):
        self.cu.execute("select 5+4; -- foo bar")

    def testExecuteTooMuchSql3(self):
        self.cu.execute("""
            select 5+4;

            /*
            foo
            */
            """)

    def testExecuteWrongSqlArg(self):
        with self.assertRaises(ValueError):
            self.cu.execute(42)

    def testExecuteArgInt(self):
        self.cu.execute("insert into test(id) values (?)", (42,))

    def testExecuteArgFloat(self):
        self.cu.execute("insert into test(income) values (?)", (2500.32,))

    def testExecuteArgString(self):
        self.cu.execute("insert into test(name) values (?)", ("Hugo",))

    def testExecuteArgStringWithZeroByte(self):
        self.cu.execute("insert into test(name) values (?)", ("Hu\x00go",))

        self.cu.execute("select name from test where id=?", (self.cu.lastrowid,))
        row = self.cu.fetchone()
        self.assertEqual(row[0], "Hu\x00go")

    def testExecuteNonIterable(self):
        with self.assertRaises(ValueError) as cm:
            self.cu.execute("insert into test(id) values (?)", 42)
        self.assertEqual(str(cm.exception), 'parameters are of unsupported type')

    def testExecuteWrongNoOfArgs1(self):
        # too many parameters
        with self.assertRaises(dbapi2.ProgrammingError):
            self.cu.execute("insert into test(id) values (?)", (17, "Egon"))

    def testExecuteWrongNoOfArgs2(self):
        # too little parameters
        with self.assertRaises(dbapi2.ProgrammingError):
            self.cu.execute("insert into test(id) values (?)")

    def testExecuteWrongNoOfArgs3(self):
        # no parameters, parameters are needed
        with self.assertRaises(dbapi2.ProgrammingError):
            self.cu.execute("insert into test(id) values (?)")

    def testExecuteParamList(self):
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("select name from test where name=?", ["foo"])
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")

    def testExecuteParamSequence(self):
        class L(object):
            def __len__(self):
                return 1
            def __getitem__(self, x):
                assert x == 0
                return "foo"

        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("select name from test where name=?", L())
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")

    def testExecuteDictMapping(self):
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("select name from test where name=:name", {"name": "foo"})
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")

    def testExecuteDictMapping_Mapping(self):
        class D(dict):
            def __missing__(self, key):
                return "foo"

        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("select name from test where name=:name", D())
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")

    def testExecuteDictMappingTooLittleArgs(self):
        self.cu.execute("insert into test(name) values ('foo')")
        with self.assertRaises(dbapi2.ProgrammingError):
            self.cu.execute("select name from test where name=:name and id=:id", {"name": "foo"})

    def testExecuteDictMappingNoArgs(self):
        self.cu.execute("insert into test(name) values ('foo')")
        with self.assertRaises(dbapi2.ProgrammingError):
            self.cu.execute("select name from test where name=:name")

    def testExecuteDictMappingUnnamed(self):
        self.cu.execute("insert into test(name) values ('foo')")
        with self.assertRaises(dbapi2.ProgrammingError):
            self.cu.execute("select name from test where name=?", {"name": "foo"})

    def testClose(self):
        self.cu.close()

    def testRowcountExecute(self):
        self.cu.execute("delete from test")
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("update test set name='bar'")
        self.assertEqual(self.cu.rowcount, 2)

    def testRowcountSelect(self):
        """
        pydbapi2 does not know the rowcount of SELECT statements, because we
        don't fetch all rows after executing the select statement. The rowcount
        has thus to be -1.
        """
        self.cu.execute("select 5 union select 6")
        self.assertEqual(self.cu.rowcount, -1)

    def testRowcountExecutemany(self):
        self.cu.execute("delete from test")
        self.cu.executemany("insert into test(name) values (?)", [(1,), (2,), (3,)])
        self.assertEqual(self.cu.rowcount, 3)

    def testTotalChanges(self):
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("insert into test(name) values ('foo')")
        self.assertLess(2, self.cx.total_changes, msg='total changes reported wrong value')

    # Checks for executemany:
    # Sequences are required by the DB-API, iterators
    # enhancements in pydbapi2.

    def testExecuteManySequence(self):
        self.cu.executemany("insert into test(income) values (?)", [(x,) for x in range(100, 110)])

    def testExecuteManyIterator(self):
        class MyIter:
            def __init__(self):
                self.value = 5

            def __next__(self):
                if self.value == 10:
                    raise StopIteration
                else:
                    self.value += 1
                    return (self.value,)

        self.cu.executemany("insert into test(income) values (?)", MyIter())

    def testExecuteManyGenerator(self):
        def mygen():
            for i in range(5):
                yield (i,)

        self.cu.executemany("insert into test(income) values (?)", mygen())

    def testExecuteManyWrongSqlArg(self):
        with self.assertRaises(ValueError):
            self.cu.executemany(42, [(3,)])

    def testExecuteManySelect(self):
        with self.assertRaises(dbapi2.ProgrammingError):
            self.cu.executemany("select ?", [(3,)])

    def testExecuteManyNotIterable(self):
        with self.assertRaises(TypeError):
            self.cu.executemany("insert into test(income) values (?)", 42)

    def testFetchIter(self):
        # Optional DB-API extension.
        self.cu.execute("delete from test")
        self.cu.execute("insert into test(id) values (?)", (5,))
        self.cu.execute("insert into test(id) values (?)", (6,))
        self.cu.execute("select id from test order by id")
        lst = []
        for row in self.cu:
            lst.append(row[0])
        self.assertEqual(lst[0], 5)
        self.assertEqual(lst[1], 6)

    def testFetchone(self):
        self.cu.execute("select name from test")
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")
        row = self.cu.fetchone()
        self.assertEqual(row, None)

    def testFetchoneNoStatement(self):
        cur = self.cx.cursor()
        row = cur.fetchone()
        self.assertEqual(row, None)

    def testArraySize(self):
        # must default ot 1
        self.assertEqual(self.cu.arraysize, 1)

        # now set to 2
        self.cu.arraysize = 2

        # now make the query return 3 rows
        self.cu.execute("delete from test")
        self.cu.execute("insert into test(name) values ('A')")
        self.cu.execute("insert into test(name) values ('B')")
        self.cu.execute("insert into test(name) values ('C')")
        self.cu.execute("select name from test")
        res = self.cu.fetchmany()

        self.assertEqual(len(res), 2)

    def testFetchmany(self):
        self.cu.execute("select name from test")
        res = self.cu.fetchmany(100)
        self.assertEqual(len(res), 1)
        res = self.cu.fetchmany(100)
        self.assertEqual(res, [])

    def testFetchmanyKwArg(self):
        """Checks if fetchmany works with keyword arguments"""
        self.cu.execute("select name from test")
        res = self.cu.fetchmany(size=100)
        self.assertEqual(len(res), 1)

    def testFetchall(self):
        self.cu.execute("select name from test")
        res = self.cu.fetchall()
        self.assertEqual(len(res), 1)
        res = self.cu.fetchall()
        self.assertEqual(res, [])

    def testSetinputsizes(self):
        self.cu.setinputsizes([3, 4, 5])

    def testSetoutputsize(self):
        self.cu.setoutputsize(5, 0)

    def testSetoutputsizeNoColumn(self):
        self.cu.setoutputsize(42)

    def testCursorConnection(self):
        # Optional DB-API extension.
        self.assertEqual(self.cu.connection, self.cx)

    def testWrongCursorCallable(self):
        with self.assertRaises(TypeError):
            def f(): pass
            cur = self.cx.cursor(f)

    def testCursorWrongClass(self):
        class Foo: pass
        foo = Foo()
        with self.assertRaises(TypeError):
            cur = dbapi2.Cursor(foo)

    def testLastRowIDOnReplace(self):
        """
        INSERT OR REPLACE and REPLACE INTO should produce the same behavior.
        """
        sql = '{} INTO test(id, unique_test) VALUES (?, ?)'
        for statement in ('INSERT OR REPLACE', 'REPLACE'):
            with self.subTest(statement=statement):
                self.cu.execute(sql.format(statement), (1, 'foo'))
                self.assertEqual(self.cu.lastrowid, 1)

    def testLastRowIDOnIgnore(self):
        self.cu.execute(
            "insert or ignore into test(unique_test) values (?)",
            ('test',))
        self.assertEqual(self.cu.lastrowid, 2)
        self.cu.execute(
            "insert or ignore into test(unique_test) values (?)",
            ('test',))
        self.assertEqual(self.cu.lastrowid, 2)

    def testLastRowIDInsertOR(self):
        results = []
        for statement in ('FAIL', 'ABORT', 'ROLLBACK'):
            sql = 'INSERT OR {} INTO test(unique_test) VALUES (?)'
            with self.subTest(statement='INSERT OR {}'.format(statement)):
                self.cu.execute(sql.format(statement), (statement,))
                results.append((statement, self.cu.lastrowid))
                with self.assertRaises(dbapi2.IntegrityError):
                    self.cu.execute(sql.format(statement), (statement,))
                results.append((statement, self.cu.lastrowid))
        expected = [
            ('FAIL', 2), ('FAIL', 2),
            ('ABORT', 3), ('ABORT', 3),
            ('ROLLBACK', 4), ('ROLLBACK', 4),
        ]
        self.assertEqual(results, expected)


class ThreadTests(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.con = dbapi2.connect(getConnection())
        self.cur = self.con.cursor()
        self.cur.execute("create table test(id integer primary key, name text, bin binary, ratio number, ts timestamp)")

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def testConCursor(self):
        def run(con, errors):
            try:
                cur = con.cursor()
                errors.append("did not raise ProgrammingError")
                return
            except dbapi2.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"con": self.con, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def testConCommit(self):
        def run(con, errors):
            try:
                con.commit()
                errors.append("did not raise ProgrammingError")
                return
            except dbapi2.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"con": self.con, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def testConRollback(self):
        def run(con, errors):
            try:
                con.rollback()
                errors.append("did not raise ProgrammingError")
                return
            except dbapi2.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"con": self.con, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def testConClose(self):
        def run(con, errors):
            try:
                con.close()
                errors.append("did not raise ProgrammingError")
                return
            except dbapi2.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"con": self.con, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def testCurImplicitBegin(self):
        def run(cur, errors):
            try:
                cur.execute("insert into test(name) values ('a')")
                errors.append("did not raise ProgrammingError")
                return
            except dbapi2.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"cur": self.cur, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def testCurClose(self):
        def run(cur, errors):
            try:
                cur.close()
                errors.append("did not raise ProgrammingError")
                return
            except dbapi2.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"cur": self.cur, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def testCurExecute(self):
        def run(cur, errors):
            try:
                cur.execute("select name from test")
                errors.append("did not raise ProgrammingError")
                return
            except dbapi2.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        self.cur.execute("insert into test(name) values ('a')")
        t = threading.Thread(target=run, kwargs={"cur": self.cur, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def testCurIterNext(self):
        def run(cur, errors):
            try:
                row = cur.fetchone()
                errors.append("did not raise ProgrammingError")
                return
            except dbapi2.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        self.cur.execute("insert into test(name) values ('a')")
        self.cur.execute("select name from test")
        t = threading.Thread(target=run, kwargs={"cur": self.cur, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))


class ConstructorTests(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testDate(self):
        d = dbapi2.Date(2004, 10, 28)

    def testTime(self):
        t = dbapi2.Time(12, 39, 35)

    def testTimestamp(self):
        ts = dbapi2.Timestamp(2004, 10, 28, 12, 39, 35)

    def testDateFromTicks(self):
        d = dbapi2.DateFromTicks(42)

    def testTimeFromTicks(self):
        t = dbapi2.TimeFromTicks(42)

    def testTimestampFromTicks(self):
        ts = dbapi2.TimestampFromTicks(42)

    def testBinary(self):
        b = dbapi2.Binary(b"\0'")


class ExtensionTests(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testScriptStringSql(self):
        con = dbapi2.connect(getConnection())
        cur = con.cursor()
        cur.executescript("""
            -- bla bla
            /* a stupid comment */
            create table a(i);
            insert into a(i) values (5);
            """)
        cur.execute("select i from a")
        res = cur.fetchone()[0]
        self.assertEqual(res, 5)

    def testScriptSyntaxError(self):
        con = dbapi2.connect(getConnection())
        cur = con.cursor()
        with self.assertRaises(dbapi2.OperationalError):
            cur.executescript("create table test(x); asdf; create table test2(x)")

    def testScriptErrorNormal(self):
        con = dbapi2.connect(getConnection())
        cur = con.cursor()
        with self.assertRaises(dbapi2.OperationalError):
            cur.executescript("create table test(sadfsadfdsa); select foo from hurz;")

    def testCursorExecutescriptAsBytes(self):
        con = dbapi2.connect(getConnection())
        cur = con.cursor()
        with self.assertRaises(ValueError) as cm:
            cur.executescript(b"create table test(foo); insert into test(foo) values (5);")
        self.assertEqual(str(cm.exception), 'script argument must be unicode.')

    def testConnectionExecute(self):
        con = dbapi2.connect(getConnection())
        result = con.execute("select 5").fetchone()[0]
        self.assertEqual(result, 5, "Basic test of Connection.execute")

    def testConnectionExecutemany(self):
        con = dbapi2.connect(getConnection())
        con.execute("create table test(foo)")
        con.executemany("insert into test(foo) values (?)", [(3,), (4,)])
        result = con.execute("select foo from test order by foo").fetchall()
        self.assertEqual(result[0][0], 3, "Basic test of Connection.executemany")
        self.assertEqual(result[1][0], 4, "Basic test of Connection.executemany")

    def testConnectionExecutescript(self):
        con = dbapi2.connect(getConnection())
        con.executescript("create table test(foo); insert into test(foo) values (5);")
        result = con.execute("select foo from test").fetchone()[0]
        self.assertEqual(result, 5, "Basic test of Connection.executescript")


class ClosedConTests(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testClosedConCursor(self):
        con = dbapi2.connect(getConnection())
        con.close()
        with self.assertRaises(dbapi2.ProgrammingError):
            cur = con.cursor()

    def testClosedConCommit(self):
        con = dbapi2.connect(getConnection())
        con.close()
        with self.assertRaises(dbapi2.ProgrammingError):
            con.commit()

    def testClosedConRollback(self):
        con = dbapi2.connect(getConnection())
        con.close()
        with self.assertRaises(dbapi2.ProgrammingError):
            con.rollback()

    def testClosedCurExecute(self):
        con = dbapi2.connect(getConnection())
        cur = con.cursor()
        con.close()
        with self.assertRaises(dbapi2.ProgrammingError):
            cur.execute("select 4")

    def testClosedCreateFunction(self):
        con = dbapi2.connect(getConnection())
        con.close()
        def f(x): return 17
        with self.assertRaises(dbapi2.ProgrammingError):
            con.create_function("foo", 1, f)

    def testClosedCreateAggregate(self):
        con = dbapi2.connect(getConnection())
        con.close()
        class Agg:
            def __init__(self):
                pass
            def step(self, x):
                pass
            def finalize(self):
                return 17
        with self.assertRaises(dbapi2.ProgrammingError):
            con.create_aggregate("foo", 1, Agg)

    def testClosedSetAuthorizer(self):
        con = dbapi2.connect(getConnection())
        con.close()
        def authorizer(*args):
            return dbapi2.DENY
        with self.assertRaises(dbapi2.ProgrammingError):
            con.set_authorizer(authorizer)

    def testClosedSetProgressCallback(self):
        con = dbapi2.connect(getConnection())
        con.close()
        def progress(): pass
        with self.assertRaises(dbapi2.ProgrammingError):
            con.set_progress_handler(progress, 100)

    def testClosedCall(self):
        con = dbapi2.connect(getConnection())
        con.close()
        with self.assertRaises(dbapi2.ProgrammingError):
            con()

class ClosedCurTests(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testClosed(self):
        con = dbapi2.connect(getConnection())
        cur = con.cursor()
        cur.close()

        for method_name in ("execute", "executemany", "executescript", "fetchall", "fetchmany", "fetchone"):
            if method_name in ("execute", "executescript"):
                params = ("select 4 union select 5",)
            elif method_name == "executemany":
                params = ("insert into foo(bar) values (?)", [(3,), (4,)])
            else:
                params = []

            with self.assertRaises(dbapi2.ProgrammingError):
                method = getattr(cur, method_name)
                method(*params)

#  
#  class SqliteOnConflictTests(unittest.TestCase):
#      """
#      Tests for SQLite's "insert on conflict" feature.
#  
#      See https://www.dbapi2.org/lang_conflict.html for details.
#      """
#  
#      def setUp(self):
#          self.cx = dbapi2.connect(getConnection())
#          self.cu = self.cx.cursor()
#          self.cu.execute("""
#            CREATE TABLE test(
#              id INTEGER PRIMARY KEY, name TEXT, unique_name TEXT UNIQUE
#            );
#          """)
#  
#      def tearDown(self):
#          self.cu.close()
#          self.cx.close()
#  
#      def testOnConflictRollbackWithExplicitTransaction(self):
#          self.cx.isolation_level = None  # autocommit mode
#          self.cu = self.cx.cursor()
#          # Start an explicit transaction.
#          self.cu.execute("BEGIN")
#          self.cu.execute("INSERT INTO test(name) VALUES ('abort_test')")
#          self.cu.execute("INSERT OR ROLLBACK INTO test(unique_name) VALUES ('foo')")
#          with self.assertRaises(dbapi2.IntegrityError):
#              self.cu.execute("INSERT OR ROLLBACK INTO test(unique_name) VALUES ('foo')")
#          # Use connection to commit.
#          self.cx.commit()
#          self.cu.execute("SELECT name, unique_name from test")
#          # Transaction should have rolled back and nothing should be in table.
#          self.assertEqual(self.cu.fetchall(), [])
#  
#      def testOnConflictAbortRaisesWithExplicitTransactions(self):
#          # Abort cancels the current sql statement but doesn't change anything
#          # about the current transaction.
#          self.cx.isolation_level = None  # autocommit mode
#          self.cu = self.cx.cursor()
#          # Start an explicit transaction.
#          self.cu.execute("BEGIN")
#          self.cu.execute("INSERT INTO test(name) VALUES ('abort_test')")
#          self.cu.execute("INSERT OR ABORT INTO test(unique_name) VALUES ('foo')")
#          with self.assertRaises(dbapi2.IntegrityError):
#              self.cu.execute("INSERT OR ABORT INTO test(unique_name) VALUES ('foo')")
#          self.cx.commit()
#          self.cu.execute("SELECT name, unique_name FROM test")
#          # Expect the first two inserts to work, third to do nothing.
#          self.assertEqual(self.cu.fetchall(), [('abort_test', None), (None, 'foo',)])
#  
#      def testOnConflictRollbackWithoutTransaction(self):
#          # Start of implicit transaction
#          self.cu.execute("INSERT INTO test(name) VALUES ('abort_test')")
#          self.cu.execute("INSERT OR ROLLBACK INTO test(unique_name) VALUES ('foo')")
#          with self.assertRaises(dbapi2.IntegrityError):
#              self.cu.execute("INSERT OR ROLLBACK INTO test(unique_name) VALUES ('foo')")
#          self.cu.execute("SELECT name, unique_name FROM test")
#          # Implicit transaction is rolled back on error.
#          self.assertEqual(self.cu.fetchall(), [])
#  
#      def testOnConflictAbortRaisesWithoutTransactions(self):
#          # Abort cancels the current sql statement but doesn't change anything
#          # about the current transaction.
#          self.cu.execute("INSERT INTO test(name) VALUES ('abort_test')")
#          self.cu.execute("INSERT OR ABORT INTO test(unique_name) VALUES ('foo')")
#          with self.assertRaises(dbapi2.IntegrityError):
#              self.cu.execute("INSERT OR ABORT INTO test(unique_name) VALUES ('foo')")
#          # Make sure all other values were inserted.
#          self.cu.execute("SELECT name, unique_name FROM test")
#          self.assertEqual(self.cu.fetchall(), [('abort_test', None), (None, 'foo',)])
#  
#      def testOnConflictFail(self):
#          self.cu.execute("INSERT OR FAIL INTO test(unique_name) VALUES ('foo')")
#          with self.assertRaises(dbapi2.IntegrityError):
#              self.cu.execute("INSERT OR FAIL INTO test(unique_name) VALUES ('foo')")
#          self.assertEqual(self.cu.fetchall(), [])
#  
#      def testOnConflictIgnore(self):
#          self.cu.execute("INSERT OR IGNORE INTO test(unique_name) VALUES ('foo')")
#          # Nothing should happen.
#          self.cu.execute("INSERT OR IGNORE INTO test(unique_name) VALUES ('foo')")
#          self.cu.execute("SELECT unique_name FROM test")
#          self.assertEqual(self.cu.fetchall(), [('foo',)])
#  
#      def testOnConflictReplace(self):
#          self.cu.execute("INSERT OR REPLACE INTO test(name, unique_name) VALUES ('Data!', 'foo')")
#          # There shouldn't be an IntegrityError exception.
#          self.cu.execute("INSERT OR REPLACE INTO test(name, unique_name) VALUES ('Very different data!', 'foo')")
#          self.cu.execute("SELECT name, unique_name FROM test")
#          self.assertEqual(self.cu.fetchall(), [('Very different data!', 'foo')])
#  
#  
