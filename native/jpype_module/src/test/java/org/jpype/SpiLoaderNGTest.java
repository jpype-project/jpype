// --- file: org/jpype/SpiLoaderNGTest.java ---
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
package org.jpype;

import java.util.Collections;
import java.util.List;
import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;
import static org.testng.Assert.fail;

/**
 * Exercises {@code SpiLoader}'s own error handling and directory-scan edge
 * cases directly, without going through a real {@code WrapperService}
 * ServiceLoader registration (the happy path is already exercised
 * end-to-end by {@code python.io.PyLazySpiNGTest}). The jar-protocol branch
 * of {@link SpiLoader#listPyspiResources} is not covered here: it only
 * triggers when scanning resources packaged inside an actual jar on the
 * classpath, which {@code mvn test}'s exploded-directory classpath never
 * produces (see plan/Coverage.md).
 */
public class SpiLoaderNGTest
{

  @Test
  public void testMissingResourceThrowsIllegalStateException() throws Exception
  {
    // readResource() is private and only reachable through SpiLoader.load(),
    // which requires a real ServiceLoader-discovered provider on the
    // classpath - reflection is the only way to reach this exact not-found
    // branch directly and deterministically.
    java.lang.reflect.Method readResource = SpiLoader.class.getDeclaredMethod(
            "readResource", Class.class, String.class);
    readResource.setAccessible(true);
    try
    {
      readResource.invoke(null, SpiLoaderNGTest.class, "/no/such/resource.pyspi");
      fail("Expected IllegalStateException for a missing SPI resource");
    } catch (java.lang.reflect.InvocationTargetException wrapped)
    {
      Throwable expected = wrapped.getCause();
      assertTrue(expected instanceof IllegalStateException, "Unexpected exception: " + expected);
      assertTrue(expected.getMessage().contains("SPI resource not found"),
              "Unexpected message: " + expected.getMessage());
    }
  }

  @Test
  public void testListPyspiResourcesReturnsEmptyForMissingDirectory()
  {
    List<String> found = SpiLoader.listPyspiResources(SpiLoaderNGTest.class, "/no/such/directory");
    assertEquals(found, Collections.emptyList());
  }

  @Test
  public void testListPyspiResourcesFileProtocolSorted()
  {
    // python.io's own worked example (plan/StreamRedirect.md /
    // doc/spi.rst) ships real .pyspi resources under this directory on the
    // exploded test classpath.
    List<String> found = SpiLoader.listPyspiResources(
            python.io.PyIOWrapperService.class, "/python/io/spi");
    assertTrue(!found.isEmpty(), "Expected at least one .pyspi resource");
    List<String> sorted = new java.util.ArrayList<>(found);
    Collections.sort(sorted);
    assertEquals(found, sorted);
    for (String name : found)
      assertTrue(name.endsWith(".pyspi"), "Unexpected entry: " + name);
  }
}
