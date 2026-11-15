// --- file: org/jpype/ref/NativeReferenceQueueNGTest.java ---
package org.jpype.ref;

import java.lang.reflect.Field;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

/**
 * Covers {@link NativeReferenceQueue#getQueueSize()}.
 *
 * <p>
 * {@code NativeReferenceQueue} can only be constructed with a live
 * {@code NativeContext} (its constructor immediately registers a real JNI
 * callback via {@code NativeReference.init}), so - like
 * {@code GlobalPoolNGTest}'s siblings that need native, but unlike
 * {@code ReferenceSetNGTest}/{@code NativeReferenceNGTest} which avoid it -
 * this suite reaches the queue through the already-running
 * {@link PyTestHarness#context}.</p>
 *
 * <p>
 * Growing the queue through its real production path
 * ({@code registerRef}/{@code ProxyType.newInstance}) requires a genuine
 * native cleanup function pointer bound to a real Python object; fabricating
 * one would risk a native crash either immediately or later when the JVM
 * shutdown hook flushes the queue. Instead this test verifies
 * {@code getQueueSize()}'s actual contract - that it is a live facade over
 * the backing {@code ReferenceSet}'s size - directly, the same way
 * {@code ReferenceSetNGTest} pokes at {@code ReferenceSet} internals from
 * within the same package, without ever touching native memory.</p>
 */
public class NativeReferenceQueueNGTest extends PyTestHarness
{

  @Test
  public void testGetQueueSizeReflectsBackingReferenceSetSize() throws Exception
  {
    NativeReferenceQueue queue = context.getContext().getReferenceQueue();
    int baseline = queue.getQueueSize();
    assertTrue(baseline >= 0, "queue size should never be negative");

    Field hostReferencesField = NativeReferenceQueue.class.getDeclaredField("hostReferences");
    hostReferencesField.setAccessible(true);
    ReferenceSet set = (ReferenceSet) hostReferencesField.get(queue);
    assertSame(set.size(), baseline);

    Field itemsField = ReferenceSet.class.getDeclaredField("items");
    itemsField.setAccessible(true);
    try
    {
      itemsField.setInt(set, baseline + 5);
      assertEquals(queue.getQueueSize(), baseline + 5);

      itemsField.setInt(set, baseline);
      assertEquals(queue.getQueueSize(), baseline);
    } finally
    {
      // Always restore the true count, regardless of assertion outcome -
      // this is the live, suite-lifetime queue used by every other test.
      itemsField.setInt(set, baseline);
    }
  }

  @Test
  public void testRegisterRefWithZeroCleanupDoesNotGrowQueueSize()
  {
    // Mirrors NativeReferenceQueue.registerRef's early-return on
    // cleanup == 0 - the one production call path that is safe to drive
    // for real here, since it is guaranteed to never touch native memory.
    NativeReferenceQueue queue = context.getContext().getReferenceQueue();
    int before = queue.getQueueSize();
    queue.registerRef(new Object(), 123456789L, 0L);
    assertEquals(queue.getQueueSize(), before);
  }
}
