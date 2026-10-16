# This file is Public Domain and may be used without restrictions.
import common
import jpype.dbapi2 as dbapi2

try:
    import pandas as pd
    gotPandas = True
except ImportError:
    gotPandas = False


def havePandas():
    return gotPandas


db_name = "jdbc:sqlite::memory:"


@common.unittest.skipUnless(havePandas(), "pandas not available")
class PandasTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def test_dataframe_from_cursor(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table t (id integer, name varchar(20))")
            cur.executemany("insert into t values (?, ?)", [(1, "a"), (2, "b")])
            cur.execute("select * from t")
            columns = [d[0] for d in cur.description]
            df = pd.DataFrame(cur.fetchall(), columns=columns)
            self.assertEqual(list(df.columns), ["id", "name"])
            self.assertEqual(len(df), 2)
            self.assertEqual(list(df["id"]), [1, 2])
            self.assertEqual(list(df["name"]), ["a", "b"])

    def test_read_sql(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table t (id integer, name varchar(20))")
            cur.executemany("insert into t values (?, ?)", [(1, "a"), (2, "b")])
            df = pd.read_sql("select * from t", cx)
            self.assertEqual(len(df), 2)
            self.assertEqual(list(df.columns), ["id", "name"])

    def test_read_sql_with_params(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table t (id integer, name varchar(20))")
            cur.executemany(
                "insert into t values (?, ?)", [(1, "a"), (2, "b"), (3, "c")]
            )
            df = pd.read_sql("select * from t where id > ?", cx, params=(1,))
            self.assertEqual(len(df), 2)
            self.assertEqual(list(df["id"]), [2, 3])

    def test_dataframe_roundtrip(self):
        with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
            cur.execute("create table t (id integer, name varchar(20))")
            src = pd.DataFrame({"id": [1, 2, 3], "name": ["a", "b", "c"]})
            cur.executemany(
                "insert into t values (?, ?)",
                list(src.itertuples(index=False, name=None)),
            )
            cur.execute("select * from t")
            columns = [d[0] for d in cur.description]
            out = pd.DataFrame(cur.fetchall(), columns=columns)
            pd.testing.assert_frame_equal(out, src)
