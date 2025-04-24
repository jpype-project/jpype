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
 *
 * @author nelson85
 */
public class JPypeReferenceSet
{

  static final int SIZE = 256;
  ArrayList<Pool> pools = new ArrayList<>();
  Pool current;
  private int items;

  JPypeReferenceSet()
  {
  }

  int size()
  {
    return items;
  }

  /**
   * Add a reference to the set.
   *
   * This should be O(1).
   *
   * @param ref
   */
  synchronized void add(JPypeReference ref)
  {
    if (ref.cleanup == 0)
      return;

    this.items++;
    if (current == null)
    {
      current = new Pool(pools.size());
      pools.add(current);
    }

    if (current.add(ref))
    {
      // It is full
      current = null;

      // Find a free pool
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
   * Remove a reference from the set.
   *
   * @param ref
   */
  synchronized void remove(JPypeReference ref)
  {
    if (ref.cleanup == 0)
      return;
    pools.get(ref.pool).remove(ref);
    this.items--;
    ref.cleanup = 0;
    ref.pool = -1;
  }

  /**
   * Release all resources.
   *
   * This is triggered by shutdown to release an current Python references that
   * are being held.
   *
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
        // This is a sanity check to prevent calling a cleanup with a null
        // pointer, it would only occur if we failed to manage a deleted
        // item.
        if (cleanup == 0)
          continue;
        ref.cleanup = 0;
        JPypeReferenceNative.removeHostReference(hostRef, cleanup);
      }
      pool.tail = 0;
    }
  }

//<editor-fold desc="internal" defaultstate="collapsed">
  static class Pool
  {

    JPypeReference[] entries = new JPypeReference[SIZE];
    int tail;
    int id;

    Pool(int id)
    {
      this.id = id;
    }

    boolean add(JPypeReference ref)
    {
      ref.pool = id;
      ref.index = tail;
      entries[tail++] = ref;
      return (tail == entries.length);
    }

    void remove(JPypeReference ref)
    {
      entries[ref.index] = entries[--tail];
      entries[ref.index].index = ref.index;
    }
  }
//</editor-fold>
}
