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

import org.jpype.bridge.Interpreter;
import static python.lang.PyBuiltIn.backend;

/**
 * Java front-end interface for the Python `enumerate` type.
 *
 * This interface provides functionality for working with Python's `enumerate`
 * objects in a Java environment, mimicking the behavior of Python's built-in
 * `enumerate` function.
 * <p>
 * The Python `enumerate` getType is a generator that yields pairs of an index
 * and the corresponding element from an iterable. This interface allows Java
 * developers to interact with Python `enumerate` objects seamlessly.
 */
public interface PyEnumerate extends PyIter<PyTuple>
{

  /**
   * Creates a new Python `enumerate` object from the specified Java
   * {@link Iterable}. The resulting `PyEnumerate` object will yield pairs of an
   * index (starting from 0) and the corresponding element from the iterable,
   * similar to Python's `enumerate` function.
   *
   * @param iterable the {@link Iterable} whose elements will be enumerated.
   * @return a new {@link PyEnumerate} instance representing the Python
   * `enumerate` object.
   */
  static PyEnumerate of(Iterable<?> iterable)
  {
    return backend().newEnumerate(iterable);
  }

}
