// --- file: org/jpype/GilConcurrencyParityNGTest.java ---
package org.jpype;

import static org.testng.Assert.*;
import org.testng.annotations.Test;

import org.jpype.internal.NativeLauncherControl;
import python.lang.PyBuiltIn;
import python.lang.PyCallable;
import python.lang.PyTestHarness;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Study A (plan/JpypeDeRiskingStudy.md, deephaven-core): does JPype's
 * automatic per-call GIL guard ({@code JPPyCallAcquire}) hold up under
 * many uncoordinated Java threads calling into the single shared
 * {@link MainInterpreter}, with no synchronization on the Java side beyond
 * what the guard itself provides?
 *
 * Parity baseline: jpy's {@code MultiThreadedEvalTestFixture} +
 * {@code jpy_mt_eval_exec_test.py} (~/devel/jpy/src/test/), which spawns
 * {@code NUM_THREADS} Java threads each calling
 * {@code PyObject.executeCode(...)} against one shared globals/locals
 * dict, no external synchronization. {@code NUM_THREADS = 20} here matches
 * jpy's constant so the two suites are an apples-to-apples comparison, not
 * just "a" concurrency test.
 */
public class GilConcurrencyParityNGTest extends PyTestHarness
{
  private static final int NUM_THREADS = 20;

  /**
   * CPU-bound workload (mirrors jpy's {@code count_primes}) run
   * concurrently from N threads against the one shared {@code Script}
   * (globals/locals) with zero Java-side synchronization. Passes only if
   * every thread's result is independently correct - a shared/corrupted
   * interpreter state under a torn GIL would show up as wrong counts, not
   * just crashes.
   */
  @Test(timeOut = 120_000)
  public void testConcurrentEvalNoCorruption() throws InterruptedException
  {
    context.exec(
            "def is_prime(n):\n"
            + "    if n <= 1:\n"
            + "        return False\n"
            + "    i = 2\n"
            + "    while i * i <= n:\n"
            + "        if n % i == 0:\n"
            + "            return False\n"
            + "        i += 1\n"
            + "    return True\n"
            + "def count_primes(start, end):\n"
            + "    c = 0\n"
            + "    for i in range(start, end):\n"
            + "        if is_prime(i):\n"
            + "            c += 1\n"
            + "    return c\n");

    final int RANGE_PER_THREAD = 20_000;
    final int EXPECTED = referenceCountPrimes(0, RANGE_PER_THREAD);

    List<Thread> threads = new ArrayList<>();
    AtomicBoolean sawFailure = new AtomicBoolean(false);
    AtomicInteger mismatches = new AtomicInteger(0);
    AtomicInteger gilLeaks = new AtomicInteger(0);
    List<Throwable> errors = java.util.Collections.synchronizedList(new ArrayList<>());

    for (int t = 0; t < NUM_THREADS; t++)
    {
      Thread thread = new Thread(() ->
      {
        try
        {
          PyCallable countPrimes = (PyCallable) context.eval("count_primes");
          Object result = countPrimes.call(0, RANGE_PER_THREAD);
          long value = ((Number) ((python.lang.PyInt) result).toNumber()).longValue();
          if (value != EXPECTED)
            mismatches.incrementAndGet();
          if (NativeLauncherControl.isGilHeld())
            gilLeaks.incrementAndGet();
        } catch (Throwable ex)
        {
          sawFailure.set(true);
          errors.add(ex);
        }
      });
      threads.add(thread);
    }

    for (Thread thread : threads)
      thread.start();
    for (Thread thread : threads)
      thread.join();

    if (!errors.isEmpty())
      errors.get(0).printStackTrace();

    assertFalse(sawFailure.get(), "at least one worker thread threw: " + errors);
    assertEquals(mismatches.get(), 0, "at least one worker thread computed a wrong prime count under contention");
    assertEquals(gilLeaks.get(), 0, "at least one worker thread left the GIL held after its call returned");
  }

  /**
   * Shared mutable Python state, mutated concurrently from N threads with
   * no Java-side lock - a lost update here (final count != expected) means
   * the automatic GIL guard is not actually serializing access the way a
   * manually-marked-@ConcurrentMethod caller would assume it doesn't need
   * to.
   */
  @Test(timeOut = 120_000, dependsOnMethods = "testConcurrentEvalNoCorruption")
  public void testConcurrentSharedStateMutationNoLostUpdates() throws InterruptedException
  {
    context.exec("shared_counter = 0\n"
            + "def bump():\n"
            + "    global shared_counter\n"
            + "    shared_counter = shared_counter + 1\n");

    final int ITERATIONS_PER_THREAD = 2_000;
    List<Thread> threads = new ArrayList<>();
    AtomicBoolean sawFailure = new AtomicBoolean(false);
    List<Throwable> errors = java.util.Collections.synchronizedList(new ArrayList<>());

    for (int t = 0; t < NUM_THREADS; t++)
    {
      Thread thread = new Thread(() ->
      {
        try
        {
          PyCallable bump = (PyCallable) context.eval("bump");
          for (int i = 0; i < ITERATIONS_PER_THREAD; i++)
            bump.invoker().execute();
        } catch (Throwable ex)
        {
          sawFailure.set(true);
          errors.add(ex);
        }
      });
      threads.add(thread);
    }

    for (Thread thread : threads)
      thread.start();
    for (Thread thread : threads)
      thread.join();

    if (!errors.isEmpty())
      errors.get(0).printStackTrace();
    assertFalse(sawFailure.get(), "at least one worker thread threw: " + errors);

    long finalValue = ((Number) ((python.lang.PyInt) context.eval("shared_counter")).toNumber()).longValue();
    assertEquals(finalValue, (long) NUM_THREADS * ITERATIONS_PER_THREAD,
            "lost update(s) under concurrent shared-state mutation - GIL guard did not fully serialize access");
  }

