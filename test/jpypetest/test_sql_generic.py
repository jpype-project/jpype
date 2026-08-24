# This file is Public Domain and may be used without restrictions.
import jpype.dbapi2 as dbapi2
import common
import time
import unittest.mock as mock



class ConnectionIsolationLevelTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def _connection(self):
        cx = object.__new__(dbapi2.Connection)
        cx._closed = False
        cx._jcx = mock.MagicMock()
        cx._jcx.isClosed.return_value = False
        return cx

    def test_isolation_level_get_set(self):
        cx = self._connection()
        cx._jcx.getTransactionIsolation.return_value = dbapi2.TRANSACTION_READ_COMMITTED
        self.assertEqual(cx.isolation_level, dbapi2.TRANSACTION_READ_COMMITTED)
        cx.isolation_level = dbapi2.TRANSACTION_SERIALIZABLE
        cx._jcx.setTransactionIsolation.assert_called_once_with(dbapi2.TRANSACTION_SERIALIZABLE)

    def test_isolation_level_unsupported_raises(self):
        cx = self._connection()
        cx._jcx.setTransactionIsolation.side_effect = dbapi2._SQLException("nope")
        with self.assertRaises(dbapi2.NotSupportedError):
            cx.isolation_level = dbapi2.TRANSACTION_SERIALIZABLE


class ConnectionCommitRollbackErrorTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def _connection(self):
        cx = object.__new__(dbapi2.Connection)
        cx._closed = False
        cx._jcx = mock.MagicMock()
        cx._jcx.isClosed.return_value = False
        cx._jcx.getAutoCommit.return_value = False
        return cx

    def test_commit_wraps_driver_exception(self):
        # Autocommit is off (the normal case), but the driver itself
        # rejects the commit -- this must surface as OperationalError
        # rather than the raw java.sql.SQLException.
        cx = self._connection()
        cx._jcx.commit.side_effect = dbapi2._SQLException("boom")
        with self.assertRaises(dbapi2.OperationalError):
            cx.commit()

    def test_rollback_wraps_driver_exception(self):
        cx = self._connection()
        cx._jcx.rollback.side_effect = dbapi2._SQLException("boom")
        with self.assertRaises(dbapi2.OperationalError):
            cx.rollback()


class ConnectionTypeinfoTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def test_typeinfo_unknown_data_type_raises(self):
        # A driver reporting a JDBC type code that isn't in the module's
        # type registry must surface as DatabaseError, not a raw KeyError.
        cx = object.__new__(dbapi2.Connection)
        cx._closed = False
        cx._jcx = mock.MagicMock()
        cx._jcx.isClosed.return_value = False

        rs = mock.MagicMock()
        rs.next.side_effect = [True, False]
        rs.getString.return_value = "WEIRDTYPE"
        rs.getInt.return_value = -999999  # not a registered JDBC type code

        type_info_cm = mock.MagicMock()
        type_info_cm.__enter__.return_value = rs
        type_info_cm.__exit__.return_value = False
        cx._jcx.getMetaData.return_value.getTypeInfo.return_value = type_info_cm

        with self.assertRaises(dbapi2.DatabaseError):
            cx.typeinfo


class CursorExecuteManyRepeatTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def _cursor(self, batch):
        cx = mock.MagicMock(spec=dbapi2.Connection)
        cx._jcx = mock.MagicMock()
        cx._jcx.isClosed.return_value = False
        cx._batch = batch
        cx._setters = mock.MagicMock()
        cur = dbapi2.Cursor(cx)
        stmt = mock.MagicMock()
        stmt.execute.return_value = False
        stmt.getParameterMetaData.return_value.getParameterCount.return_value = 0
        cx._jcx.prepareStatement.return_value = stmt
        return cur, stmt

    def test_executemany_uses_repeat_fallback_when_batch_unsupported(self):
        # When the driver doesn't support batch updates, executemany()
        # must fall back to executing each parameter set individually
        # via _executeRepeat rather than addBatch()/executeBatch().
        cur, stmt = self._cursor(batch=False)
        stmt.getUpdateCount.side_effect = [1, 1]
        cur.executemany("insert into t values (?)", [(), ()])
        self.assertEqual(cur.rowcount, 2)
        self.assertEqual(stmt.execute.call_count, 2)
        stmt.addBatch.assert_not_called()
        stmt.executeBatch.assert_not_called()

    def test_executemany_uses_batch_when_supported(self):
        cur, stmt = self._cursor(batch=True)
        stmt.executeBatch.return_value = [1, 1]
        cur.executemany("insert into t values (?)", [(), ()])
        self.assertEqual(cur.rowcount, 2)
        self.assertEqual(stmt.addBatch.call_count, 2)
        stmt.executeBatch.assert_called_once()


class SQLModuleTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def assertIsSubclass(self, a, b):
        self.assertTrue(issubclass(a, b), "`%s` is not a subclass of `%s`" % (a.__name__, b.__name__))

    def testConstants(self):
        self.assertEqual(dbapi2.apilevel, "2.0")
        self.assertEqual(dbapi2.threadsafety, 2)
        self.assertEqual(dbapi2.paramstyle, "qmark")
        self.assertEqual(dbapi2.TRANSACTION_NONE, 0)
        self.assertEqual(dbapi2.TRANSACTION_READ_UNCOMMITTED, 1)
        self.assertEqual(dbapi2.TRANSACTION_READ_COMMITTED, 2)
        self.assertEqual(dbapi2.TRANSACTION_REPEATABLE_READ, 4)
        self.assertEqual(dbapi2.TRANSACTION_SERIALIZABLE, 8)

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

    def testConnectionExceptions(self):
        cx = dbapi2.Connection
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


class SQLTablesTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testStr(self):
        for i in dbapi2._types:
            self.assertIsInstance(str(i), str)

    def testRepr(self):
        for i in dbapi2._types:
            self.assertIsInstance(repr(i), str)
