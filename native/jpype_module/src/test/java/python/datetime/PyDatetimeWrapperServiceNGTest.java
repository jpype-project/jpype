// --- file: python/datetime/PyDatetimeWrapperServiceNGTest.java ---
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
package python.datetime;

import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

/**
 * Exercises {@code PyDatetimeWrapperService}'s own metadata directly: the
 * happy path (SPI provider actually registering {@code date}/
 * {@code datetime}/{@code timedelta}) is already covered end-to-end by the
 * other {@code python.datetime} NGTest classes.
 */
public class PyDatetimeWrapperServiceNGTest
{

  @Test
  public void testGetModuleNames()
  {
    PyDatetimeWrapperService service = new PyDatetimeWrapperService();
    assertEquals(service.getModuleNames(), new String[]
    {
      "datetime"
    });
  }

  @Test
  public void testGetVersion()
  {
    PyDatetimeWrapperService service = new PyDatetimeWrapperService();
    assertEquals(service.getVersion(), "1");
  }

  @Test
  public void testGetResources()
  {
    PyDatetimeWrapperService service = new PyDatetimeWrapperService();
    Iterable<String> resources = service.getResources();
    boolean any = false;
    for (String name : resources)
    {
      any = true;
      assertTrue(name.endsWith(".pyspi"), "Unexpected entry: " + name);
    }
    assertTrue(any, "Expected at least one .pyspi resource");
  }
}
