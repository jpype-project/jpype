# This file is Public Domain and may be used without restrictions,
# because noone should have to waste their lives typing this again.
import _jpype
import jpype
from jpype.types import *
from jpype import java
import jpype.dbapi2 as dbapi2
import common
import time
import datetime
import decimal
import threading

java = jpype.java

try:
    import zlib
except ImportError:
    zlib = None


db_name = "jdbc:hsqldb:mem:myDb"
#db_name = "jdbc:derby:memory:myDb"
#first = "jdbc:derby:memory:myDb;create=True"


class ConnectTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def testConnect(self):
        cx = dbapi2.connect(db_name)
        self.assertIsInstance(cx, dbapi2.Connection)

    def testClose(self):
        cx = dbapi2.connect(db_name)
        cx.close()
        # Closing twice is an error
        with self.assertRaises(dbapi2.ProgrammingError):
            cx.close()

    def testScope(self):
        with dbapi2.connect(db_name) as cx:
            pass
        with self.assertRaises(dbapi2.ProgrammingError):
            cx.close()

    def testAdapters(self):
        cx = dbapi2.connect(db_name)
        self.assertEqual(cx.adapters, dbapi2._default_adapters)
        cx = dbapi2.connect(db_name, adapters=None)
        self.assertEqual(cx.adapters, {})
        m = {}
        cx = dbapi2.connect(db_name, adapters=m)
        self.assertEqual(cx.adapters, m)

    def testConverters(self):
        cx = dbapi2.connect(db_name)
        self.assertEqual(cx.converters, dbapi2._default_converters)
        cx = dbapi2.connect(db_name, converters=None)
        self.assertEqual(cx.converters, {})
        m = {}
        cx = dbapi2.connect(db_name, converters=m)
        self.assertEqual(cx.converters, m)


class ConnectionTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def test_commit(self):
        with dbapi2.connect(db_name) as cx:
            # Commit must work without a command
            cx.commit()

    def test_rollback(self):
        with dbapi2.connect(db_name) as cx:
            # Commit must work without a command
            cx.rollback()

    def test_cursor(self):
        with dbapi2.connect(db_name) as cx:
            self.assertIsInstance(cx.cursor(), dbapi2.Cursor)

    def testAdapters(self):
        with dbapi2.connect(db_name) as cx:
            m = {'foo': 1}
            cx.adapters = m
            self.assertEqual(cx._adapters, m)
            with self.assertRaises(TypeError):
                cx.adapters = object()

    def testConverters(self):
        with dbapi2.connect(db_name) as cx:
            m = {'foo': 1}
            cx.converters = m
            self.assertEqual(cx._converters, m)
            with self.assertRaises(TypeError):
                cx.converters = object()

    def testClosedClass(self):
        with dbapi2.connect(db_name) as cx:
            with self.assertRaises(AttributeError):
                cx.seters = object()

    def testAutocommitFail(self):
        with dbapi2.connect(db_name) as cx:
            cx.autocommit = True
            with self.assertRaises(dbapi2.NotSupportedError):
                cx.commit()
            with self.assertRaises(dbapi2.NotSupportedError):
                cx.rollback()

    def test_typeinfo(self):
        with dbapi2.connect(db_name) as cx:
            ti = cx.typeinfo
            self.assertIsInstance(ti, dict)
            for p, v in ti.items():
                self.assertIsInstance(p, str)
                self.assertIsInstance(v, dbapi2.JDBCType)
        with self.assertRaises(dbapi2.ProgrammingError):
            ti = cx.typeinfo

    def test_connection(self):
        with dbapi2.connect(db_name) as cx:
            c = cx.connection
            self.assertIsInstance(c, java.sql.Connection)

    def test_getters(self):
        with dbapi2.connect(db_name) as cx:
            cx.getters = dbapi2.GETTERS_BY_NAME
            self.assertEqual(cx._getters, dbapi2.GETTERS_BY_NAME)
            self.assertEqual(cx.getters, dbapi2.GETTERS_BY_NAME)
            cx.getters = dbapi2.GETTERS_BY_TYPE
            self.assertEqual(cx._getters, dbapi2.GETTERS_BY_TYPE)
            self.assertEqual(cx.getters, dbapi2.GETTERS_BY_TYPE)

    def test_setters(self):
        with dbapi2.connect(db_name) as cx:
            cx.setters = dbapi2.SETTERS_BY_META
            self.assertEqual(cx._setters, dbapi2.SETTERS_BY_META)
            self.assertEqual(cx.setters, dbapi2.SETTERS_BY_META)
            cx.setters = dbapi2.SETTERS_BY_TYPE
            self.assertEqual(cx._setters, dbapi2.SETTERS_BY_TYPE)
            self.assertEqual(cx.setters, dbapi2.SETTERS_BY_TYPE)

    def test_adapters(self):
        with dbapi2.connect(db_name) as cx:
            self.assertEqual(cx.adapters, dbapi2._default_adapters)
            cx.adapters = None
            self.assertEqual(cx.adapters, {})
            m = {}
            cx.adapters = m
            self.assertEqual(cx.adapters, m)
            with self.assertRaises(dbapi2.InterfaceError):
                cx.adapters = object()

    def test_converters(self):
        with dbapi2.connect(db_name) as cx:
            self.assertEqual(cx.converters, dbapi2._default_converters)
            cx.converters = None
            self.assertEqual(cx.converters, {})
            m = {}
            cx.converters = m
            self.assertEqual(cx.converters, m)
            with self.assertRaises(dbapi2.InterfaceError):
                cx.converters = object()


class CursorTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def tearDown(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            try:
                cur.execute("drop table booze")
            except dbapi2.Error:
                pass
            try:
                cur.execute("drop table barflys")
            except dbapi2.Error:
                pass

    def testCursorIsolation(self):
        with dbapi2.connect(db_name) as cx:
            # Make sure cursors created from the same connection have
            # the documented transaction isolation level
            cur1 = cx.cursor()
            cur2 = cx.cursor()
            cur1.execute("create table booze (name varchar(20))")
            cur1.execute("insert into booze values ('Victoria Bitter')")
            cur2.execute("select name from booze")
            booze = cur2.fetchall()
            self.assertEqual(len(booze), 1)
            self.assertEqual(len(booze[0]), 1)
            self.assertEqual(booze[0][0], "Victoria Bitter")

    def testDescription(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table booze (name varchar(20))")
            self.assertEqual(
                cur.description,
                None,
                "cursor.description should be none after executing a "
                "statement that can return no rows (such as create table)",
            )
            cur.execute("select name from booze")
            self.assertEqual(
                len(cur.description), 1, "cursor.description describes too many columns"
            )
            self.assertEqual(
                len(cur.description[0]),
                7,
                "cursor.description[x] tuples must have 7 elements",
            )
            self.assertEqual(
                cur.description[0][0].lower(),
                "name",
                "cursor.description[x][0] must return column name",
            )
            self.assertEqual(
                cur.description[0][1],
                dbapi2.STRING,
                "cursor.description[x][1] must return column type. Got %r"
                % cur.description[0][1],
            )

            # Make sure self.description gets reset
            cur.execute("create table barflys (name varchar(20))")
            self.assertEqual(
                cur.description,
                None,
                "cursor.description not being set to None when executing "
                "no-result statements (eg. create table)",
            )

    def testRowcount(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table booze (name varchar(20))")
            # This was dropped from the specification
            # self.assertEqual(
            #    cur.rowcount,
            #    -1,
            #    "cursor.rowcount should be -1 after executing no-result " "statements",
            # )
            cur.execute("insert into booze values ('Victoria Bitter')")
            self.assertEqual(
                cur.rowcount, 1,
                "cursor.rowcount should == number or rows inserted, or "
                "set to -1 after executing an insert statement",
            )
            cur.execute("select name from booze")
            self.assertEqual(
                cur.rowcount, -1,
                "cursor.rowcount should == number of rows returned, or "
                "set to -1 after executing a select statement",
            )
            cur.execute("create table barflys (name varchar(20))")
            # THis test does not match jdbc parameters
            # self.assertEqual(
            #    cur.rowcount,
            #    -1,
            #    "cursor.rowcount not being reset to -1 after executing "
            #    "no-result statements",
            # )

    def testClose(self):
        cx = dbapi2.connect(db_name)
        try:
            cur = cx.cursor()
        finally:
            cx.close()

        # cursor.execute should raise an Error if called after connection
        # closed
        with self.assertRaises(dbapi2.Error):
            cur.execute("create table booze (name varchar(20))")

        # connection.commit should raise an Error if called after connection'
        # closed.'
        self.assertRaises(dbapi2.Error, cx.commit)

        # connection.close should raise an Error if called more than once
        self.assertRaises(dbapi2.Error, cx.close)

    def test_None(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table booze (name varchar(20))")
            cur.execute("insert into booze values (NULL)")
            cur.execute("select name from booze")
            r = cur.fetchall()
            self.assertEqual(len(r), 1)
            self.assertEqual(len(r[0]), 1)
            self.assertEqual(r[0][0], None, "NULL value not returned as None")

    def testArraysize(self):
        # Not much here - rest of the tests for this are in test_fetchmany
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            self.assertTrue(
                hasattr(cur, "arraysize"), "cursor.arraysize must be defined"
            )

    def _paraminsert(self, cur):
        cur.execute("create table booze (name varchar(20))")
        cur.execute("insert into booze values ('Victoria Bitter')")
        self.assertTrue(cur.rowcount in (-1, 1))
        cur.execute("insert into booze values (?)", ("Cooper's",))
        self.assertTrue(cur.rowcount in (-1, 1))
        cur.execute("select name from booze")
        res = cur.fetchall()
        self.assertEqual(len(res), 2, "cursor.fetchall returned too few rows")
        beers = [res[0][0], res[1][0]]
        beers.sort()
        self.assertEqual(
            beers[0],
            "Cooper's",
            "cursor.fetchall retrieved incorrect data, or data inserted " "incorrectly",
        )
        self.assertEqual(
            beers[1],
            "Victoria Bitter",
            "cursor.fetchall retrieved incorrect data, or data inserted " "incorrectly",
        )

    def test_execute(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            self._paraminsert(cur)

    def test_executeBad(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            with self.assertRaises(dbapi2.InterfaceError):
                cur.execute("insert into booze values (?)", object())
            with self.assertRaises(dbapi2.InterfaceError):
                cur.execute(object())
            with self.assertRaises(dbapi2.ProgrammingError):
                cur.execute("inert into booze values (?)", [["?"]])

    def test_setinputsizes(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.setinputsizes((25,))
            self._paraminsert(cur)  # Make sure cursor still works

    def test_setoutputsize_basic(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.setoutputsize(1000)
            cur.setoutputsize(2000, 0)
            self._paraminsert(cur)  # Make sure the cursor still works

    def test_executemany(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table booze (name varchar(20))")
            largs = [("Cooper's",), ("Boag's",)]
            margs = [{"beer": "Cooper's"}, {"beer": "Boag's"}]
            cur.executemany("insert into booze values (?)", largs)
            self.assertEqual(
                cur.rowcount, 2,
                "insert using cursor.executemany set cursor.rowcount to "
                "incorrect value %r" % cur.rowcount,
            )
            cur.execute("select name from booze")
            res = cur.fetchall()
            self.assertEqual(
                len(res), 2, "cursor.fetchall retrieved incorrect number of rows"
            )
            beers = [res[0][0], res[1][0]]
            beers.sort()
            self.assertEqual(beers[0], "Boag's", "incorrect data retrieved")
            self.assertEqual(beers[1], "Cooper's", "incorrect data retrieved")

    def test_executemanyGenerator(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            def mygen(ls):
                for i in ls:
                    yield [i]
            cu.execute("create table booze (name varchar(20))")
            cu.executemany("insert into booze values (?)", mygen(self.samples))
            cu.execute("select * from booze")
            f = cu.fetchall()
            for i, v in enumerate(self.samples):
                self.assertEqual(f[i][0], v)

    def test_executemanyIterator(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            class myiter:
                def __init__(self, ls):
                    self.ls = ls
                    self.i = 0

                def __iter__(self):
                    return self

                def __next__(self):
                    if self.i == len(self.ls):
                        raise StopIteration
                    rc = [self.ls[self.i]]
                    self.i += 1
                    return rc
            cu.execute("create table booze (name varchar(20))")
            cu.executemany("insert into booze values (?)", myiter(self.samples))
            cu.execute("select * from booze")
            f = cu.fetchall()
            for i, v in enumerate(self.samples):
                self.assertEqual(f[i][0], v)

    def test_executemanyBad(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table booze (name varchar(20))")
            with self.assertRaises(TypeError):
                cu.executemany("insert into booze values (?)", object())
            with self.assertRaises(dbapi2.InterfaceError):
                cu.executemany("insert into booze values (?)", object())
            with self.assertRaises(dbapi2.InterfaceError):
                cu.executemany(object(), [])
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.executemany("insert into booze values (?)", None)
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.executemany("inert into booze values (?)", [['?']])

    def test_fetchone(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            # cursor.fetchone should raise an Error if called before
            # executing a select-type query
            self.assertRaises(dbapi2.ProgrammingError, cur.fetchone)

            # cursor.fetchone should raise an Error if called after
            # executing a query that cannot return rows
            cur.execute("create table booze (name varchar(20))")
            self.assertRaises(dbapi2.Error, cur.fetchone)

            cur.execute("select name from booze")
            self.assertEqual(
                cur.fetchone(),
                None,
                "cursor.fetchone should return None if a query retrieves " "no rows",
            )
            self.assertEqual(cur.rowcount, -1)

            # cursor.fetchone should raise an Error if called after
            # executing a query that cannot return rows
            cur.execute("insert into booze values ('Victoria Bitter')")
            self.assertRaises(dbapi2.Error, cur.fetchone)

            cur.execute("select name from booze")
            r = cur.fetchone()
            self.assertEqual(
                len(r), 1, "cursor.fetchone should have retrieved a single row"
            )
            self.assertEqual(
                r[0], "Victoria Bitter", "cursor.fetchone retrieved incorrect data"
            )
            self.assertEqual(
                cur.fetchone(),
                None,
                "cursor.fetchone should return None if no more rows available",
            )
            self.assertEqual(cur.rowcount, -1)

    samples = [
        "Carlton Cold",
        "Carlton Draft",
        "Mountain Goat",
        "Redback",
        "Victoria Bitter",
        "XXXX",
    ]

    def _populate(self):
        """ Return a list of sql commands to setup the DB for the fetch
            tests.
        """
        populate = [
            "insert into booze values ('{}')".format(s)
            for s in self.samples
        ]
        return populate

    def test_fetchmany(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:

            # cursor.fetchmany should raise an Error if called without
            # issuing a query
            self.assertRaises(dbapi2.ProgrammingError, cur.fetchmany, 4)

            cur.execute("create table booze (name varchar(20))")
            for sql in self._populate():
                cur.execute(sql)

            cur.execute("select name from booze")
            r = cur.fetchmany()
            self.assertEqual(
                len(r),
                1,
                "cursor.fetchmany retrieved incorrect number of rows, "
                "default of arraysize is one.",
            )
            cur.arraysize = 10
            r = cur.fetchmany(3)  # Should get 3 rows
            self.assertEqual(
                len(r), 3, "cursor.fetchmany retrieved incorrect number of rows"
            )
            r = cur.fetchmany(4)  # Should get 2 more
            self.assertEqual(
                len(r), 2, "cursor.fetchmany retrieved incorrect number of rows"
            )
            r = cur.fetchmany(4)  # Should be an empty sequence
            self.assertEqual(
                len(r),
                0,
                "cursor.fetchmany should return an empty sequence after "
                "results are exhausted",
            )
            self.assertEqual(cur.rowcount, -1)

            # Same as above, using cursor.arraysize
            cur.arraysize = 4
            cur.execute("select name from booze")
            r = cur.fetchmany()  # Should get 4 rows
            self.assertEqual(
                len(r), 4, "cursor.arraysize not being honoured by fetchmany"
            )
            r = cur.fetchmany()  # Should get 2 more
            self.assertEqual(len(r), 2)
            r = cur.fetchmany()  # Should be an empty sequence
            self.assertEqual(len(r), 0)
            self.assertEqual(cur.rowcount, -1)

            cur.arraysize = 6
            cur.execute("select name from booze")
            rows = cur.fetchmany()  # Should get all rows
            self.assertTrue(cur.rowcount in (-1, 6))
            self.assertEqual(len(rows), 6)
            self.assertEqual(len(rows), 6)
            rows = [r[0] for r in rows]
            rows.sort()

            # Make sure we get the right data back out
            for i in range(0, 6):
                self.assertEqual(
                    rows[i],
                    self.samples[i],
                    "incorrect data retrieved by cursor.fetchmany",
                )

            rows = cur.fetchmany()  # Should return an empty list
            self.assertEqual(
                len(rows),
                0,
                "cursor.fetchmany should return an empty sequence if "
                "called after the whole result set has been fetched",
            )
            self.assertEqual(cur.rowcount, -1)

            cur.execute("create table barflys (name varchar(20))")
            cur.execute("select name from barflys")
            r = cur.fetchmany()  # Should get empty sequence
            self.assertEqual(
                len(r),
                0,
                "cursor.fetchmany should return an empty sequence if "
                "query retrieved no rows",
            )

    def test_fetchall(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            # cursor.fetchall should raise an Error if called
            # without executing a query that may return rows (such
            # as a select)
            self.assertRaises(dbapi2.Error, cur.fetchall)

            cur.execute("create table booze (name varchar(20))")
            for sql in self._populate():
                cur.execute(sql)

            # cursor.fetchall should raise an Error if called
            # after executing a statement that cannot return rows
            self.assertRaises(dbapi2.ProgrammingError, cur.fetchall)

            cur.execute("select name from booze")
            rows = cur.fetchall()
            self.assertEqual(cur.rowcount, -1)
            self.assertEqual(
                len(rows),
                len(self.samples),
                "cursor.fetchall did not retrieve all rows",
            )
            rows = [r[0] for r in rows]
            rows.sort()
            for i in range(0, len(self.samples)):
                self.assertEqual(
                    rows[i], self.samples[i], "cursor.fetchall retrieved incorrect rows"
                )
            rows = cur.fetchall()
            self.assertEqual(
                len(rows),
                0,
                "cursor.fetchall should return an empty list if called "
                "after the whole result set has been fetched",
            )
            self.assertEqual(cur.rowcount, -1)

            cur.execute("create table barflys (name varchar(20))")
            cur.execute("select name from barflys")
            rows = cur.fetchall()
            self.assertEqual(cur.rowcount, -1)
            self.assertEqual(
                len(rows),
                0,
                "cursor.fetchall should return an empty list if "
                "a select query returns no rows",
            )

    def test_mixedfetch(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table booze (name varchar(20))")
            for sql in self._populate():
                cur.execute(sql)

            cur.execute("select name from booze")
            self.assertEqual(cur.rowcount, -1)
            rows1 = cur.fetchone()
            rows23 = cur.fetchmany(2)
            rows4 = cur.fetchone()
            rows56 = cur.fetchall()
            self.assertEqual(cur.rowcount, -1)
            self.assertEqual(
                len(rows23), 2, "fetchmany returned incorrect number of rows"
            )
            self.assertEqual(
                len(rows56), 2, "fetchall returned incorrect number of rows"
            )

            rows = [rows1[0]]
            rows.extend([rows23[0][0], rows23[1][0]])
            rows.append(rows4[0])
            rows.extend([rows56[0][0], rows56[1][0]])
            rows.sort()
            for i in range(0, len(self.samples)):
                self.assertEqual(
                    rows[i], self.samples[i], "incorrect data retrieved or inserted"
                )

    def test_nextset(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table booze (name varchar(20))")
            booze = ["Wiskey", ]
            cu.execute("insert into booze(name) values (?)", booze)
            self.assertEqual(cu.rowcount, 1)
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.execute("select * from booze; select * from booze")

    def test_lastrowid(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table booze(id INTEGER IDENTITY PRIMARY KEY, name varchar(255))")
            cu.execute("insert into booze(name) values('hello')", keys=True)
            id0 = cu.lastrowid
            self.assertIsInstance(id0, int)
            # Call more than once
            id1 = cu.lastrowid
            self.assertIsInstance(id1, int)
            cx.commit()
            cu.execute("insert into booze(name) values('there')", keys=True)
            id2 = cu.lastrowid
            self.assertIsInstance(id2, int)
            cu.execute("select * from booze")
            f = cu.fetchall()
            self.assertEqual(cu.lastrowid, None)
            self.assertEqual(f[0][0], id0)
            self.assertEqual(f[1][0], id2)

    def test_lastrowidMany(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table booze(id INTEGER IDENTITY PRIMARY KEY, name varchar(255))")
            cu.executemany("insert into booze(name) values(?)", [['Redback'], ['Fat Yak']], keys=True)
            ids = cu.lastrowid
            self.assertIsInstance(ids, list)
            self.assertEqual(len(ids), 2)
            cu.execute("select * from booze")
            f = cu.fetchall()
            self.assertEqual(f[0][0], ids[0])
            self.assertEqual(f[1][0], ids[1])

    def test_iter(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            ls = [['Tooheys Old'], ['Nine Tales Original Amber Ale'], ['Dogbolter']]
            cu.execute("create table booze(name varchar(255))")
            cu.executemany("insert into booze(name) values(?)", ls, keys=True)

            # make sure we throw if there is no result set
            with self.assertRaises(dbapi2.ProgrammingError):
                for booze in cu:
                    pass
            cu.execute("select * from booze")
            for i, booze in enumerate(cu):
                self.assertEqual(ls[i], booze)

    def testNoAdapter(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table booze(name varchar(50))")
            with self.assertRaises(dbapi2.InterfaceError):
                cu.execute("insert into booze(name) values(?)", [object()])

    def testBadParameters(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table booze(name varchar(50))")
            with self.assertRaises(dbapi2.InterfaceError):
                cu.execute("insert into booze(name) values(?)", object())

    def test_callproc(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            r = cu.callproc("lower", ("FOO",))
            self.assertEqual(len(r), 1)
            self.assertEqual(r[0], "FOO")
            r = cu.fetchall()
            self.assertEqual(len(r), 1)
            self.assertEqual(len(r[0]), 1)
            self.assertEqual(r[0][0], "foo")

    def test_callprocBad(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            with self.assertRaises(dbapi2.ProgrammingError):
                r = cu.callproc("not_there", ("FOO",))
            with self.assertRaises(dbapi2.InterfaceError):
                r = cu.callproc(object(), ("FOO",))
            with self.assertRaises(dbapi2.InterfaceError):
                r = cu.callproc("lower", (object(),))
            with self.assertRaises(dbapi2.InterfaceError):
                r = cu.callproc("lower", object())

    def test_close(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.close()
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.close()

    def test_parameters(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            with self.assertRaises(dbapi2.ProgrammingError):
                p = cu.parameters
            cu.execute("create table booze(name varchar(50))")
            cu.execute("insert into booze(name) values(?)", ('Hahn Super Dry',))
            p = cu.parameters
            self.assertEqual(p[0][0:2], ('VARCHAR', dbapi2.VARCHAR))

    def test_resultSet(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table booze(name varchar(50))")
            cu.execute("insert into booze(name) values(?)", ('Hahn Super Dry',))
            cu.execute("select * from booze")
            rs = cu.resultSet
            self.assertIsInstance(rs, java.sql.ResultSet)

    def test_fetchClosed(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table booze(name varchar(50))")
            cu.execute("insert into booze(name) values(?)", ('Hahn Super Dry',))
            cu.execute("select * from booze")
            cu.close()
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.fetchone()
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.fetchmany()
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.fetchall()
            self.assertEqual(cu.resultSet, None)

    def test_executeBadParameters(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            def mygen(ls):
                for i in ls:
                    yield i
            cu.execute("create table booze(name varchar(50), price integer)")
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.executemany("insert into booze(name,price) values(?,?)", [('Hahn Super Dry', 2), ('Coors',)])
            with self.assertRaises(dbapi2.InterfaceError):
                cu.execute("insert into booze(name,price) values(?,?)", {'name': 'Budweiser', 'price': 10})
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.executemany("insert into booze(name,price) values(?,?)", mygen([['Budweiser']]))
            cu.execute("insert into booze(name,price) values(?,?)", mygen(['Pabst Blue Robot', 20000]))
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.execute("insert into booze(name,price) values(?,?)", mygen(['Budweiser']))
            with self.assertRaises(dbapi2.InterfaceError):
                cu.execute("insert into booze(name,price) values(?,?)", 'Budweiser')
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.execute("insert into booze(name,price) values(?,?)", mygen(['Budweiser', 2, 'no']))
            with self.assertRaises(dbapi2.InterfaceError):
                cu.execute("insert into booze(name,price) values(?,?)", object())


class AdapterTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")
        self.testdata = b"abcdefg" * 10
        self.params = memoryview(zlib.compress(self.testdata))

    def tearDown(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            try:
                cur.execute("drop table test")
            except dbapi2.Error:
                pass

    def testOnConnector(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name varchar(255))")
            cx.adapters[object] = lambda x: "none"
            cu.execute("insert into test(name) values(?)", [object()])
            cu.execute("select * from test")
            f = cu.fetchall()
            self.assertEqual(f[0][0], "none")


@common.unittest.skipUnless(zlib, "requires zlib")
class ConverterTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")
        self.testdata = b"abcdefg" * 10
        self.params = memoryview(zlib.compress(self.testdata))

    def tearDown(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            try:
                cur.execute("drop table test")
            except dbapi2.Error:
                pass

    @staticmethod
    def convert(s):
        return zlib.decompress(s)

    def testConvertersBad(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name varchar(20))")
            cu.executemany("insert into test(name) values(?)", [['alice'], ['bob'], ['charlie']])
            cu.execute("select name from test")
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.fetchone(converters=[])
            cu.execute("select name from test")
            with self.assertRaises(dbapi2.InterfaceError):
                cu.fetchone(converters=[int])
            cu.execute("select name from test")
            with self.assertRaises(dbapi2.ProgrammingError):
                cu.fetchone(converters=[str, str])

    def testConvertersNone(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name varchar(20))")
            cu.executemany("insert into test(name) values(?)", [['alice'], ['bob'], ['charlie']])
            cu.execute("select name from test")
            f = cu.fetchone(converters=None)
            self.assertIsInstance(f[0], java.lang.String)
            cu.execute("select name from test")

    def testConvertersPositional(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name varchar(20))")
            cu.executemany("insert into test(name) values(?)", [['alice'], ['bob'], ['charlie']])
            cu.execute("select name from test")
            f = cu.fetchone(converters=[str])
            self.assertIsInstance(f[0], str)
            cu.execute("select name from test")

    def test_fetchoneTypesPositional(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name varchar(20))")
            cu.executemany("insert into test(name) values(?)", [['alice'], ['bob'], ['charlie']])
            cu.execute("select name from test")
            f = cu.fetchone(types=[dbapi2.STRING])
            self.assertIsInstance(f[0], str)

    def test_fetchmanyTypesPositional(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name varchar(20))")
            cu.executemany("insert into test(name) values(?)", [['alice'], ['bob'], ['charlie']])
            cu.execute("select name from test")
            f = cu.fetchmany(types=[dbapi2.STRING])
            self.assertIsInstance(f[0][0], str)

    def test_fetchallTypesPositional(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name varchar(20))")
            cu.executemany("insert into test(name) values(?)", [['alice'], ['bob'], ['charlie']])
            cu.execute("select name from test")
            f = cu.fetchall(types=[dbapi2.STRING])
            self.assertIsInstance(f[0][0], str)

    def testTypesPositionalBAD(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name varchar(20))")
            cu.executemany("insert into test(name) values(?)", [['alice'], ['bob'], ['charlie']])
            cu.execute("select name from test")
            with self.assertRaises(dbapi2.ProgrammingError):
                f = cu.fetchone(types=[dbapi2.STRING, dbapi2.STRING])
            with self.assertRaises(dbapi2.ProgrammingError):
                f = cu.fetchone(types=[])


class GettersTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")
        self.testdata = b"abcdefg" * 10
        self.params = memoryview(zlib.compress(self.testdata))

    def tearDown(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            try:
                cur.execute("drop table test")
            except dbapi2.Error:
                pass


@common.unittest.skip
class TransactionsTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def tearDown(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            try:
                cur.execute("drop table test")
            except dbapi2.Error:
                pass

    def testTransactionRollbackCreate(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cx.commit()  # needed before starting transactions
            cu.execute("begin")
            cu.execute("create table test(name VARCHAR(10))")
            cx.rollback()
            # with self.assertRaises(dbapi2.ProgrammingError):
            cu.execute("select * from test")

    def testTransactionRollbackInsert(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cx.commit()  # needed before starting transactions
            cu.execute("create table test(name VARCHAR(10))")
            cx.commit()
            cu.execute("begin")
            cu.execute("insert into test(name) values('alice')")
            cx.rollback()
            # Alice should go away
            result = cu.execute("select * from test").fetchall()
            self.assertEqual(result, [])

    def testTransactionRollbackClose(self):
        with dbapi2.connect(db_name) as cx2:
            with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
                cx.commit()  # needed before starting transactions
                cu.execute("create table test(name VARCHAR(10))")
                cu.execute("insert into test(name) values('alice')")
                cx.commit()
                cu.execute("begin")
                cu.execute("insert into test(name) values('bob')")
                result = cu.execute("select * from test").fetchall()
                self.assertEqual(result, [['alice'], ['bob']])
                # Bob should go away
            with cx2.cursor() as cu:
                result = cu.execute("select * from test").fetchall()
                self.assertEqual(result, [['alice']])


class TypeTestCase(common.JPypeTestCase):
    """ This test is db dependent, but needed to check all
    code paths. """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def tearDown(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            try:
                cur.execute("drop table test")
            except dbapi2.Error:
                pass

    def testTime(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(NAME TIME)")
            cu.execute("select * from test")
            cu.execute("insert into test(NAME) values('12:05:10')")
            f = cu.execute('select * from test').fetchone()
            self.assertIsInstance(f[0], datetime.time)
            cu.execute("delete from test")
            cu.execute("insert into test(NAME) values(?)", f)
            f2 = cu.execute('select * from test').fetchone()
            self.assertEqual(f2, f)
            cu.execute("delete from test")

            # Test with Java
            t1 = jpype.java.sql.Time(1, 2, 3)
            cu.execute("insert into test(NAME) values(?)", [t1])
            f3 = cu.execute('select * from test').fetchone()
            self.assertEqual(f3[0], datetime.time(1, 2, 3))
            f4 = cu.execute('select * from test').fetchone(converters=None)
            self.assertEqual(f4[0], t1)
            self.assertIsInstance(f4[0], jpype.java.sql.Time)
            cu.execute("delete from test")

            # Test with dbapi2.Time
            t1 = dbapi2.Time(6, 21, 32)
            cu.execute("insert into test(NAME) values(?)", [t1])
            f3 = cu.execute('select * from test').fetchone()
            self.assertEqual(f3[0], datetime.time(6, 21, 32))

    def testDate(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(NAME DATE)")
            cu.execute("select * from test")
            cu.execute("insert into test(NAME) values('2012-02-05')")
            f = cu.execute('select * from test').fetchone()
            self.assertIsInstance(f[0], datetime.date)
            cu.execute("delete from test")
            cu.execute("insert into test(NAME) values(?)", f)
            f2 = cu.execute('select * from test').fetchone()
            self.assertEqual(f2, f)
            cu.execute("delete from test")

            # Test with Java
            t1 = java.sql.Date(1972 - 1900, 4 - 1, 1)
            cu.execute("insert into test(NAME) values(?)", [t1])
            f3 = cu.execute('select * from test').fetchone()
            self.assertEqual(f3[0], datetime.date(1972, 4, 1))
            f4 = cu.execute('select * from test').fetchone(converters=None)
            self.assertEqual(f4[0], t1)
            self.assertIsInstance(f4[0], java.sql.Date)
            cu.execute("delete from test")

            # Test with dbapi2.Date
            t1 = dbapi2.Date(2020, 5, 21)
            cu.execute("insert into test(NAME) values(?)", [t1])
            f3 = cu.execute('select * from test').fetchone()
            self.assertEqual(f3[0], datetime.date(2020, 5, 21))

    def testTimestamp(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(NAME TIMESTAMP)")
            cu.execute("select * from test")
            cu.execute("insert into test(NAME) values('2012-02-05 12:02:45.123')")
            f = cu.execute('select * from test').fetchone()
            self.assertIsInstance(f[0], datetime.date)
            cu.execute("delete from test")
            cu.execute("insert into test(NAME) values(?)", f)
            f2 = cu.execute('select * from test').fetchone()
            self.assertEqual(f2, f)
            cu.execute("delete from test")

            # Test with Java
            t1 = java.sql.Timestamp(1972 - 1900, 4 - 1, 1, 12, 2, 45, 123456000)
            cu.execute("insert into test(NAME) values(?)", [t1])
            f3 = cu.execute('select * from test').fetchone()
            self.assertEqual(f3[0], datetime.datetime(1972, 4, 1, 12, 2, 45, 123456))
            f4 = cu.execute('select * from test').fetchone(converters=None)
            self.assertEqual(f4[0], t1)
            self.assertIsInstance(f4[0], java.sql.Timestamp)
            cu.execute("delete from test")

            # Test with dbapi2.Date
            t1 = dbapi2.Timestamp(2020, 5, 21, 3, 4, 5, 123122000)
            cu.execute("insert into test(NAME) values(?)", [t1])
            f3 = cu.execute('select * from test').fetchone()
            self.assertEqual(f3[0], datetime.datetime(2020, 5, 21, 3, 4, 5, 123122))

    def _testInt(self, tp, desc, jtype, null=True):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(NAME %s)" % tp)
            cu.execute("insert into test(NAME) values(?)", [123])
            f = cu.execute("select * from test").fetchone()
            self.assertEqual(f[0], 123)
            self.assertIsInstance(f[0], int)
            self.assertEqual(cu.description[0][0:2], desc)
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], 123)
            self.assertIsInstance(f[0], jtype)
            cu.execute("delete from test")
            if null:
                cu.execute("insert into test(NAME) values(?)", [None])
                f = cu.execute("select * from test").fetchone(converters=None)
                self.assertEqual(f[0], None)

    def testTinyInt(self):
        self._testInt('TINYINT', ('NAME', 'TINYINT'), jpype.JShort)

    def testBigInt(self):
        self._testInt('BIGINT', ('NAME', 'BIGINT'), jpype.JLong)

    def testIdentity(self):
        self._testInt('IDENTITY', ('NAME', 'INTEGER'), jpype.JInt, False)

    def testInteger(self):
        self._testInt('INTEGER', ('NAME', 'INTEGER'), jpype.JInt)

    def testSmallInt(self):
        self._testInt('SMALLINT', ('NAME', 'SMALLINT'), jpype.JShort)

    def _testFloat(self, tp, desc, jtype):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(NAME %s)" % tp)
            cu.execute("insert into test(NAME) values(?)", [1.25])
            f = cu.execute("select * from test").fetchone()
            self.assertEqual(f[0], 1.25)
            self.assertIsInstance(f[0], float)
            self.assertEqual(cu.description[0][0:2], desc)
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], 1.25)
            self.assertIsInstance(f[0], jtype)
            cu.execute("delete from test")
            cu.execute("insert into test(NAME) values(?)", [None])
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], None)

    def testFloat(self):
        self._testFloat('FLOAT', ('NAME', 'DOUBLE'), jpype.JDouble)

    def testDouble(self):
        self._testFloat('DOUBLE', ('NAME', 'DOUBLE'), jpype.JDouble)

    def testReal(self):
        self._testFloat('REAL', ('NAME', 'DOUBLE'), jpype.JDouble)

    def _testNumeric(self, tp, desc, jtype):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(NAME %s(5,2))" % tp)
            cu.execute("insert into test(NAME) values(?)", [1.25])
            f = cu.execute("select * from test").fetchone()
            self.assertEqual(f[0], 1.25)
            self.assertIsInstance(f[0], decimal.Decimal)
            self.assertEqual(cu.description[0][0:2], desc)
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], decimal.Decimal(1.25))
            self.assertIsInstance(f[0], jtype)
            cu.execute("delete from test")
            cu.execute("insert into test(NAME) values(?)", [None])
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], None)

    def testNumeric(self):
        self._testNumeric('NUMERIC', ('NAME', 'NUMERIC'), java.math.BigDecimal)

    def testDecimal(self):
        self._testNumeric('DECIMAL', ('NAME', 'DECIMAL'), java.math.BigDecimal)

    def _testBinary(self, tp, desc, jtype):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(NAME %s)" % tp)
            v = bytes([1, 2, 3])
            cu.execute("insert into test(NAME) values(?)", [v])
            f = cu.execute("select * from test").fetchone()
            self.assertEqual(f[0], v)
            self.assertIsInstance(f[0], bytes)
            self.assertEqual(cu.description[0][0:2], desc)
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(bytes(f[0]), v)
            self.assertIsInstance(f[0], jtype)
            cu.execute("delete from test")
            cu.execute("insert into test(NAME) values(?)", [None])
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], None)

    def testBinary(self):
        self._testBinary('BINARY(3)', ('NAME', 'BINARY'), JArray(JByte))

    def testBlob(self):
        self._testBinary('BLOB', ('NAME', 'BLOB'), JArray(JByte))

    def testLongVarBinary(self):
        self._testBinary('LONGVARBINARY', ('NAME', 'VARBINARY'), JArray(JByte))

    def testVarBinary(self):
        self._testBinary('VARBINARY(10)', ('NAME', 'VARBINARY'), JArray(JByte))

    def _testChars(self, tp, desc, jtype, v="hello"):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(NAME %s)" % tp)
            cu.execute("insert into test(NAME) values(?)", [v])
            f = cu.execute("select * from test").fetchone()
            self.assertEqual(f[0], v)
            self.assertIsInstance(f[0], str)
            self.assertEqual(cu.description[0][0:2], desc)
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], v)
            self.assertIsInstance(f[0], jtype)
            cu.execute("delete from test")
            cu.execute("insert into test(NAME) values(?)", [None])
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], None)

    def testChar(self):
        self._testChars('CHAR', ('NAME', 'CHARACTER'), java.lang.String, 'a')

    def testVarChar(self):
        self._testChars('VARCHAR(10)', ('NAME', 'VARCHAR'), java.lang.String)

    def testLongVarChar(self):
        self._testChars('LONGVARCHAR', ('NAME', 'VARCHAR'), java.lang.String)

    def testClob(self):
        self._testChars('CLOB', ('NAME', 'CLOB'), java.lang.String)

    def testBoolean(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(NAME BOOLEAN)")
            cu.execute("insert into test(NAME) values(?)", [True])
            f = cu.execute("select * from test").fetchone()
            self.assertEqual(f[0], True)
            self.assertIsInstance(f[0], int)
            self.assertEqual(cu.description[0][0:2], ('NAME', 'BOOLEAN'))
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], True)
            self.assertIsInstance(f[0], type(True))
            cu.execute("delete from test")
            cu.execute("insert into test(NAME) values(?)", [None])
            f = cu.execute("select * from test").fetchone(converters=None)
            self.assertEqual(f[0], None)


class ThreadingTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def tearDown(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            try:
                cur.execute("drop table test")
            except dbapi2.Error:
                pass

    def _testThread(self, cu, cmd, args, exc):
        error = []

        def run():
            try:
                cmd(*args)
            except Exception as ex:
                error.append(ex)
        t = threading.Thread(target=run)
        t.start()
        t.join()
        self.assertEqual(len(error), 1)
        with self.assertRaises(exc):
            raise error[0]

    def test_execute(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name VARCHAR(10))")
            self._testThread(cu, cu.execute, ("insert into test(name) values ('a')",), dbapi2.ProgrammingError)

    def test_executemany(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name VARCHAR(10))")
            self._testThread(cu, cu.executemany, ("insert into test(name) values ('?')", [[1]]), dbapi2.ProgrammingError)

    def test_fetchone(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name VARCHAR(10))")
            cu.execute("insert into test(name) values('a')")
            self._testThread(cu, cu.fetchone, (), dbapi2.ProgrammingError)

    def test_fetchmany(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name VARCHAR(10))")
            cu.execute("insert into test(name) values('a')")
            self._testThread(cu, cu.fetchmany, (), dbapi2.ProgrammingError)

    def test_fetchall(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name VARCHAR(10))")
            cu.execute("insert into test(name) values('a')")
            self._testThread(cu, cu.fetchall, (), dbapi2.ProgrammingError)

    def test_close(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name VARCHAR(10))")
            cu.execute("insert into test(name) values('a')")
            self._testThread(cu, cu.close, (), dbapi2.ProgrammingError)

    def test_callproc(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cu:
            cu.execute("create table test(name VARCHAR(10))")
            cu.execute("insert into test(name) values('a')")
            self._testThread(cu, cu.callproc, ("lower", ["FOO"]), dbapi2.ProgrammingError)
