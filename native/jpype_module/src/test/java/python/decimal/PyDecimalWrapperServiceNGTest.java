// --- file: python/decimal/PyDecimalWrapperServiceNGTest.java ---
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
package python.decimal;

import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

/**
 * Exercises {@code PyDecimalWrapperService}'s own metadata directly: the
 * happy path (SPI provider actually registering {@code decimal.Decimal})
 * is already covered end-to-end by {@code PyDecimalNGTest}.
 */
public class PyDecimalWrapperServiceNGTest
{

  @Test
  public void testGetModuleNames()
  {
    PyDecimalWrapperService service = new PyDecimalWrapperService();
    assertEquals(service.getModuleNames(), new String[]
    {
      "decimal"
    });
  }

  @Test
  public void testGetVersion()
  {
    PyDecimalWrapperService service = new PyDecimalWrapperService();
    assertEquals(service.getVersion(), "1");
  }

  @Test
  public void testGetResources()
  {
    PyDecimalWrapperService service = new PyDecimalWrapperService();
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
