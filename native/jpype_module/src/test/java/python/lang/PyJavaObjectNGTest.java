// --- file: python/lang/PyJavaObjectNGTest.java ---
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

public class PyJavaObjectNGTest extends PyTestHarness
{

  @Test
  public void testGet()
  {
    Object obj = new Object();
    PyJavaObject wrapped = new PyJavaObject(obj);
    assertSame(wrapped.get(), obj);
  }

  @Test
  public void testToString()
  {
    PyJavaObject wrapped = new PyJavaObject("hello");
    assertEquals(wrapped.toString(), "hello");
  }

  @Test
  public void testToStringOfNull()
  {
    PyJavaObject wrapped = new PyJavaObject(null);
    assertEquals(wrapped.toString(), "null");
  }

  @Test
  public void testHashCode()
  {
    PyJavaObject wrapped = new PyJavaObject("hello");
    assertEquals(wrapped.hashCode(), "hello".hashCode());
  }

  @Test
  public void testEqualsSameWrappedObject()
  {
    PyJavaObject a = new PyJavaObject("hello");
    PyJavaObject b = new PyJavaObject("hello");
    assertEquals(a, b);
  }

  @Test
  public void testEqualsDifferentWrappedObject()
  {
    PyJavaObject a = new PyJavaObject("hello");
    PyJavaObject b = new PyJavaObject("world");
    assertNotEquals(a, b);
  }

  @Test
  public void testEqualsSelf()
  {
    PyJavaObject a = new PyJavaObject("hello");
    assertEquals(a, a);
  }

  @Test
  public void testNotEqualsNull()
  {
    PyJavaObject a = new PyJavaObject("hello");
    assertNotEquals(a, null);
  }

  @Test
  public void testNotEqualsDifferentType()
  {
    PyJavaObject a = new PyJavaObject("hello");
    assertNotEquals(a, "hello");
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testBuiltinThrows()
  {
    PyJavaObject wrapped = new PyJavaObject("hello");
    wrapped.builtin();
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testGetAttributesThrows()
  {
    PyJavaObject wrapped = new PyJavaObject("hello");
    wrapped.getAttributes();
  }

}
