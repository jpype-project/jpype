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

import static python.lang.PyBuiltIn.backend;

/**
 * Java front end for concrete Python zip.
 */
public interface PyZip extends PyIter<PyTuple>
{

  /**
   * Creates a new PyZip object by zipping the provided iterables.
   *
   * @param items is the iterables to zip together.
   * @return a PyZip object representing the zipped iterables.
   */
  static PyZip of(Iterable<?>... items)
  {
    return backend().newZip(items);
  }

  /**
   * Retrieves all remaining items from the zipped iterables as a list.
   *
   * This method collects all remaining tuples from the iterator and returns
   * them in a PyList, similar to Python's `list(zip(...))`.
   *
   * @return a PyList containing all remaining tuples from the zipped iterables.
   */
  PyList toList();

}
