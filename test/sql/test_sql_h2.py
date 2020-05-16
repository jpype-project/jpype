import _jpype
import jpype
from jpype.types import *
from jpype import java
import jpype.dbapi2 as dbapi2
import common

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
