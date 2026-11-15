// --- file: org/jpype/ref/GlobalPool.java ---
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

import java.lang.invoke.MethodHandles;
import java.lang.invoke.VarHandle;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Per-interpreter pool of Java object references, addressed by opaque long
 * handles instead of held directly by native code.
 *
 * <p>
 * Native (C++) type wrappers (JPClass, JPMethod, JPField, ...) that used to
 * hold a raw JNI global reference hold one of these handles instead.
 * Ordinary Java reachability from this pool's own array keeps the object
 * alive, so no {@code NewGlobalRef}/{@code DeleteGlobalRef} is needed for
 * them at all - the object is just a normal Java reference for as long as
 * this pool holds it.</p>
 *
 * <p>
 * Each pool stamps its own prefix - assigned once, at construction, from
 * the static counter below, and recorded in the static registry - into
 * every handle it mints. {@link #tryRelease} is a static, pool-agnostic
 * entry point that routes purely off that prefix - the one case that
 * needs it is native code (a C++ destructor) releasing a handle with no
 * other live context in hand.</p>
 *
 * <p>
 * The prefix is only 16 bits, so after 65536 pool constructions in one
 * process it wraps and gets reused by a later, unrelated pool. A handle
 * whose prefix now collides with a different pool than the one that
 * actually minted it is a routing error, not a lifetime one (unlike slot
 * reuse above) - it is a real, if rare, possibility, so each pool also
 * stamps a hash of its owning interpreter's native context pointer,
 * computed once at construction, into the handle. {@link #tryRelease}
 * checks the stamp after resolving the prefix; on mismatch it logs and
 * safely drops the request instead of trusting a same-prefix pool that
 * did not actually mint the handle.</p>
 *
 * <p>
 * Freed slots are reused (see {@link #add}/{@link #remove}), the same as
 * a JNI global ref slot - there is no generation counter guarding against
 * a released handle resolving to whatever now occupies its old slot. That
 * is safe by construction, not by checked accident: a handle is only ever
 * resolved through its live owning C++ wrapper (JPClass/JPMethod/JPField/
 * ...), and that same wrapper's destructor is the only thing that calls
 * {@link #tryRelease} on it, exactly once. Nothing can call {@link #get}
 * on a handle after its owner's destructor has already run - that would
 * require using an already-destroyed C++ object - so a released handle is
 * never resolved again by anything, whether or not its slot has since
 * been reused. Matching JNI's own guarantee level here, not exceeding it.</p>
 *
 * <p>
 * Storage is 64-slot blocks with a {@code long} occupancy bitmask per
 * block (O(1) free-slot search via {@code Long.numberOfTrailingZeros}),
 * and reads ({@link #get}) use {@code VarHandle} acquire/release instead of
 * a lock - only structural mutation ({@link #add}/{@link #remove}) needs
 * to synchronize. This block/bitmask design scales far better under
 * concurrent {@link #get} - the hottest path, since it is called from JNI
 * on effectively every native-to-Java handle resolution - than a naive
 * fully-synchronized list-based allocator would. The prefix/registry
 * contract above is independent of that allocator choice: per-interpreter
 * pool identity is a correct requirement regardless of how slots
 * themselves are allocated.</p>
 */
public final class GlobalPool
{

  static final int BLOCK_SIZE = 64;
  static final int BLOCK_SHIFT = 6;
  static final int BLOCK_MASK = 0x3F;

  private static final int PREFIX_SHIFT = 48;
  private static final int STAMP_SHIFT = 32;
  private static final long INDEX_MASK = 0xFFFFFFFFL;
  private static final long STAMP_MASK = 0xFFFFL;
  private static final long PREFIX_MASK = 0xFFFFL;

  private static final VarHandle AH = MethodHandles.arrayElementVarHandle(Object[].class);
  private static final Logger LOGGER = Logger.getLogger(GlobalPool.class.getName());

  private static final AtomicInteger NEXT_PREFIX = new AtomicInteger(1);
  private static final Map<Integer, GlobalPool> REGISTRY = new ConcurrentHashMap<>();

  private final int prefix;
  private final int stamp;

  // volatile so a growing array (see realloc) is safely published to
  // concurrent lock-free readers in get() without them needing a lock.
  private volatile Object[][] slots;
  private long[] occupied;
  private int blockCount = 1;
  private int currentBlock = 0;

  /**
   * Constructs a new pool with no real owning interpreter to stamp -
   * for tests only. Production pools should use {@link #GlobalPool(long)}
   * so the stamp is actually tied to the owning interpreter's identity.
   */
  public GlobalPool()
  {
    this(ThreadLocalRandom.current().nextLong());
  }

  /**
   * Constructs a new pool and registers it under a freshly-assigned
   * prefix, unique among currently-registered pools.
   *
   * @param contextAddress The owning interpreter's native context
   * pointer - hashed once, here, into every handle this pool mints, so
   * {@link #tryRelease} can detect a prefix collision with a since-closed
   * pool after the 16-bit prefix counter wraps around (see class
   * Javadoc).
   */
  public GlobalPool(long contextAddress)
  {
    int id = NEXT_PREFIX.getAndIncrement();
    if ((id & (int) PREFIX_MASK) == 0)
    {
      // 0 is reserved - never a valid prefix - so skip it on wraparound.
      id = NEXT_PREFIX.getAndIncrement();
    }
    this.prefix = id & (int) PREFIX_MASK;
    this.stamp = (int) (mixStamp(contextAddress) & STAMP_MASK);
    slots = new Object[1][];
    slots[0] = new Object[BLOCK_SIZE];
    occupied = new long[1];
    REGISTRY.put(this.prefix, this);
  }

  /**
   * Removes this pool from the static registry and drops every slot.
   *
   * <p>
   * Called once, from the owning interpreter's own shutdown. After this,
   * {@link #tryRelease} for any handle this pool ever minted safely
   * no-ops instead of touching a dead pool, and {@link #get} likewise
   * returns {@code null}.</p>
   */
  public synchronized void close()
  {
    REGISTRY.remove(prefix);
    slots = new Object[0][];
    occupied = new long[0];
    blockCount = 0;
    currentBlock = 0;
  }

  /**
   * Stores obj and returns a self-checking handle for it.
   *
   * <p>
   * {@code null} is special-cased to the handle {@code 0} without
   * allocating a slot - this lets native code test "is this Java value
   * null" as a cheap {@code == 0} on the handle itself, with no pool
   * lookup/JNI call required (see JPValue::isJavaNull in jp_value.h).</p>
   *
   * @param obj The object to hold a reference to.
   * @return The handle, suitable for {@link #get}/{@link #tryRelease}.
   */
  public synchronized long add(Object obj)
  {
    if (obj == null)
      return 0L;
    if (blockCount == 0 || slots[currentBlock] == null || occupied[currentBlock] == -1L)
      findAvailableBlock();
    int pos = Long.numberOfTrailingZeros(~occupied[currentBlock]);
    AH.setRelease(slots[currentBlock], pos, obj);
    occupied[currentBlock] |= (1L << pos);
    int index = (currentBlock << BLOCK_SHIFT) | pos;
    return encode(prefix, stamp, index);
  }

  /**
   * Resolves handle back to the object it names. Lock-free: reads
   * {@code slots} via volatile-array publication and {@code VarHandle}
   * acquire, taking no lock, so concurrent {@link #get} calls never
   * contend with each other or with {@link #add}/{@link #remove}.
   *
   * @param handle A handle previously returned by {@link #add}.
   * @return The object, or {@code null} if handle doesn't belong to this
   * pool or names a currently-empty slot.
   */
  public Object get(long handle)
  {
    if (decodePrefix(handle) != prefix || decodeStamp(handle) != stamp)
    {
      return null;
    }
    int index = decodeIndex(handle);
    int blockIdx = index >>> BLOCK_SHIFT;
    int elementIdx = index & BLOCK_MASK;

    Object[][] localSlots = this.slots;
    if (blockIdx < 0 || blockIdx >= localSlots.length)
    {
      return null;
    }
    Object[] block = localSlots[blockIdx];
    if (block == null)
    {
      return null;
    }
    return AH.getAcquire(block, elementIdx);
  }

  /** Package-private test hook - not part of the public contract. */
  synchronized int blockCountForTest()
  {
    return blockCount;
  }

  /** Package-private test hook - not part of the public contract. */
  int prefixForTest()
  {
    return prefix;
  }

  private synchronized void remove(long handle)
  {
    int index = decodeIndex(handle);
    int blockIdx = index >>> BLOCK_SHIFT;
    int elementIdx = index & BLOCK_MASK;
    if (blockIdx < 0 || blockIdx >= blockCount || slots[blockIdx] == null)
    {
      return;
    }
    AH.setRelease(slots[blockIdx], elementIdx, null);
    occupied[blockIdx] &= ~(1L << elementIdx);
    currentBlock = Math.min(currentBlock, blockIdx);
  }

  private void findAvailableBlock()
  {
    for (int i = 0; i < blockCount; i++)
      if (occupied[i] != -1L)
      {
        currentBlock = i;
        return;
      }
    allocateNewBlock();
  }

  private void allocateNewBlock()
  {
    if (blockCount >= slots.length)
      realloc(Math.max(1, slots.length * 2));
    slots[blockCount] = new Object[BLOCK_SIZE];
    occupied[blockCount] = 0L;
    currentBlock = blockCount;
    blockCount++;
  }

  private void realloc(int newSize)
  {
    Object[][] newSlots = new Object[newSize][];
    System.arraycopy(slots, 0, newSlots, 0, slots.length);
    long[] newOccupied = new long[newSize];
    System.arraycopy(occupied, 0, newOccupied, 0, occupied.length);
    // Volatile write publishes the new array to concurrent lock-free
    // readers in get() - occupied is only ever touched under the lock, so
    // it doesn't need volatile.
    this.occupied = newOccupied;
    this.slots = newSlots;
  }

  /**
   * Releases a handle, regardless of which pool minted it - the one entry
   * point callable from anywhere (e.g. a C++ destructor with no other
   * live context in hand). Decodes handle's prefix, routes to that pool
   * if it's still registered, and safely no-ops if that interpreter has
   * already torn down.
   *
   * <p>
   * Also checks the handle's stamp against the resolved pool's own stamp
   * before touching anything - a mismatch means the prefix collided with
   * a pool that did not actually mint this handle (see class Javadoc),
   * which gets logged and safely dropped rather than acted on.</p>
   *
   * @param handle A handle previously returned by some pool's
   * {@link #add}.
   */
  public static void tryRelease(long handle)
  {
    GlobalPool pool = REGISTRY.get(decodePrefix(handle));
    if (pool == null)
    {
      return;
    }
    if (decodeStamp(handle) != pool.stamp)
    {
      LOGGER.log(Level.WARNING,
              "GlobalPool.tryRelease: stamp mismatch for handle 0x{0} - "
              + "prefix matched a pool that did not mint it (likely prefix "
              + "wraparound reuse); dropping without releasing anything.",
              Long.toHexString(handle));
      return;
    }
    pool.remove(handle);
  }

  private static long encode(int prefix, int stamp, int index)
  {
    return ((prefix & PREFIX_MASK) << PREFIX_SHIFT)
            | ((stamp & STAMP_MASK) << STAMP_SHIFT)
            | (index & INDEX_MASK);
  }

  private static int decodePrefix(long handle)
  {
    return (int) ((handle >>> PREFIX_SHIFT) & PREFIX_MASK);
  }

  private static int decodeStamp(long handle)
  {
    return (int) ((handle >>> STAMP_SHIFT) & STAMP_MASK);
  }

  private static int decodeIndex(long handle)
  {
    return (int) (handle & INDEX_MASK);
  }

  /**
   * MurmurHash3-style 64-bit finalizer, used to spread a raw context
   * pointer across all bits before masking down to the stamp's 16 - a
   * plain {@code Long.hashCode} would leave the low bits of a
   * sequential/aligned pointer largely unchanged, which is exactly the
   * part {@link #STAMP_MASK} keeps.
   */
  private static long mixStamp(long x)
  {
    x ^= x >>> 33;
    x *= 0xff51afd7ed558ccdL;
    x ^= x >>> 33;
    x *= 0xc4ceb9fe1a85ec55L;
    x ^= x >>> 33;
    return x;
  }
}
