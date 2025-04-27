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
import java.util.stream.Collectors;
import java.util.stream.Stream;
import org.jpype.bridge.Interpreter;
import org.testng.annotations.Test;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;

public class PyTupleNGTest
{

  

  @BeforeClass
  public static void setUpClass() throws Exception
  {
 Interpreter.getInstance().start(new String[0]);
  }
  
  @Test
  public void testEmpty()
  {
    PyTuple emptyTuple = PyTuple.of();
    assertNotNull(emptyTuple, "Empty tuple should not be null.");
    assertTrue(emptyTuple.isEmpty(), "Empty tuple should have no elements.");
    assertEquals(emptyTuple.size(), 0, "Empty tuple size should be 0.");
  }

  @Test
  public void testOfVarArgs()
  {
    PyTuple tuple = PyTuple.of("a", "b", "c");
    assertNotNull(tuple, "Tuple should not be null.");
    assertEquals(tuple.size(), 3, "Tuple size should be 3.");
    assertEquals(tuple.get(0), "a", "First element should be 'a'.");
    assertEquals(tuple.get(1), "b", "Second element should be 'b'.");
    assertEquals(tuple.get(2), "c", "Third element should be 'c'.");
  }

  @Test
  public void testOfIterator()
  {
    List<String> items = Arrays.asList("x", "y");
    PyTuple tuple = PyTuple.fromItems(items);
    assertNotNull(tuple, "Tuple should not be null.");
    assertEquals(tuple.size(), 2, "Tuple size should be 2.");
    assertEquals(tuple.get(0).toString(), "x", "First element should be 'x'.");
    assertEquals(tuple.get(1).toString(), "y", "Second element should be 'y'.");
  }

  @Test
  public void testType()
  {
    PyType tupleType = PyTuple.type();
    assertNotNull(tupleType, "Tuple type should not be null.");
    assertEquals(tupleType.toString(), "tuple", "Tuple type should be 'tuple'.");
  }

  @Test
  @SuppressWarnings("element-type-mismatch")
  public void testContains()
  {
    PyTuple tuple = PyTuple.of("a", "b");
    assertTrue(tuple.contains("a"), "Tuple should contain 'a'.");
    assertFalse(tuple.contains("z"), "Tuple should not contain 'z'.");
  }

  @Test
  public void testIsEmpty()
  {
    PyTuple emptyTuple = PyTuple.of();
    assertTrue(emptyTuple.isEmpty(), "Empty tuple should be empty.");

    PyTuple nonEmptyTuple = PyTuple.of("a");
    assertFalse(nonEmptyTuple.isEmpty(), "Non-empty tuple should not be empty.");
  }

  @Test
  public void testSize()
  {
    PyTuple tuple = PyTuple.of("a", "b");
    assertEquals(tuple.size(), 2, "Tuple size should be 2.");
  }

  @Test
  public void testSubList()
  {
    PyTuple tuple = PyTuple.of("a", "b", "c");
    PyTuple subTuple = tuple.subList(0, 2);
    assertEquals(subTuple.size(), 2, "Sublist size should be 2.");
    assertEquals(subTuple.get(0).toString(), "a", "First element of sublist should be 'a'.");
    assertEquals(subTuple.get(1).toString(), "b", "Second element of sublist should be 'b'.");
  }

  @Test
  public void testStream()
  {
    PyTuple tuple = PyTuple.of("a", "b");
    Stream<PyObject> stream = tuple.stream();
    List<String> elements = stream.map(PyObject::toString).collect(Collectors.toList());
    assertEquals(elements.size(), 2, "Stream should contain 2 elements.");
    assertEquals(elements.get(0), "a", "First stream element should be 'a'.");
    assertEquals(elements.get(1), "b", "Second stream element should be 'b'.");
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testAddThrowsException()
  {
       PyTuple tuple = PyTuple.of("a", "b");
    tuple.add(PyString.from("a"));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testAddThrowsException2()
  {
    PyTuple tuple = PyTuple.of("a", "b");
    tuple.add(1, PyString.from("a"));
  }
}
