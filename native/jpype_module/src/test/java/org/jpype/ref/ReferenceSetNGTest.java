// --- file: org/jpype/ref/ReferenceSetNGTest.java ---
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

import java.lang.ref.ReferenceQueue;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * {@code ReferenceSet}/{@code ReferenceSet.Pool} are pure bookkeeping - no
 * native library needed - except for {@link ReferenceSet#flush} actually
 * calling the real {@code NativeReference.removeHostReference} for any
 * entry with a nonzero {@code cleanup}. That real call needs a genuine
 * native function pointer (it's dereferenced and invoked on the C++ side);
 * fabricating one would risk crashing the JVM, so this suite only ever
 * uses {@code cleanup == 0} entries, which are guaranteed by construction
 * to short-circuit before that call.
 */
public class ReferenceSetNGTest
{

  @Test
  public void testSizeStartsAtZero()
  {
    ReferenceSet set = new ReferenceSet(0L);
    assertEquals(set.size(), 0);
  }

  @Test
  public void testAddIgnoresZeroCleanupReferences()
  {
    ReferenceSet set = new ReferenceSet(0L);
    ReferenceQueue<Object> q = new ReferenceQueue<>();
    NativeReference ref = new NativeReference(q, new Object(), 1L, 0L);
    set.add(ref);
    assertEquals(set.size(), 0);
  }

  @Test
  public void testRemoveIgnoresZeroCleanupReferences()
  {
    ReferenceSet set = new ReferenceSet(0L);
    ReferenceQueue<Object> q = new ReferenceQueue<>();
    NativeReference ref = new NativeReference(q, new Object(), 1L, 0L);
    // cleanup == 0 must short-circuit before ever touching ref.pool/pools,
    // which are not valid here (ref was never added).
    set.remove(ref);
  }

  /**
   * Directly seeds a {@code Pool} (bypassing the public {@code add}, which
   * refuses zero-cleanup entries) so {@link ReferenceSet#flush} has a
   * non-empty pool to iterate - exercising the loop body/continue/
   * tail-reset path without ever reaching the real native cleanup call.
   */
  @Test
  public void testFlushSkipsZeroCleanupEntriesAndResetsTail()
  {
    ReferenceSet set = new ReferenceSet(0L);
    ReferenceQueue<Object> q = new ReferenceQueue<>();
    NativeReference ref = new NativeReference(q, new Object(), 999L, 0L);

    ReferenceSet.Pool pool = new ReferenceSet.Pool(0);
    pool.entries[0] = ref;
    pool.tail = 1;
    set.pools.add(pool);

    set.flush();

    assertEquals(pool.tail, 0);
  }
}
