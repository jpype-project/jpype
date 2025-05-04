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

import python.lang.PyObject;

/**
 * Represents a protocol for Python objects that act as collections.
 * <p>
 * This interface serves as a Java equivalent to the Python class hierarchy for
 * collections. It does not define any methods but provides a structure for
 * implementing Python-like collection behaviors in Java. Classes implementing
 * this interface are expected to conform to the behaviors defined by the
 * inherited interfaces.
 * </p>
 *
 * <p>
 * The {@code PyCollection} interface extends the following interfaces:
 * <ul>
 * <li>{@link PySized} - Represents objects that have a size (length).</li>
 * <li>{@link PyIterable} - Represents objects that can be iterated over.</li>
 * <li>{@link PyContainer} - Represents objects that can check membership
 * (containment).</li>
 * </ul>
 * </p>
 *
 * @param <T> The type of elements contained within the collection, which must
 * extend {@link PyObject}.
 *
 * @see PySized
 * @see PyIterable
 * @see PyContainer
 */
public interface PyCollection<T extends PyObject> extends PySized, PyIterable<T>, PyContainer<T>
{
}
