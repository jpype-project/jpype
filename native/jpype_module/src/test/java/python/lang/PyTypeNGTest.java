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

import org.jpype.bridge.Context;
import org.jpype.bridge.Interpreter;
import org.testng.annotations.Test;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import python.lang.PyBuiltIn;

/**
 *
 * @author nelson85
 */
public class PyTypeNGTest
{
 static PyType objectType;
 static PyType dictType;
 static PyType rangeType;
 @BeforeClass
  public static void setUpClass() throws Exception
  {
    Interpreter interpreter = Interpreter.getInstance();
    interpreter.start(new String[0]);
    Context context = new Context();
    objectType = (PyType) context.eval("type(object)");
    dictType = (PyType) context.eval("type(dict)");
    rangeType = (PyType) context.eval("type(range)");
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
    PyObject obj = PyString.from("test");
    PyType type = PyBuiltIn.type(obj);
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
    Context context = new Context();
    context.importModule("collections");
    PyObject obj = context.eval("collections.abc.Mapping");
    PyType type = PyBuiltIn.type(obj);
    assertTrue(type.isAbstract());
    PyType concreteType = dictType;
    assertFalse(concreteType.isAbstract());
  }

  @Test
  public void testGetSubclasses()
  {
    PyType type = dictType;
    assertEquals(((PyType) type.getSubclasses().get(0)).getName(), "<class 'collections.OrderedDict'>");
  }

}
