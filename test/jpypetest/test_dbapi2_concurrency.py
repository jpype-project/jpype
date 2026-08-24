# This file is Public Domain and may be used without restrictions.
import common
import jpype.dbapi2 as dbapi2
import concurrent.futures
import threading

db_name = "jdbc:sqlite::memory:"


class ConcurrencyTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")

    def test_concurrent_futures_separate_connections(self):
        # Each worker opens its own connection (dbapi2.Connection objects
        # are bound to the thread that created them, see threadsafety in
        # the dbapi2 guide). This exercises JPype's JNI thread
        # attach/detach across a ThreadPoolExecutor, including reused pool
        # threads handling multiple tasks in sequence.
        errors = []

        def worker(i):
            try:
                with dbapi2.connect(db_name) as cx, cx.cursor() as cur:
                    cur.execute("create table t (id integer)")
                    for j in range(20):
                        cur.execute("insert into t values (?)", (j,))
                    cur.execute("select count(*) from t")
                    (count,) = cur.fetchone()
                    return (i, threading.get_ident(), count)
            except Exception as ex:
                errors.append((i, ex))
                raise

        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
            results = list(pool.map(worker, range(50)))

        self.assertEqual(errors, [])
        self.assertEqual(len(results), 50)
        for i, tid, count in results:
            self.assertEqual(count, 20)
        # Confirms the pool actually reused worker threads across tasks.
        idents = set(r[1] for r in results)
        self.assertLessEqual(len(idents), 8)
