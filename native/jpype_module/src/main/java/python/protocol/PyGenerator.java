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
package python.protocol;

import java.util.Iterator;
import python.lang.PyObject;

/**
 * Represents a Python generator in the Java environment.
 *
 * This interface provides a Java front end for the abstract concept of a Python
 * generator, which adheres to the Python {@code collections.abc.Generator} contract.
 * Generators are used to produce values lazily and support iteration, making
 * them suitable for handling sequences of data efficiently.
 *
 * <p>
 * Key features:</p>
 * <ul>
 * <li>Provides an API for interacting with Python generators.</li>
 * <li>Supports Python's iteration protocol via {@code PyIter}.</li>
 * <li>Integrates seamlessly with Java's {@link Iterator} interface.</li>
 * </ul>
 *
 * <p>
 * This interface assumes that implementing classes will provide the necessary
 * functionality to bridge Python generators with Java's iteration
 * mechanisms.</p>
 */
public interface PyGenerator<T extends PyObject> extends PyIter<T>
{
  
  /**
   * Returns a Python iterator for this generator.
   *
   * <p>
   * This method provides access to the underlying Python iterator associated
   * with the generator, allowing iteration over its elements using Python
   * semantics.</p>
   *
   * @return a {@link PyIter} representing the Python iterator for this
   * generator
   */
  PyIter<T> iter();

  /**
   * Returns a Java {@link Iterator} for this generator.
   *
   * <p>
   * This method bridges the Python iteration protocol with Java's
   * {@link Iterator} interface, enabling iteration over the generator's
   * elements using Java semantics.</p>
   *
   * @return a {@link Iterator} for iterating over the elements of the generator
   */
  @Override
  default Iterator<T> iterator()
  {
    return  iter().iterator();
  }
}
