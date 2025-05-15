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

import java.util.NoSuchElementException;
import org.jpype.bridge.Interpreter;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 *
 * @author nelson85
 */
public class PyTupleIteratorNGTest
{

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    Interpreter.getInstance().start(new String[0]);
  }

  @Test
  public void testForwardIteration()
  {
    // Arrange
    PyTuple tuple = PyTuple.of("a", "b", "c");
    PyTupleIterator iterator = (PyTupleIterator) tuple.listIterator(0);

    // Act & Assert
    assertTrue(iterator.hasNext(), "Iterator should have a next element.");
    assertEquals(iterator.next().toString(), "a", "First element should be 'a'.");
    assertTrue(iterator.hasNext(), "Iterator should have a next element.");
    assertEquals(iterator.next().toString(), "b", "Second element should be 'b'.");
    assertTrue(iterator.hasNext(), "Iterator should have a next element.");
    assertEquals(iterator.next().toString(), "c", "Third element should be 'c'.");
    assertFalse(iterator.hasNext(), "Iterator should not have a next element.");
  }

  @Test
  public void testBackwardIteration()
  {
    // Arrange
    PyTuple tuple = PyTuple.of("a", "b", "c");
    PyTupleIterator iterator = new PyTupleIterator(tuple, tuple.size());

    // Act & Assert
    assertTrue(iterator.hasPrevious(), "Iterator should have a previous element.");
    assertEquals(iterator.previous().toString(), "c", "Last element should be 'c'.");
    assertTrue(iterator.hasPrevious(), "Iterator should have a previous element.");
    assertEquals(iterator.previous().toString(), "b", "Second-to-last element should be 'b'.");
    assertTrue(iterator.hasPrevious(), "Iterator should have a previous element.");
    assertEquals(iterator.previous().toString(), "a", "First element should be 'a'.");
    assertFalse(iterator.hasPrevious(), "Iterator should not have a previous element.");
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testNextThrowsExceptionWhenNoNextElement()
  {
    // Arrange
    PyTuple tuple = PyTuple.of("a", "b", "c");
    PyTupleIterator iterator = new PyTupleIterator(tuple, tuple.size());

    // Act
    iterator.next(); // Should throw NoSuchElementException
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testPreviousThrowsExceptionWhenNoPreviousElement()
  {
    // Arrange
    PyTuple tuple = PyTuple.of("a", "b", "c");
    PyTupleIterator iterator = (PyTupleIterator) tuple.listIterator(0);

    // Act
    iterator.previous(); // Should throw NoSuchElementException
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testAddOperationThrowsException()
  {
    // Arrange
    PyTuple tuple = PyTuple.of("a", "b", "c");
    PyTupleIterator iterator = (PyTupleIterator) tuple.listIterator(0);

    // Act
    iterator.add(null); // Should throw UnsupportedOperationException
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRemoveOperationThrowsException()
  {
    // Arrange
    PyTuple tuple = PyTuple.of("a", "b", "c");
    PyTupleIterator iterator = (PyTupleIterator) tuple.listIterator(0);

    // Act
    iterator.remove(); // Should throw UnsupportedOperationException
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testSetOperationThrowsException()
  {
    // Arrange
    PyTuple tuple = PyTuple.of("a", "b", "c");
    PyTupleIterator iterator = (PyTupleIterator) tuple.listIterator(0);

    // Act
    iterator.set(null); // Should throw UnsupportedOperationException
  }

  @Test(expectedExceptions = IndexOutOfBoundsException.class)
  public void testConstructorThrowsExceptionForInvalidIndex()
  {
    // Arrange
    PyTuple tuple = PyTuple.of("a", "b", "c");

    // Act
    new PyTupleIterator(tuple, -1); // Should throw IndexOutOfBoundsException
  }

  @Test
  public void testNextIndexAndPreviousIndex()
  {
    // Arrange
    PyTuple tuple = PyTuple.of("a", "b", "c");
    PyTupleIterator iterator = (PyTupleIterator) tuple.listIterator(1);

    // Act & Assert
    assertEquals(iterator.nextIndex(), 1, "Next index should be 1.");
    assertEquals(iterator.previousIndex(), 0, "Previous index should be 0.");
  }

}
