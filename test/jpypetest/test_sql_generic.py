# This file is Public Domain and may be used without restrictions.
import _jpype
import jpype
from jpype.types import *
from jpype import java
import jpype.dbapi2 as dbapi2
import common
import time

try:
    import zlib
except ImportError:
    zlib = None


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
