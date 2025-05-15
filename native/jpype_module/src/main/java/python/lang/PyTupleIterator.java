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

import java.util.ListIterator;
import java.util.NoSuchElementException;

/**
 * An iterator for traversing a PyTuple object in both forward and reverse
 * directions.
 *
 * This class implements the {@link ListIterator} interface to provide
 * bidirectional iteration over a PyTuple, which is assumed to behave like a
 * Python tuple.
 *
 * Modifications to the PyTuple (e.g., adding or removing elements) are not
 * supported by this iterator.
 */
class PyTupleIterator implements ListIterator<PyObject>
{

  /**
   * The PyTuple being iterated over.
   */
  private final PyTuple tuple;

  /**
   * The current index of the iterator.
   */
  private int index;

  /**
   * Constructs a PyTupleIterator starting at the specified index.
   *
   * @param tuple The PyTuple to iterate over.
   * @param index The initial index of the iterator.
   */
  PyTupleIterator(PyTuple tuple, int index)
  {
    if (index < 0 || index > tuple.size())
    {
      throw new IndexOutOfBoundsException("Index out of bounds for PyTuple.");
    }
    this.tuple = tuple;
    this.index = index;
  }

  /**
   * Unsupported operation: Adding elements is not allowed for PyTupleIterator.
   *
   * @param e The element to add (ignored).
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public void add(PyObject e)
  {
    throw new UnsupportedOperationException("PyTupleIterator does not support add operation.");
  }

  /**
   * Checks if there are more elements when iterating forward.
   *
   * @return True if there is a next element, false otherwise.
   */
  @Override
  public boolean hasNext()
  {
    return index < tuple.size();
  }

  /**
   * Checks if there are more elements when iterating backward.
   *
   * @return True if there is a previous element, false otherwise.
   */
  @Override
  public boolean hasPrevious()
  {
    return index > 0;
  }

  /**
   * Returns the next element in the iteration and advances the cursor forward.
   *
   * @return The next PyObject in the tuple.
   * @throws NoSuchElementException If there is no next element.
   */
  @Override
  public PyObject next()
  {
    if (!hasNext())
    {
      throw new NoSuchElementException("No next element.");
    }
    PyObject out = tuple.get(index);
    index++;
    return out;
  }

  /**
   * Returns the index of the element that would be returned by a subsequent
   * call to {@link #next()}.
   *
   * @return The index of the next element.
   */
  @Override
  public int nextIndex()
  {
    return index;
  }

  /**
   * Returns the previous element in the iteration and moves the cursor
   * backward.
   *
   * @return The previous PyObject in the tuple.
   * @throws NoSuchElementException If there is no previous element.
   */
  @Override
  public PyObject previous()
  {
    if (!hasPrevious())
    {
      throw new NoSuchElementException("No previous element in PyTuple.");
    }
    index--;
    PyObject out = tuple.get(index);
    return out;
  }

  /**
   * Returns the index of the element that would be returned by a subsequent
   * call to {@link #previous()}.
   *
   * @return The index of the previous element.
   */
  @Override
  public int previousIndex()
  {
    return index - 1;
  }

  /**
   * Unsupported operation: Removing elements is not allowed for
   * PyTupleIterator.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public void remove()
  {
    throw new UnsupportedOperationException("PyTupleIterator does not support remove operation.");
  }

  /**
   * Unsupported operation: Setting elements is not allowed for PyTupleIterator.
   *
   * @param e The element to set (ignored).
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public void set(PyObject e)
  {
    throw new UnsupportedOperationException("PyTupleIterator does not support set operation.");
  }
}
