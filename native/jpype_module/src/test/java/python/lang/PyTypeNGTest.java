// --- file: python/lang/PyTypeNGTest.java ---
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

import org.jpype.Script;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import static org.testng.Assert.*;

/**
 *
 * @author nelson85
 */
public class PyTypeNGTest extends PyTestHarness
{

  static PyType objectType;
  static PyType dictType;
  static PyType rangeType;

  @BeforeClass
  public static void setUpTypes()
  {
    // context.type(x) returns type(x), the metaclass of x - not x itself.
    // object/dict/range are already type objects, so eval them directly.
    objectType = (PyType) context.eval("object");
    dictType = (PyType) context.eval("dict");
    rangeType = (PyType) context.eval("range");
  }


  @Test
  public void testGetName()
  {
    PyType type = dictType;
    assertEquals(type.getName(), "dict");
  }

  @Test
  public void testGetBase()
  {
    PyType type = dictType;

    assertEquals(type.getBase().getName(), "object");
  }

  @Test
  public void testGetBases()
  {
    PyType baseType1 = objectType;
    PyType type = dictType;
    assertEquals(type.getBases().size(), 1);
    assertTrue(type.getBases().contains(baseType1));
  }

  @Test
  public void testIsSubclassOf()
  {
    PyType type = dictType;
    assertTrue(type.isSubclassOf(objectType));
    assertFalse(type.isSubclassOf(rangeType));
  }

  @Test
  public void testIsInstance()
  {
    PyObject obj = context.str("test");
    PyType type = context.type(obj);
    assertTrue(type.isInstance(obj));
    assertFalse(type.isInstance(dictType));
  }

  @Test
  public void testGetMethod()
  {
    PyType type = dictType;
    assertNotNull(type.getMethod("keys"));
  }

  @Test
  public void testIsAbstract()
  {
    context.importModule("collections");
    PyType type = (PyType) context.eval("collections.abc.Mapping");
    assertTrue(type.isAbstract());
    PyType concreteType = dictType;
    assertFalse(concreteType.isAbstract());
  }

  @Test
  public void testGetSubclasses()
  {
    PyType type = dictType;
    assertEquals(((PyType) type.getSubclasses().get(0)).getName(), "OrderedDict");
  }

}
