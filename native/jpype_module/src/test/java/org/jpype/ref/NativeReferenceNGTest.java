// --- file: org/jpype/ref/NativeReferenceNGTest.java ---
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
 * {@code hashCode}/{@code equals} are plain Java - constructing a
 * {@code NativeReference} itself needs no native library (it only stores
 * the host pointer/cleanup id fields; the native methods are never
 * invoked by the constructor).
 */
public class NativeReferenceNGTest
{

  @Test
  public void testHashCodeMatchesHostReference()
  {
    ReferenceQueue<Object> q = new ReferenceQueue<>();
    NativeReference ref = new NativeReference(q, new Object(), 12345L, 0L);
    assertEquals(ref.hashCode(), (int) 12345L);
  }

  @Test
  public void testEqualsFalseForNonNativeReference()
  {
    ReferenceQueue<Object> q = new ReferenceQueue<>();
    NativeReference ref = new NativeReference(q, new Object(), 1L, 0L);
    assertFalse(ref.equals("not a reference"));
  }

  @Test
  public void testEqualsTrueForSameHostReference()
  {
    ReferenceQueue<Object> q = new ReferenceQueue<>();
    NativeReference a = new NativeReference(q, new Object(), 7L, 0L);
    NativeReference b = new NativeReference(q, new Object(), 7L, 0L);
    assertTrue(a.equals(b));
  }
}
