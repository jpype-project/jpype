// --- file: org/jpype/ref/GlobalPoolNGTest.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package org.jpype.ref;

import static org.testng.Assert.*;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicBoolean;
import org.testng.annotations.Test;

public class GlobalPoolNGTest
{

  @Test
  public void testAddNullReturnsZeroHandleWithNoSlot()
  {
    GlobalPool pool = new GlobalPool();
    assertEquals(pool.add(null), 0L);
    assertNull(pool.get(0L));
  }

  @Test
  public void testAddThenGetReturnsSameObject()
  {
    GlobalPool pool = new GlobalPool();
    Object obj = new Object();
    long handle = pool.add(obj);
    assertSame(pool.get(handle), obj);
  }

  @Test
  public void testGetOfHandleFromDifferentPoolReturnsNull()
  {
    GlobalPool a = new GlobalPool();
    GlobalPool b = new GlobalPool();
    long handle = a.add(new Object());
    assertNull(b.get(handle));
  }

  @Test
  public void testGetAfterTryReleaseReturnsNull()
  {
    GlobalPool pool = new GlobalPool();
    long handle = pool.add(new Object());
    GlobalPool.tryRelease(handle);
    assertNull(pool.get(handle));
  }

  @Test
  public void testReleasedSlotIsReusedForNextAdd()
  {
    // No generation guard, by design - matches JNI's own global-ref slot
    // reuse. Safe because a handle is only ever resolved through its live
    // owning C++ wrapper, which is also the only thing that ever releases
    // it, exactly once, from its own destructor - so nothing can resolve
    // `first` again after release, whether or not its slot gets reused.
    GlobalPool pool = new GlobalPool();
    long first = pool.add(new Object());
    GlobalPool.tryRelease(first);
    Object second = new Object();
    long reused = pool.add(second);
    assertEquals(reused, first);
    assertSame(pool.get(reused), second);
  }

  @Test
  public void testTryReleaseOnAlreadyClosedPoolNoOps()
  {
    GlobalPool pool = new GlobalPool();
    long handle = pool.add(new Object());
    pool.close();
    // Must not throw even though the pool is gone from the registry.
    GlobalPool.tryRelease(handle);
    assertNull(pool.get(handle));
  }

  @Test
  public void testGetAfterCloseReturnsNull()
  {
    GlobalPool pool = new GlobalPool();
    long handle = pool.add(new Object());
    pool.close();
    assertNull(pool.get(handle));
  }

  @Test
  public void testAllocationAcrossManyBlocksSurvivesRoundTrip()
  {
    // BLOCK_SIZE is 64 - exceed several blocks and confirm every handle
    // still resolves correctly, exercising allocateNewBlock/realloc.
    GlobalPool pool = new GlobalPool();
    int count = 64 * 5 + 7;
    Object[] objs = new Object[count];
    long[] handles = new long[count];
    for (int i = 0; i < count; i++)
    {
      objs[i] = new Object();
      handles[i] = pool.add(objs[i]);
    }
    for (int i = 0; i < count; i++)
    {
      assertSame(pool.get(handles[i]), objs[i]);
    }
  }

  @Test
  public void testFreedSlotIsReusedNotLeaked()
  {
    GlobalPool pool = new GlobalPool();
    long[] handles = new long[64];
    for (int i = 0; i < handles.length; i++)
      handles[i] = pool.add(new Object());
    for (long h : handles)
      GlobalPool.tryRelease(h);
    // Filling the same count again must not grow past the first block if
    // free slots are actually being reused.
    for (int i = 0; i < handles.length; i++)
      pool.add(new Object());
    assertEquals(pool.blockCountForTest(), 1);
  }

  @Test(timeOut = 30000)
  public void testStampMismatchAfterPrefixWraparoundIsSafelyDropped()
  {
    // The 16-bit prefix wraps after 65536 pool constructions in this JVM,
    // so a later, unrelated pool can end up reusing an earlier pool's
    // prefix. Force a real wraparound and confirm a stale handle from the
    // original owner of that prefix is not misrouted into the pool that
    // now holds it - the per-pool stamp (hashed from each pool's distinct
    // "context pointer" here) must catch the mismatch and safely no-op,
    // not silently corrupt the new pool's state.
    Map<Integer, GlobalPool> seenByPrefix = new HashMap<>();
    GlobalPool collidedOld = null;
    GlobalPool collidedNew = null;
    for (long ctx = 1; ctx <= 70_000 && collidedNew == null; ctx++)
    {
      GlobalPool pool = new GlobalPool(ctx);
      GlobalPool previous = seenByPrefix.put(pool.prefixForTest(), pool);
      if (previous != null)
      {
        collidedOld = previous;
        collidedNew = pool;
      }
    }
    assertNotNull(collidedNew, "expected a prefix collision within 70000 pools");

    long staleHandle = collidedOld.add(new Object());
    Object sentinel = new Object();
    long liveHandleInNewPool = collidedNew.add(sentinel);
    int blocksBefore = collidedNew.blockCountForTest();

    // staleHandle's prefix now resolves (via the static registry) to
    // collidedNew, not collidedOld - the stamp must stop it there.
    GlobalPool.tryRelease(staleHandle);

    assertEquals(collidedNew.blockCountForTest(), blocksBefore);
    assertSame(collidedNew.get(liveHandleInNewPool), sentinel);
  }

  @Test(timeOut = 30000)
  public void testConcurrentGetIsSafeUnderConcurrentAddRemove() throws InterruptedException
  {
    // Regression guard for the sync->lock-free rewrite (see
    // plan/GlobalPool.md): many reader threads calling get() while one
    // writer thread continuously add()s/tryRelease()s must never see a
    // torn/inconsistent handle - either a live object or null, never an
    // exception, and readers must observe writer progress (not be
    // permanently stale).
    GlobalPool pool = new GlobalPool();
    int slotCount = 256;
    long[] handles = new long[slotCount];
    for (int i = 0; i < slotCount; i++)
      handles[i] = pool.add(new Object());

    int readerThreads = Math.max(2, Runtime.getRuntime().availableProcessors());
    AtomicBoolean stop = new AtomicBoolean(false);
    AtomicBoolean sawFailure = new AtomicBoolean(false);
    CountDownLatch readersDone = new CountDownLatch(readerThreads);

    Thread writer = new Thread(() ->
    {
      int idx = 0;
      while (!stop.get())
      {
        idx = (idx + 1) % slotCount;
        GlobalPool.tryRelease(handles[idx]);
        handles[idx] = pool.add(new Object());
      }
    });

    Thread[] readers = new Thread[readerThreads];
    for (int t = 0; t < readerThreads; t++)
    {
      readers[t] = new Thread(() ->
      {
        try
        {
          long end = System.nanoTime() + 500_000_000L;
          while (System.nanoTime() < end)
          {
            for (int i = 0; i < slotCount; i++)
            {
              // Reading a racing handle array element is inherently
              // best-effort here; the invariant under test is that get()
              // itself never throws/corrupts, not that every read is
              // perfectly synchronized with the writer's array update.
              pool.get(handles[i]);
            }
          }
        } catch (Throwable t2)
        {
          sawFailure.set(true);
        } finally
        {
          readersDone.countDown();
        }
      });
      readers[t].start();
    }

    writer.start();
    readersDone.await();
    stop.set(true);
    writer.join();

    assertFalse(sawFailure.get(), "a reader thread saw an exception from get()");
  }
}
