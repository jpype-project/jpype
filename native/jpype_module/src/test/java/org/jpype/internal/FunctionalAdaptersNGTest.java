// --- file: org/jpype/internal/FunctionalAdaptersNGTest.java ---
/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package org.jpype.internal;

import java.util.Arrays;
import java.util.Iterator;
import java.util.Map;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class FunctionalAdaptersNGTest
{

  @Test
  public void testMapIteratorTransformsElements()
  {
    Iterator<Integer> source = Arrays.asList(1, 2, 3).iterator();
    Iterator<String> mapped = FunctionalAdapters.mapIterator(source, i -> "v" + i);
    assertTrue(mapped.hasNext());
    assertEquals(mapped.next(), "v1");
    assertEquals(mapped.next(), "v2");
    assertEquals(mapped.next(), "v3");
    assertFalse(mapped.hasNext());
  }

  @Test
  public void testMapEntryWithSetGetters()
  {
    FunctionalAdapters.MapEntryWithSet<String, Integer> entry
            = new FunctionalAdapters.MapEntryWithSet<>("key", 1, (k, v) -> v);
    assertEquals(entry.getKey(), "key");
    assertEquals(entry.getValue(), Integer.valueOf(1));
  }

  @Test
  public void testMapEntryWithSetUsesCustomSetter()
  {
    Map<String, Integer> backing = new java.util.HashMap<>();
    backing.put("key", 1);
    FunctionalAdapters.MapEntryWithSet<String, Integer> entry
            = new FunctionalAdapters.MapEntryWithSet<>("key", 1, backing::put);
    Integer previous = entry.setValue(2);
    assertEquals(previous, Integer.valueOf(1));
    assertEquals(backing.get("key"), Integer.valueOf(2));
  }

}
