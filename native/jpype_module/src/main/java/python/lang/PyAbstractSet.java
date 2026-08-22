// --- file: python/lang/PyAbstractSet.java ---
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

import java.util.Iterator;
import java.util.Set;
import org.jpype.annotation.Bypass;

/**
 * Represents a protocol for Python classes that act as sets.
 * <p>
 * This interface bridges the functionality of Python sets with Java's
 * {@link Set} interface, providing seamless interoperability between Python and
 * Java collections. It extends {@link PyCollection}, which provides
 * foundational collection behaviors, and Java's {@link Set}.
 * <p>
 * Note: this protocol covers membership testing and iteration only.
 * Python's operator-based set algebra (union, intersection, difference,
 * etc., normally spelled {@code |}, {@code &}, {@code -} in Python) is not
 * exposed here; use {@link PyBuiltIn#eval eval} to invoke those operators
 * directly if needed.
 * <p>
 * Due to name conflicts between protocols and concrete types, this interface
 * has been renamed {@code PyAbstractSet}.
 *
 * @param <T> the type of elements contained in the set, which must extend
 * {@link PyObject}
 */
public interface PyAbstractSet<T extends PyObject> extends PyCollection<T>, Set<T>
{

  /**
   * Checks whether the specified object is contained in this set.
   * <p>
   * This method overrides the default {@link Set#contains(Object)}
   * implementation to use the Python backend for determining membership.
   *
   * @param obj the object to check for membership in the set
   * @return {@code true} if the object is contained in the set; {@code false}
   * otherwise
   */
  @Bypass
  @Override
  default boolean contains(Object obj)
  {
    return builtin().backend.contains(this, obj);
  }

  /**
   * Checks whether this set is empty.
   * <p>
   * This method overrides the default {@link Set#isEmpty()} implementation and
   * checks the size of the set to determine if it is empty.
   *
   * @return {@code true} if the set contains no elements; {@code false}
   * otherwise
   */
    @Bypass
  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  /**
   * Provides a Java {@link Iterator} implementation for this set.
   * <p>
   * This method overrides the default {@link Set#iterator()} implementation and
   * uses a {@link PyIterator} to adapt Python's iteration protocol to Java's
   * {@link Iterator}.
   *
   * @return a Java iterator for this set
   */
    @Bypass
  @Override
  default Iterator<T> iterator()
  {
    return new PyIterator<>(this.iter());
  }

  /**
   * Returns the number of elements in this set.
   * <p>
   * This method overrides the default {@link Set#size()} implementation and
   * uses the Python {@code len()} built-in function to determine the size of
   * the set.
   *
   * @return the number of elements in the set
   */
    @Bypass
  @Override
  default int size()
  {
    return builtin().len(this);
  }

}
