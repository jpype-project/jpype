// --- file: python/lang/PyMemoryViewNGTest.java ---
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
package python.lang;

import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyMemoryViewNGTest extends PyTestHarness
{

  @Test
  public void testCreate()
  {
    PyByteArray bytes = context.bytearray(5);
    PyMemoryView view = context.memoryview(bytes);
    assertNotNull(view);
    assertEquals(view.size(), 5);
  }

  @Test
  public void testIsReadOnly()
  {
    PyByteArray bytes = context.bytearray(3);
    PyMemoryView view = context.memoryview(bytes);
    assertFalse(view.isReadOnly());
  }

  @Test
  public void testGetFormat()
  {
    PyByteArray bytes = context.bytearray(3);
    PyMemoryView view = context.memoryview(bytes);
    assertEquals(view.getFormat(), "B");
  }

  @Test
  public void testGetShape()
  {
    PyByteArray bytes = context.bytearray(4);
    PyMemoryView view = context.memoryview(bytes);
    PyTuple shape = view.getShape();
    assertEquals(shape.size(), 1);
    assertEquals(shape.get(0).toString(), "4");
  }

  @Test
  public void testGetSlice()
  {
    PyByteArray bytes = context.bytearray(10);
    PyMemoryView view = context.memoryview(bytes);
    PyMemoryView slice = view.getSlice(2, 5);
    assertEquals(slice.size(), 3);
  }

  @Test
  public void testGetBuffer()
  {
    PyByteArray bytes = context.bytearray(3);
    PyMemoryView view = context.memoryview(bytes);
    assertNotNull(view.getBuffer());
  }

  @Test
  public void testRelease()
  {
    PyByteArray bytes = context.bytearray(3);
    PyMemoryView view = context.memoryview(bytes);
    view.release();
  }

}
