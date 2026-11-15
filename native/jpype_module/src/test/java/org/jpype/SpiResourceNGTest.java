// --- file: org/jpype/SpiResourceNGTest.java ---
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

import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertTrue;
import static org.testng.Assert.fail;

/**
 * Exercises {@code SpiResource.parse}'s header-parsing validation directly
 * ({@code .pyspi} format documented on
 * {@code WrapperService#getResources()}): every {@code IllegalArgumentException}
 * branch, comment/blank-line skipping, and the class/backend/lazy field
 * combinations. Pure string parsing, no I/O.
 */
public class SpiResourceNGTest
{

  @Test
  public void testMissingSeparatorThrows()
  {
    try
    {
      SpiResource.parse("kind: class\ninterface: foo.Bar\n");
      fail("Expected IllegalArgumentException");
    } catch (IllegalArgumentException expected)
    {
      assertTrue(expected.getMessage().contains("---"));
    }
  }

  @Test
  public void testMalformedHeaderLineThrows()
  {
    try
    {
      SpiResource.parse("not a valid line\n---\nBODY");
      fail("Expected IllegalArgumentException");
    } catch (IllegalArgumentException expected)
    {
      assertTrue(expected.getMessage().contains("Malformed"));
    }
  }

  @Test
  public void testMissingInterfaceThrows()
  {
    try
    {
      SpiResource.parse("kind: class\nmodule: m\nclass: C\n---\nBODY");
      fail("Expected IllegalArgumentException");
    } catch (IllegalArgumentException expected)
    {
      assertTrue(expected.getMessage().contains("interface:"));
    }
  }

  @Test
  public void testLazyBackendThrows()
  {
    try
    {
      SpiResource.parse("kind: backend\ninterface: foo.Bar\nlazy: true\n---\nBODY");
      fail("Expected IllegalArgumentException");
    } catch (IllegalArgumentException expected)
    {
      assertTrue(expected.getMessage().contains("eager"));
    }
  }

  @Test
  public void testClassKindMissingModuleOrClassThrows()
  {
    try
    {
      SpiResource.parse("kind: class\ninterface: foo.Bar\n---\nBODY");
      fail("Expected IllegalArgumentException");
    } catch (IllegalArgumentException expected)
    {
      assertTrue(expected.getMessage().contains("module:"));
    }
  }

  @Test
  public void testCommentsAndBlankLinesSkipped()
  {
    SpiResource resource = SpiResource.parse(
            "# a comment\n\nkind: backend\ninterface: foo.Bar\n---\nMETHODS = {}\n");
    assertEquals(resource.kind, "backend");
    assertEquals(resource.javaInterface, "foo.Bar");
    assertFalse(resource.lazy);
    assertEquals(resource.body, "METHODS = {}\n");
  }

  @Test
  public void testValidClassResourceEagerAndLazy()
  {
    SpiResource eager = SpiResource.parse(
            "kind: class\nmodule: _io\nclass: BytesIO\ninterface: python.io.PyBytesIO\n---\nMETHODS = {}");
    assertEquals(eager.kind, "class");
    assertEquals(eager.module, "_io");
    assertEquals(eager.className, "BytesIO");
    assertEquals(eager.javaInterface, "python.io.PyBytesIO");
    assertFalse(eager.lazy);

    SpiResource lazy = SpiResource.parse(
            "kind: class\nmodule: _io\nclass: StringIO\ninterface: python.io.PyStringIO\nlazy: true\n---\nMETHODS = {}");
    assertTrue(lazy.lazy);
  }

  @Test
  public void testKindDefaultsToClass()
  {
    SpiResource resource = SpiResource.parse(
            "module: m\nclass: C\ninterface: foo.Bar\n---\nBODY");
    assertEquals(resource.kind, "class");
  }
}
