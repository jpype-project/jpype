# This file is Public Domain and may be used without restrictions.
import common
import jpype.dbapi2 as dbapi2

try:
    import polars as pl
    gotPolars = True
except ImportError:
    gotPolars = False


def havePolars():
    return gotPolars


db_name = "jdbc:sqlite::memory:"


@common.unittest.skipUnless(havePolars(), "polars not available")
class PolarsTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def test_read_database_connection(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table t (id integer, name varchar(20))")
            cur.executemany("insert into t values (?, ?)", [(1, "a"), (2, "b")])
            df = pl.read_database(query="select * from t", connection=cx)
            self.assertEqual(df.columns, ["id", "name"])
            self.assertEqual(df["id"].to_list(), [1, 2])
            self.assertEqual(df["name"].to_list(), ["a", "b"])

    def test_read_database_cursor(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table t (id integer, name varchar(20))")
            cur.executemany("insert into t values (?, ?)", [(1, "a"), (2, "b")])
            cur.execute("select * from t")
            df = pl.read_database(query="select * from t", connection=cx)
            self.assertEqual(len(df), 2)
