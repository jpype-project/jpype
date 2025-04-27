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
import static python.lang.PyBuiltIn.*;

/**
 *
 * @author nelson85
 */
public class PyTypeNGTest
{

  @BeforeClass
  public static void setUpClass() throws Exception
  {
 Interpreter.getInstance().start(new String[0]);
  }
  
  @Test
  public void testGetName()
  {
    PyType type = PyDict.type();
    assertEquals(type.getName(), "dict");
  }

  @Test
  public void testGetBase()
  {
    PyType type = PyDict.type();

    assertEquals(type.getBase().getName(), "object");
  }

  @Test
  public void testGetBases()
  {
    PyType baseType1 = PyObject.type();
    PyType type = PyDict.type();
    assertEquals(type.getBases().size(), 1);
    assertTrue(type.getBases().contains(baseType1));
  }

  @Test
  public void testIsSubclassOf()
  {
    PyType type = PyDict.type();
    assertTrue(type.isSubclassOf(PyObject.type()));
    assertFalse(type.isSubclassOf(PyRange.type()));
  }

  @Test
  public void testIsInstance()
  {
    PyObject obj = PyString.from("test");
    PyType type = type(obj);
    assertTrue(type.isInstance(obj));
    assertFalse(type.isInstance(PyDict.type()));
  }

  @Test
  public void testGetMethod()
  {
    PyType type = PyDict.type();
    assertNotNull(type.getMethod("keys"));
  }

  @Test
  public void testIsAbstract()
  {
    Context context = new Context();
    context.importModule("collections");
    PyObject obj = context.eval("collections.abc.Mapping");
    PyType type = type(obj);
    assertTrue(type.isAbstract());
    PyType concreteType = PyDict.type();
    assertFalse(concreteType.isAbstract());
  }

  @Test
  public void testGetSubclasses()
  {
    PyType type = PyDict.type();
    assertEquals(((PyType) type.getSubclasses().get(0)).getName(), "<class 'collections.OrderedDict'>");
  }

}
