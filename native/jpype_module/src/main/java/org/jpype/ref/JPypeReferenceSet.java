/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
  
  See NOTICE file for details.
**************************************************************************** */
package org.jpype.ref;

import java.util.ArrayList;

/**
 * A set that manages references to Python objects used by JPype.
 *
 * <p>
 * This class is responsible for storing and managing {@link JPypeReference}
 * objects efficiently. It uses an internal pooling mechanism to ensure that
 * adding and removing references are performed in constant time (O(1)).</p>
 *
 * <p>
 * The {@code JPypeReferenceSet} is used internally by JPype to track Python
 * objects (`PyObject*`) that are referenced by Java objects. It ensures proper
 * cleanup of Python resources when references are removed or during JVM
 * shutdown.</p>
 *
 * <p>
 * Note: This class is intended for internal use and should not be used directly
 * by external code.</p>
 */
public class JPypeReferenceSet
{

  /**
   * The size of each pool used to store references.
   */
  static final int SIZE = 256;

  /**
   * A list of pools that store references.
   */
  ArrayList<Pool> pools = new ArrayList<>();

  /**
   * The current pool being used for adding new references.
   */
  Pool current;

  /**
   * The total number of active references in the set.
   */
  private int items;

  /**
   * Constructs a new {@code JPypeReferenceSet}.
   *
   * <p>
   * This constructor initializes the reference set but does not create any
   * pools until references are added.</p>
   */
  JPypeReferenceSet()
  {
  }

  /**
   * Returns the number of active references in the set.
   *
   * @return The number of active references.
   */
  int size()
  {
    return items;
  }

  /**
   * Adds a reference to the set.
   *
   * <p>
   * This operation is performed in constant time (O(1)) by using a pooling
   * mechanism.</p>
   *
   * @param ref The {@link JPypeReference} to add to the set.
   */
  synchronized void add(JPypeReference ref)
  {
    if (ref.cleanup == 0)
    {
      return;
    }

    this.items++;
    if (current == null)
    {
      current = new Pool(pools.size());
      pools.add(current);
    }

    if (current.add(ref))
    {
      // Current pool is full, find a free pool
      current = null;
      for (Pool pool : pools)
      {
        if (pool.tail < SIZE)
        {
          current = pool;
          return;
        }
      }
    }
  }

  /**
   * Removes a reference from the set.
   *
   * <p>
   * This operation is performed in constant time (O(1)) by directly accessing
   * the reference's pool and index.</p>
   *
   * @param ref The {@link JPypeReference} to remove from the set.
   */
  synchronized void remove(JPypeReference ref)
  {
    if (ref.cleanup == 0)
    {
      return;
    }
    pools.get(ref.pool).remove(ref);
    this.items--;
    ref.cleanup = 0;
    ref.pool = -1;
  }

  /**
   * Releases all resources held by the reference set.
   *
   * <p>
   * This method is triggered during JVM shutdown to release all Python
   * references that are still being held. It ensures proper cleanup of Python
   * resources.</p>
   */
  void flush()
  {
    for (Pool pool : pools)
    {
      for (int i = 0; i < pool.tail; ++i)
      {
        JPypeReference ref = pool.entries[i];
        long hostRef = ref.hostReference;
        long cleanup = ref.cleanup;

        // Sanity check to prevent calling cleanup on a null pointer
        if (cleanup == 0)
        {
          continue;
        }
        ref.cleanup = 0;
        JPypeReferenceNative.removeHostReference(hostRef, cleanup);
      }
      pool.tail = 0;
    }
  }

  //<editor-fold desc="internal" defaultstate="collapsed">
  /**
   * A pool that stores references to Python objects.
   *
   * <p>
   * Each pool has a fixed size ({@link JPypeReferenceSet#SIZE}) and is
   * responsible for managing references efficiently. Pools are used to ensure
   * that adding and removing references are performed in constant time
   * (O(1)).</p>
   */
  static class Pool
  {

    /**
     * The array of references stored in this pool.
     */
    JPypeReference[] entries = new JPypeReference[SIZE];

    /**
     * The index of the next available slot in the pool.
     */
    int tail;

    /**
     * The unique ID of this pool.
     */
    int id;

    /**
     * Constructs a new {@code Pool}.
     *
     * @param id The unique ID of the pool.
     */
    Pool(int id)
    {
      this.id = id;
    }

    /**
     * Adds a reference to the pool.
     *
     * <p>
     * This operation is performed in constant time (O(1)).</p>
     *
     * @param ref The {@link JPypeReference} to add to the pool.
     * @return {@code true} if the pool is full after adding the reference;
     * {@code false} otherwise.
     */
    boolean add(JPypeReference ref)
    {
      ref.pool = id;
      ref.index = tail;
      entries[tail++] = ref;
      return (tail == entries.length);
    }

    /**
     * Removes a reference from the pool.
     *
     * <p>
     * This operation is performed in constant time (O(1)) by swapping the
     * reference to be removed with the last reference in the pool.</p>
     *
     * @param ref The {@link JPypeReference} to remove from the pool.
     */
    void remove(JPypeReference ref)
    {
      entries[ref.index] = entries[--tail];
      entries[ref.index].index = ref.index;
    }
  }
  //</editor-fold>
}
