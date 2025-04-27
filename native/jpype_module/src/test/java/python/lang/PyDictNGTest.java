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

import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import org.jpype.bridge.Interpreter;
import org.testng.annotations.Test;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;

/**
 *
 * @author nelson85
 */
public class PyDictNGTest
{

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    Interpreter.getInstance().start(new String[0]);
  }

  @Test
  public void testPutAndGet()
  {
    PyDict dict = PyDict.create();
    PyObject value = PyString.from("value1");
    dict.put("key1", value);
    assertEquals(dict.get("key1"), value);
  }

  @Test
  public void testGetOrDefault()
  {
    PyDict dict = PyDict.create();
    PyObject defaultValue = PyString.from("default");

    assertEquals(dict.getOrDefault("missingKey", defaultValue), defaultValue);
  }

  @Test
  public void testPop()
  {
    PyDict dict = PyDict.create();
    PyObject value = PyString.from("value1");
    PyObject defaultValue = PyString.from("default");
    dict.put("key1", value);
    assertEquals(dict.pop("key1", defaultValue), value);
    assertEquals(dict.pop("missingKey", defaultValue), defaultValue);
  }

  @Test
  public void testPopItem()
  {
    PyDict dict = PyDict.create();
    PyObject value = PyString.from("value1");
    dict.put("key1", value);
    Map.Entry<Object, PyObject> entry = dict.popItem();
    assertEquals(entry.getKey(), "key1");
    assertEquals(entry.getValue(), value);
    assertTrue(dict.isEmpty());
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testPopItemEmptyDict()
  {
    PyDict dict = PyDict.create();
    dict.popItem(); // Should throw NoSuchElementException
  }

  @Test
  public void testSize()
  {
    PyDict dict = PyDict.create();
    assertEquals(dict.size(), 0);
    dict.put("key1", PyString.from("value1"));
    assertEquals(dict.size(), 1);
  }

  @Test
  public void testIsEmpty()
  {
    PyDict dict = PyDict.create();
    assertTrue(dict.isEmpty());
    dict.put("key1", PyString.from("value1"));
    assertFalse(dict.isEmpty());
  }

  @Test
  public void testClear()
  {
    PyDict dict = PyDict.create();
    dict.put("key1", PyString.from("value1"));
    dict.put("key2", PyString.from("value2"));
    dict.clear();
    assertTrue(dict.isEmpty());
  }

  @Test
  public void testUpdateWithMap()
  {
    PyDict dict = PyDict.create();
    Map<Object, PyObject> updateMap = new HashMap<>();
    updateMap.put("key1", PyString.from("value1"));
    updateMap.put("key2", PyString.from("value2"));
    dict.update(updateMap);
    assertEquals(dict.size(), 2);
    assertEquals(dict.get("key1").toString(), "value1");
    assertEquals(dict.get("key2").toString(), "value2");
  }

  @Test
  public void testUpdateWithIterable()
  {
    PyDict dict = PyDict.create();
    List<Map.Entry<Object, PyObject>> updateList = new ArrayList<>();
    updateList.add(new AbstractMap.SimpleEntry<>("key1", PyString.from("value1")));
    updateList.add(new AbstractMap.SimpleEntry<>("key2", PyString.from("value2")));
    dict.update(updateList);
    assertEquals(dict.size(), 2);
    assertEquals(dict.get("key1").toString(), "value1");
    assertEquals(dict.get("key2").toString(), "value2");
  }

}
