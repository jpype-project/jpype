# This file is Public Domain and may be used without restrictions.
import _jpype
import jpype
from jpype.types import *
from jpype import java
import jpype.dbapi2 as dbapi2
import common
import time

db_name = "jdbc:h2:mem:testdb"


class SQLModuleTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def assertIsSubclass(self, a, b):
        self.assertTrue(issubclass(a, b), "`%s` is not a subclass of `%s`" % (a.__name__, b.__name__))

    def testConstants(self):
        self.assertEqual(dbapi2.apilevel, "2.0")
        self.assertEqual(dbapi2.threadsafety, 2)
        self.assertEqual(dbapi2.paramstyle, "qmark")

    def testExceptions(self):
        self.assertIsSubclass(dbapi2.Warning, Exception)
        self.assertIsSubclass(dbapi2.Error, Exception)
        self.assertIsSubclass(dbapi2.InterfaceError, dbapi2.Error)
        self.assertIsSubclass(dbapi2.DatabaseError, dbapi2.Error)
        self.assertIsSubclass(dbapi2._SQLException, dbapi2.Error)
        self.assertIsSubclass(dbapi2.DataError, dbapi2.DatabaseError)
        self.assertIsSubclass(dbapi2.OperationalError, dbapi2.DatabaseError)
        self.assertIsSubclass(dbapi2.IntegrityError, dbapi2.DatabaseError)
        self.assertIsSubclass(dbapi2.InternalError, dbapi2.DatabaseError)
        self.assertIsSubclass(dbapi2.InternalError, dbapi2.DatabaseError)
        self.assertIsSubclass(dbapi2.ProgrammingError, dbapi2.DatabaseError)
        self.assertIsSubclass(dbapi2.NotSupportedError, dbapi2.DatabaseError)

    def test_Date(self):
        d1 = dbapi2.Date(2002, 12, 25)  # noqa F841
        d2 = dbapi2.DateFromTicks(  # noqa F841
            time.mktime((2002, 12, 25, 0, 0, 0, 0, 0, 0))
        )
        # Can we assume this? API doesn't specify, but it seems implied
        # self.assertEqual(str(d1),str(d2))

    def test_Time(self):
        t1 = dbapi2.Time(13, 45, 30)  # noqa F841
        t2 = dbapi2.TimeFromTicks(  # noqa F841
            time.mktime((2001, 1, 1, 13, 45, 30, 0, 0, 0))
        )
        # Can we assume this? API doesn't specify, but it seems implied
        # self.assertEqual(str(t1),str(t2))

    def test_Timestamp(self):
        t1 = dbapi2.Timestamp(2002, 12, 25, 13, 45, 30)  # noqa F841
        t2 = dbapi2.TimestampFromTicks(  # noqa F841
            time.mktime((2002, 12, 25, 13, 45, 30, 0, 0, 0))
        )
        # Can we assume this? API doesn't specify, but it seems implied
        # self.assertEqual(str(t1),str(t2))

    def test_Binary(self):
        b = dbapi2.Binary(b"Something")
        b = dbapi2.Binary(b"")  # noqa F841

    def test_STRING(self):
        self.assertTrue(hasattr(dbapi2, "STRING"), "module.STRING must be defined")

    def test_BINARY(self):
        self.assertTrue(
            hasattr(dbapi2, "BINARY"), "module.BINARY must be defined."
        )

    def test_NUMBER(self):
        self.assertTrue(
            hasattr(dbapi2, "NUMBER"), "module.NUMBER must be defined."
        )

    def test_DATETIME(self):
        self.assertTrue(
            hasattr(dbapi2, "DATETIME"), "module.DATETIME must be defined."
        )

    def test_ROWID(self):
        self.assertTrue(hasattr(dbapi2, "ROWID"), "module.ROWID must be defined.")


class SQLConnectTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

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

    def testGetters(self):
        m = {}
        cx = dbapi2.connect(db_name, getters=m)
        self.assertEqual(cx.getters, m)
        cx = dbapi2.connect(db_name)
        self.assertEqual(cx.getters, dbapi2._default_getters)
        # Passing None gives default
        cx = dbapi2.connect(db_name, getters=None)
        self.assertEqual(cx.getters, dbapi2._default_getters)

    def testSetters(self):
        m = {}
        cx = dbapi2.connect(db_name, setters=m)
        self.assertEqual(cx.setters, m)
        cx = dbapi2.connect(db_name)
        self.assertEqual(cx.setters, dbapi2._default_setters)
        # Passing None gives default
        cx = dbapi2.connect(db_name, setters=None)
        self.assertEqual(cx.setters, dbapi2._default_setters)

    def testAdapters(self):
        m = {}
        cx = dbapi2.connect(db_name, adapters=m)
        self.assertEqual(cx.adapters, m)
        cx = dbapi2.connect(db_name)
        self.assertEqual(cx.adapters, dbapi2._default_adapters)

    def testConverters(self):
        m = {}
        cx = dbapi2.connect(db_name)
        self.assertEqual(cx.converters, dbapi2._default_converters)
        cx = dbapi2.connect(db_name, converters=m)
        self.assertEqual(cx.converters, m)

    def testExceptions(self):
        cx = dbapi2.connect(db_name)
        self.assertEqual(cx.Warning, dbapi2.Warning)
        self.assertEqual(cx.Error, dbapi2.Error)
        self.assertEqual(cx.InterfaceError, dbapi2.InterfaceError)
        self.assertEqual(cx.DatabaseError, dbapi2.DatabaseError)
        self.assertEqual(cx.DataError, dbapi2.DataError)
        self.assertEqual(cx.OperationalError, dbapi2.OperationalError)
        self.assertEqual(cx.IntegrityError, dbapi2.IntegrityError)
        self.assertEqual(cx.InternalError, dbapi2.InternalError)
        self.assertEqual(cx.InternalError, dbapi2.InternalError)
        self.assertEqual(cx.ProgrammingError, dbapi2.ProgrammingError)
        self.assertEqual(cx.NotSupportedError, dbapi2.NotSupportedError)

    def testConverterKey(self):
        cx = dbapi2.connect(db_name, converterkeys=dbapi2.BY_JDBCTYPE)
        self.assertEqual(cx._keystyle, (dbapi2.BY_JDBCTYPE,))
        cx = dbapi2.connect(db_name, converterkeys=dbapi2.BY_COLNAME)
        self.assertEqual(cx._keystyle, (dbapi2.BY_COLNAME,))
        cx = dbapi2.connect(db_name, converterkeys=(dbapi2.BY_COLNAME, dbapi2.BY_JDBCTYPE))
        self.assertEqual(cx._keystyle, (dbapi2.BY_COLNAME, dbapi2.BY_JDBCTYPE))


class SQLConnectionTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

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


class SQLCursorTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

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
            self.assertTrue(
                cur.rowcount in (-1, 1),
                "cursor.rowcount should == number or rows inserted, or "
                "set to -1 after executing an insert statement",
            )
            cur.execute("select name from booze")
            self.assertTrue(
                cur.rowcount in (-1, 1),
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
            self.assertTrue(
                cur.rowcount in (-1, 2),
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

    def test_fetchone(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            # cursor.fetchone should raise an Error if called before
            # executing a select-type query
            self.assertRaises(dbapi2.Error, cur.fetchone)

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
            self.assertTrue(cur.rowcount in (-1, 0))

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
            self.assertTrue(cur.rowcount in (-1, 1))

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
            self.assertRaises(dbapi2.Error, cur.fetchmany, 4)

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
            self.assertTrue(cur.rowcount in (-1, 6))

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
            self.assertTrue(cur.rowcount in (-1, 6))

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
            self.assertTrue(cur.rowcount in (-1, 6))

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
            self.assertRaises(dbapi2.Error, cur.fetchall)

            cur.execute("select name from booze")
            rows = cur.fetchall()
            self.assertTrue(cur.rowcount in (-1, len(self.samples)))
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
            self.assertTrue(cur.rowcount in (-1, len(self.samples)))

            cur.execute("create table barflys (name varchar(20))")
            cur.execute("select name from barflys")
            rows = cur.fetchall()
            #self.assertTrue(cur.rowcount in (-1, 0))
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
            rows1 = cur.fetchone()
            rows23 = cur.fetchmany(2)
            rows4 = cur.fetchone()
            rows56 = cur.fetchall()
            self.assertTrue(cur.rowcount in (-1, 6))
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


#    def test_callproc(self):
#        con = self._connect()
#        try:
#            cur = con.cursor()
#            if self.lower_func and hasattr(cur, "callproc"):
#                r = cur.callproc(self.lower_func, ("FOO",))
#                self.assertEqual(len(r), 1)
#                self.assertEqual(r[0], "FOO")
#                r = cur.fetchall()
#                self.assertEqual(len(r), 1, "callproc produced no result set")
#                self.assertEqual(len(r[0]), 1, "callproc produced invalid result set")
#                self.assertEqual(r[0][0], "foo", "callproc produced invalid results")
#        finally:
#            con.close()
#    def help_nextset_setUp(self, cur):
#        """ Should create a procedure called deleteme
#            that returns two result sets, first the
#            number of rows in booze then "name from booze"
#        """
#        raise NotImplementedError("Helper not implemented")
#        # sql="""
#        #    create procedure deleteme as
#        #    begin
#        #        select count(*) from booze
#        #        select name from booze
#        #    end
#        # """
#        # cur.execute(sql)
#
#    def help_nextset_tearDown(self, cur):
#        "If cleaning up is needed after nextSetTest"
#        raise NotImplementedError("Helper not implemented")
#        # cur.execute("drop procedure deleteme")
#
# Need a database with stored procedures
#    def test_nextset(self):
#        con = self._connect()
#        try:
#            cur = con.cursor()
#            if not hasattr(cur, "nextset"):
#                return
#
#            try:
#                self.executeDDL1(cur)
#                sql = self._populate()
#                for sql in self._populate():
#                    cur.execute(sql)
#
#                self.help_nextset_setUp(cur)
#
#                cur.callproc("deleteme")
#                numberofrows = cur.fetchone()
#                assert numberofrows[0] == len(self.samples)
#                assert cur.nextset()
#                names = cur.fetchall()
#                assert len(names) == len(self.samples)
#                s = cur.nextset()
#                assert s is None, "No more return sets, should return None"
#            finally:
#                self.help_nextset_tearDown(cur)
#
#        finally:
#            con.close()
#
#    def test_nextset(self):  # noqa: F811
#        raise NotImplementedError("Drivers need to override this test")
#    def test_setoutputsize(self):
#        # Real test for setoutputsize is driver dependant
#        raise NotImplementedError("Driver need to override this test")