  /**
   * Control for {@link #testConcurrentSharedStateMutationNoLostUpdates}:
   * same total call count (NUM_THREADS * ITERATIONS_PER_THREAD), but from
   * a single thread with no concurrency at all. If this also fails to
   * reach the expected total, the bug is in global-state mutation
   * visibility through this {@code Script}/{@code PyCallable} API path in
   * general, not specifically a concurrency/GIL-serialization bug - an
   * important distinction the concurrent test alone can't make.
   */
  @Test(timeOut = 120_000, dependsOnMethods = "testConcurrentEvalNoCorruption")
  public void testSingleThreadedSharedStateMutationControl() throws Exception
  {
    context.exec("shared_counter_control = 0\n"
            + "def bump_control():\n"
            + "    global shared_counter_control\n"
            + "    shared_counter_control = shared_counter_control + 1\n");

    final int TOTAL_CALLS = NUM_THREADS * 2_000;
    PyCallable bump = (PyCallable) context.eval("bump_control");
    for (int i = 0; i < TOTAL_CALLS; i++)
      bump.invoker().execute();

    long finalValue = ((Number) ((python.lang.PyInt) context.eval("shared_counter_control")).toNumber()).longValue();
    assertEquals(finalValue, (long) TOTAL_CALLS,
            "single-threaded, non-concurrent repeated calls did not correctly mutate shared global state - "
            + "this points at a bug in global-state visibility through this API path generally, not a "
            + "concurrency/GIL-serialization bug specifically");
  }

  /**
   * jpy's {@code test_exec_import} analog: concurrent {@code import} of a
   * module not yet in {@code sys.modules}, from many threads at once, with
   * no external synchronization - exercises CPython's own import-lock
   * interaction with the automatic GIL guard, not just simple call/return.
   *
   * Uses a freshly-generated, uniquely-named module on disk rather than a
   * stdlib module name (e.g. {@code csv}) - {@code context} is a single
   * {@code Script} shared across every test class in the whole suite run
   * (see {@code PyTestHarness}), so any stdlib module name is liable to
   * already be in {@code sys.modules} by the time this test runs,
   * independent of suite ordering. A unique module guarantees this
   * actually exercises the uncached first-import path (and its lock)
   * every run, not just a {@code sys.modules} cache hit.
   */
  @Test(timeOut = 120_000, dependsOnMethods = "testConcurrentEvalNoCorruption")
  public void testConcurrentImportIsSafe() throws InterruptedException
  {
    String moduleName = "study_a_scratch_module_" + System.nanoTime();
    context.exec(
            "import sys, os, tempfile\n"
            + "_scratch_dir = tempfile.mkdtemp(prefix='study_a_import_')\n"
            + "with open(os.path.join(_scratch_dir, '" + moduleName + ".py'), 'w') as _f:\n"
            + "    _f.write('X = 1\\n')\n"
            + "sys.path.insert(0, _scratch_dir)\n"
            + "assert '" + moduleName + "' not in sys.modules, 'scratch module unexpectedly already imported'\n");

    List<Thread> threads = new ArrayList<>();
    AtomicBoolean sawFailure = new AtomicBoolean(false);
    List<Throwable> errors = java.util.Collections.synchronizedList(new ArrayList<>());
    CountDownLatch start = new CountDownLatch(1);

    for (int t = 0; t < NUM_THREADS; t++)
    {
      Thread thread = new Thread(() ->
      {
        try
        {
          start.await();
          context.exec("import " + moduleName + "\n");
        } catch (Throwable ex)
        {
          sawFailure.set(true);
          errors.add(ex);
        }
      });
      threads.add(thread);
    }

    for (Thread thread : threads)
      thread.start();
    start.countDown();
    for (Thread thread : threads)
      thread.join();

    if (!errors.isEmpty())
      errors.get(0).printStackTrace();
    assertFalse(sawFailure.get(), "concurrent import raced/threw: " + errors);
  }

  private static int referenceCountPrimes(int start, int end)
  {
    int c = 0;
    outer:
    for (int n = start; n < end; n++)
    {
      if (n <= 1)
        continue;
      for (int i = 2; (long) i * i <= n; i++)
        if (n % i == 0)
          continue outer;
      c++;
    }
    return c;
  }
}
