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

import python.lang.PyObject;

/**
 * Represents objects that can be used to index arrays or sequences in
 * Python-like operations.
 *
 * <p>
 * This interface defines the contract for objects that can serve as indices in
 * array or sequence operations, similar to Python's indexing behavior. It
 * provides a foundation for implementing Python-style indexing in Java,
 * including support for extended indexing constructs such as slices.
 *
 * <p>
 * Examples of objects that can implement this interface include:
 * <ul>
 * <li>Integer-based indices for accessing single elements</li>
 * <li>Slice objects for accessing ranges of elements</li>
 * <li>Custom index types for advanced indexing behavior</li>
 * </ul>
 *
 * <p>
 * Implementations of this interface should ensure compatibility with Python's
 * indexing semantics, including support for negative indices, slicing, and
 * other advanced features where applicable.
 *
 * <h2>Usage Example</h2>
 * <pre>
 * PyIndex index = BuiltIn.slice(0, 10, 2); // Create a slice object
 * PyObject result = array.get(index);   // Use the index to retrieve elements
 * </pre>
 *
 * <p>
 * Note: This interface does not define any methods directly, as it serves as a
 * marker interface for objects that can be used as indices. Specific behaviors
 * and operations should be implemented in concrete classes that extend this
 * interface.
 *
 * @see PySlice
 * @see PyProtocol
 */
public interface PyIndex extends PyObject
{
}
