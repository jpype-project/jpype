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
 * A custom implementation of {@link ListIterator} for iterating over a
 * {@link PyList}.
 *
 * This iterator supports forward and backward traversal, as well as
 * modification of the list.
 */
class PyListIterator implements ListIterator<PyObject>
{

  private final PyList list; // The list being iterated
  private int index; // Current position in the list
  private boolean forward = false; // Flag indicating if the last operation was forward traversal
  private boolean reverse = false; // Flag indicating if the last operation was reverse traversal

  /**
   * Constructs a new {@code PyListIterator} for the specified list starting at
   * the given index.
   *
   * @param list The {@link PyList} to iterate over.
   * @param index The starting index for iteration.
   */
  PyListIterator(PyList list, int index)
  {
    this.list = list;
    this.index = index;
  }

  @Override
  public void add(PyObject e)
  {
    list.add(index, e); // Insert the element at the current index
    index++; // Increment index to reflect the added element
    forward = false;
    reverse = false;
  }

  @Override
  public boolean hasNext()
  {
    return index < list.size();
  }

  @Override
  public boolean hasPrevious()
  {
    return index > 0;
  }

  @Override
  public PyObject next()
  {
    if (!hasNext())
    {
      throw new NoSuchElementException();
    }
    PyObject out = list.get(index);
    index++;
    forward = true;
    reverse = false;
    return out;
  }

  @Override
  public int nextIndex()
  {
    return index;
  }

  @Override
  public PyObject previous()
  {
    if (!hasPrevious())
    {
      throw new NoSuchElementException();
    }
    index--;
    PyObject out = list.get(index);
    forward = false;
    reverse = true;
    return out;
  }

  @Override
  public int previousIndex()
  {
    return index - 1;
  }

  @Override
  public void remove()
  {
    if (!forward && !reverse)
    {
      throw new IllegalStateException("Cannot remove element without a valid traversal.");
    }
    if (forward)
    {
      list.remove(index - 1); // Remove the last element traversed in the forward direction
      index--; // Adjust index to reflect the removal
    } else if (reverse)
    {
      list.remove(index); // Remove the last element traversed in the reverse direction
    }
    forward = false;
    reverse = false;
  }

  @Override
  public void set(PyObject e)
  {
    if (!forward && !reverse)
    {
      throw new IllegalStateException("Cannot set element without a valid traversal.");
    }
    if (forward)
    {
      list.set(index - 1, e); // Replace the last element traversed in the forward direction
    } else if (reverse)
    {
      list.set(index, e); // Replace the last element traversed in the reverse direction
    }
  }
}
