# This file is Public Domain and may be used without restrictions.
import common
import jpype.dbapi2 as dbapi2

try:
    from dbutils.pooled_db import PooledDB
    gotDBUtils = True
except ImportError:
    gotDBUtils = False


def haveDBUtils():
    return gotDBUtils


db_name = "jdbc:sqlite::memory:"


@common.unittest.skipUnless(haveDBUtils(), "DBUtils not available")
class DBUtilsPooledDBTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def test_basic_pool_use(self):
        pool = PooledDB(creator=dbapi2, maxconnections=3, dsn=db_name)
        cx = pool.connection()
        cur = cx.cursor()
        cur.execute("create table t (id integer)")
        cur.execute("insert into t values (1)")
        cur.execute("select * from t")
        self.assertEqual(cur.fetchall(), [[1]])
        cx.close()

    def test_pool_reuses_connections(self):
        # PooledDB.connection().close() should return the connection to
        # the pool rather than actually closing the underlying JDBC
        # connection each time.
        real_connects = []
        orig_connect = dbapi2.connect

        def counting_connect(*a, **kw):
            cx = orig_connect(*a, **kw)
            real_connects.append(cx)
            return cx

        class Wrapper:
            connect = staticmethod(counting_connect)
            paramstyle = dbapi2.paramstyle
            threadsafety = dbapi2.threadsafety
            Error = dbapi2.Error

        pool = PooledDB(creator=Wrapper, maxconnections=2, dsn=db_name)
        for _ in range(10):
            cx = pool.connection()
            cur = cx.cursor()
            cur.execute("select 1")
            cur.fetchall()
            cx.close()
        self.assertLessEqual(len(real_connects), 2)
