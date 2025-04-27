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

import java.util.Arrays;
import java.util.List;
import org.jpype.bridge.Interpreter;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 *
 * @author nelson85
 */
public class PyZipNGTest
{

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    Interpreter.getInstance().start(new String[0]);
  }

  @Test
  public void testStaticOfMethod()
  {
    // Create test iterables
    List<Integer> list1 = Arrays.asList(1, 2, 3);
    List<String> list2 = Arrays.asList("a", "b", "c");

    // Create PyZip using the static `of` method
    PyZip pyZip = PyBuiltIn.zip(list1, list2);

    // Convert to list and verify zipped items
    PyList result = pyZip.toList();

    assertEquals(result.size(), 3);
    assertEquals(result.get(0).toString(), "[1, a]");
    assertEquals(result.get(1).toString(), "[2, b]");
    assertEquals(result.get(2).toString(), "[3, c]");
  }

  @Test
  public void testStaticOfMethodUnevenLengths()
  {
    // Create test iterables with uneven lengths
    List<Integer> list1 = Arrays.asList(1, 2);
    List<String> list2 = Arrays.asList("a", "b", "c");

    // Create PyZip using the static `of` method
    PyZip pyZip = PyBuiltIn.zip(list1, list2);

    // Convert to list and verify zipped items
    PyList result = pyZip.toList();

    assertEquals(result.size(), 3);
    assertEquals(result.get(0).toString(), "[1, a]");
    assertEquals(result.get(1).toString(), "[2, b]");
    assertEquals(result.get(2).toString(), "[null, c]");
  }

  @Test
  public void testStaticTypeMethod()
  {
    // Test the static `type` method
    PyType pyType = PyZip.type();
    assertEquals(pyType.toString(), "zip");
  }

  @Test
  public void testToListMethod()
  {
    // Create test iterables
    List<Integer> list1 = Arrays.asList(1, 2, 3);
    List<String> list2 = Arrays.asList("a", "b", "c");

    // Create PyZip instance
    PyZip pyZip = PyBuiltIn.zip(list1, list2);

    // Convert to list and verify zipped items
    PyList result = pyZip.toList();

    assertEquals(result.size(), 3);
    assertEquals(result.get(0).toString(), "[1, a]");
    assertEquals(result.get(1).toString(), "[2, b]");
    assertEquals(result.get(2).toString(), "[3, c]");
  }
}
