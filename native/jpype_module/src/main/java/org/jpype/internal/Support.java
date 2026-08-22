/*****************************************************************************
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
 *****************************************************************************/
package org.jpype.internal;

import java.lang.reflect.Array;
import java.net.URISyntaxException;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.nio.DoubleBuffer;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.nio.LongBuffer;
import java.nio.ShortBuffer;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.IntStream;
import org.jpype.PyExceptionProxy;
import org.jpype.annotation.Exported;

/**
 * Static helpers for multi-dimensional primitive array transfer, called
 * exclusively from C++ (see JPJavaFrame::collectRectangular/assemble/
 * fillMultiArrayFromBuffer/collectMultiArrayToBuffer). Split out of
 * JPypeContext so this can be looked up and invoked as plain static
 * methods (GetStaticMethodID) instead of round-tripping through the
 * context instance.
 */
class Support
{

  private Support()
  {
  }

  /**
   * The directory containing the jar (or class directory) c was loaded
   * from. Restored from the pre-merge Support.java (was silently dropped
   * during the array-transfer-phase3/reverse reconciliation - its own
   * test, SupportNGTest, still referenced it) - not currently called from
   * C++, but a small self-contained utility worth keeping alongside its
   * test rather than deleting the test.
   */
  @Exported
  public static Path getJarPath(Class<?> c)
  {
    try
    {
      return Paths.get(c.getProtectionDomain().getCodeSource().getLocation()
              .toURI()).getParent();
    } catch (URISyntaxException ex)
    {
      return null;
    }
  }

  /**
   * Helper function for collect rectangular.
   */
  private static boolean collect(List<Object> l, Object o, int q, int[] shape, int d)
  {
    if (Array.getLength(o) != shape[q])
      return false;
    if (q + 1 == d)
    {
      l.add(o);
      return true;
    }
    for (int i = 0; i < shape[q]; ++i)
    {
      if (!collect(l, Array.get(o, i), q + 1, shape, d))
        return false;
    }
    return true;
  }

  /**
   * Collect up a rectangular primitive array for a Python memory view.
   *
   * If it is a rectangular primitive array then the result will be an object
   * array containing. - the primitive type - an int array with the shape of the
   * array - each of the primitive arrays that will need be visited in order.
   *
   * This is the safest way to provide a view as we are verifying and collected
   * thus even if something mutates the shape of the array after we have
   * visited, we have a locked copy.
   *
   * @param o is the object to be tested.
   * @return null if the object is not a rectangular primitive array.
   */
  public static Object[] collectRectangular(Object o)
  {
    if (o == null || !o.getClass().isArray())
      return null;

    // We only support flattening up to 4 dimensions for fast transfer
    int[] shape = new int[4];
    int d = 0;

    Object o1 = o;
    Class<?> c1 = o1.getClass();
    while (c1.isArray())
    {
      // If we hit a 5th nested dimension, immediately reject it before doing work
      if (d == 4)
        return null;

      int l = Array.getLength(o1);
      if (l == 0)
        return null;

      shape[d++] = l;
      o1 = Array.get(o1, 0);
      if (o1 == null)
        return null;

      c1 = c1.getComponentType();
    }

    if (!c1.isPrimitive())
      return null;

    ArrayList<Object> out = new ArrayList<>();
    out.add(c1);

    shape = Arrays.copyOfRange(shape, 0, d);
    out.add(shape);

    int total = 1;
    for (int i = 0; i < d - 1; i++)
      total *= shape[i];
    out.ensureCapacity(total + 2);

    if (!collect(out, o, 0, shape, d))
      return null;

    return out.toArray();
  }

  public static Object unpack(int size, Object parts)
  {
    Object e0 = Array.get(parts, 0);
    Class<?> c = e0.getClass();
    int segments = Array.getLength(parts) / size;
    Object a2;
    Object a1 = Array.newInstance(Array.newInstance(c, size).getClass(), segments);
    int k = 0;
    for (int i = 0; i < segments; i++)
    {
      a2 = Array.newInstance(c, size);

      for (int j = 0; j < size; j++, k++)
      {
        Object o = Array.get(parts, k);
        Array.set(a2, j, o);
      }

      Array.set(a1, i, a2);
    }
    return a1;
  }

  public static Object assemble(int[] dims, Object parts)
  {
    int n = dims.length;
    if (n == 1)
      return Array.get(parts, 0);
    if (n == 2)
      return Array.get(unpack(dims[0], parts), 0);
    for (int i = 0; i < n - 2; ++i)
    {
      parts = unpack(dims[n - i - 2], parts);
    }
    return parts;
  }

  // ---- Buffer-handoff multi-dim push/pull.
  //
  // Both methods below replace an O(leaf-array-count) JNI-call sequence
  // (one GetPrimitiveArrayCritical-pinned pack loop per leaf on push, one
  // reflective Array.get + one Get<Type>ArrayRegion per leaf on pull) with
  // a *single* JNI entry into one of these methods, after which
  // everything is plain Java: bulk java.nio typed-buffer reads/writes
  // (IntBuffer.get/put etc.) against a caller-supplied direct buffer, no
  // further JNI calls at all. Validated by the 3.3b/3.4 experiments
  // (DeepBench.java's fillBuffer*/collectBuffer* prototypes, now
  // superseded by these) -- serial wins unconditionally over the old
  // per-leaf approach; parallel (IntStream.parallel()) only pays off
  // above roughly 1e6-1e7 total elements. That decision is made here, in
  // Java (leafRange() below), not by the C++ caller -- C++ has no better
  // way to reason about IntStream/ForkJoinPool dispatch cost than Java
  // does, and total element count is already known on this side once the
  // shape is in hand, so there's no reason to compute it twice or thread
  // a boolean across the JNI boundary.
  //
  // `typeCode` is the JNI primitive type signature character
  // (Z/B/C/S/I/J/F/D, see JPPrimitiveType::getTypeCode()). The caller
  // must have already set the buffer's contents in host byte order, or
  // -- as here -- this method sets `.order(nativeOrder())` on its own
  // view before reading/writing, since a fresh direct buffer (JNI
  // NewDirectByteBuffer) always defaults to big-endian regardless of
  // platform (a real bug caught during the 3.4 experiment's sanity
  // check, not just defensive boilerplate).
  //
  // There is no BooleanBuffer in java.nio -- boolean is the one type
  // handled as a manual per-element byte loop instead of a bulk typed
  // get/put, still with no JNI/reflection per element or per leaf.

  // Deliberately conservative -- the 3.3b/3.4 experiments found the real
  // crossover ranges from ~1e6 (push) to ~1e7 (pull) elements depending on
  // per-leaf task size, not just total count, so a single shared
  // threshold errs toward "not worth it yet" rather than risking the far
  // worse case (measured up to ~35x slower) of dispatching parallel work
  // that doesn't pay for itself.
  private static final long PARALLEL_THRESHOLD_ELEMENTS = 1_000_000L;

  private static IntStream leafRange(int leaves, int leafLength)
  {
    IntStream range = IntStream.range(0, leaves);
    long total = (long) leaves * leafLength;
    return total >= PARALLEL_THRESHOLD_ELEMENTS ? range.parallel() : range;
  }

  // Mirrors JPRawTransferMode in native/common/include/jpype.h -- kept in
  // sync by hand, there being no shared header between the two languages.
  // RAW_NONE (0) never reaches here: the C++ caller only takes this path
  // (rather than the general element-by-element converter path) when it
  // resolved something other than NONE.
  private static final int RAW_NATIVE = 1;
  private static final int RAW_SWAPPED = 2;
  private static final int RAW_HALF_NATIVE = 3;
  private static final int RAW_HALF_SWAPPED = 4;

  private static ByteOrder swapped(ByteOrder order)
  {
    return order == ByteOrder.BIG_ENDIAN ? ByteOrder.LITTLE_ENDIAN : ByteOrder.BIG_ENDIAN;
  }

  /**
   * Decode a single IEEE 754 binary16 (numpy float16 / Python 'e' format)
   * value into its exact float32 equivalent. Standard bit-twiddling
   * decode (subnormal/normal/inf-or-nan branches) -- there is no half
   * type or ByteBuffer support in java.nio to lean on instead.
   */
  private static float halfToFloat(short bits)
  {
    int h = bits & 0xFFFF;
    int sign = (h & 0x8000) << 16;
    int exp = (h & 0x7C00) >> 10;
    int frac = h & 0x03FF;
    if (exp == 0)
    {
      if (frac == 0)
        return Float.intBitsToFloat(sign);
      // Subnormal half -> normalize into a normal float32.
      int e = -1;
      do
      {
        e++;
        frac <<= 1;
      } while ((frac & 0x0400) == 0);
      frac &= 0x03FF;
      int exp32 = 127 - 15 - e;
      return Float.intBitsToFloat(sign | (exp32 << 23) | (frac << 13));
    }
    if (exp == 0x1F)
      return Float.intBitsToFloat(sign | 0x7F800000 | (frac << 13));
    return Float.intBitsToFloat(sign | ((exp - 15 + 127) << 23) | (frac << 13));
  }

  /**
   * Build a rectangular multi-dimensional primitive array of the given
   * shape from a flat, C-contiguous direct buffer -- the push-side half
   * of the buffer-handoff redesign. `src`'s declared byte order is not
   * meaningful yet (a fresh NewDirectByteBuffer always defaults to
   * big-endian regardless of platform); `mode` says how to interpret it.
   *
   * @param typeCode primitive type signature character (the *target*
   * type -- for RAW_HALF_* this differs from the source's own type,
   * which is always 16-bit float).
   * @param mode one of RAW_NATIVE/RAW_SWAPPED/RAW_HALF_NATIVE/RAW_HALF_SWAPPED.
   * @param src a direct buffer over the source's total element count.
   * @param shape the array's shape, outermost dimension first.
   * @return the assembled array (e.g. int[][] for a 2D shape).
   */
  public static Object fillFromBuffer(char typeCode, int mode, ByteBuffer src, int[] shape)
  {
    if (mode == RAW_HALF_NATIVE || mode == RAW_HALF_SWAPPED)
    {
      src.order(mode == RAW_HALF_NATIVE ? ByteOrder.nativeOrder() : swapped(ByteOrder.nativeOrder()));
      return fillFromHalfBuffer(typeCode, src, shape);
    }
    src.order(mode == RAW_SWAPPED ? swapped(ByteOrder.nativeOrder()) : ByteOrder.nativeOrder());
    int dims = shape.length;
    int last = shape[dims - 1];
    int leaves = 1;
    for (int i = 0; i < dims - 1; i++)
      leaves *= shape[i];

    IntStream range = leafRange(leaves, last);

    Object flat;
    switch (typeCode)
    {
      case 'I':
      {
        IntBuffer buf = src.asIntBuffer();
        int[][] out = new int[leaves][];
        range.forEach(i ->
        {
          int[] row = new int[last];
          IntBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'D':
      {
        DoubleBuffer buf = src.asDoubleBuffer();
        double[][] out = new double[leaves][];
        range.forEach(i ->
        {
          double[] row = new double[last];
          DoubleBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'J':
      {
        LongBuffer buf = src.asLongBuffer();
        long[][] out = new long[leaves][];
        range.forEach(i ->
        {
          long[] row = new long[last];
          LongBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'F':
      {
        FloatBuffer buf = src.asFloatBuffer();
        float[][] out = new float[leaves][];
        range.forEach(i ->
        {
          float[] row = new float[last];
          FloatBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'S':
      {
        ShortBuffer buf = src.asShortBuffer();
        short[][] out = new short[leaves][];
        range.forEach(i ->
        {
          short[] row = new short[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'C':
      {
        CharBuffer buf = src.asCharBuffer();
        char[][] out = new char[leaves][];
        range.forEach(i ->
        {
          char[] row = new char[last];
          CharBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'B':
      {
        byte[][] out = new byte[leaves][];
        range.forEach(i ->
        {
          byte[] row = new byte[last];
          ByteBuffer dup = src.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'Z':
      {
        boolean[][] out = new boolean[leaves][];
        range.forEach(i ->
        {
          boolean[] row = new boolean[last];
          ByteBuffer dup = src.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = dup.get() != 0;
          out[i] = row;
        });
        flat = out;
        break;
      }
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }

    // assemble()/unpack() work by reflection (Array.get/Array.set), so a
    // concretely-typed leaf array (int[][], double[][], ...) works
    // exactly like the Object[] they were originally written for -- no
    // changes needed there. Nesting itself is O(leaf count), already
    // cheap, not the part this redesign targets.
    return assemble(shape, flat);
  }

  /**
   * fillFromBuffer's RAW_HALF_NATIVE/RAW_HALF_SWAPPED case: `src` holds
   * IEEE 754 binary16 (numpy float16) values rather than typeCode's own
   * native encoding, so every element needs an actual decode (there is no
   * bulk java.nio path for a type java.nio doesn't know about) -- still
   * one JNI entry and no per-row critical sections, just a per-element
   * decode+cast done in bulk Java instead of one converter() call per
   * element back on the C++ side.
   */
  private static Object fillFromHalfBuffer(char typeCode, ByteBuffer src, int[] shape)
  {
    int dims = shape.length;
    int last = shape[dims - 1];
    int leaves = 1;
    for (int i = 0; i < dims - 1; i++)
      leaves *= shape[i];

    ShortBuffer buf = src.asShortBuffer();
    IntStream range = leafRange(leaves, last);

    Object flat;
    switch (typeCode)
    {
      case 'D':
      {
        double[][] out = new double[leaves][];
        range.forEach(i ->
        {
          double[] row = new double[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'F':
      {
        float[][] out = new float[leaves][];
        range.forEach(i ->
        {
          float[] row = new float[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'J':
      {
        long[][] out = new long[leaves][];
        range.forEach(i ->
        {
          long[] row = new long[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (long) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'I':
      {
        int[][] out = new int[leaves][];
        range.forEach(i ->
        {
          int[] row = new int[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (int) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'S':
      {
        short[][] out = new short[leaves][];
        range.forEach(i ->
        {
          short[] row = new short[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (short) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'C':
      {
        char[][] out = new char[leaves][];
        range.forEach(i ->
        {
          char[] row = new char[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (char) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'B':
      {
        byte[][] out = new byte[leaves][];
        range.forEach(i ->
        {
          byte[] row = new byte[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (byte) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'Z':
      {
        boolean[][] out = new boolean[leaves][];
        range.forEach(i ->
        {
          boolean[] row = new boolean[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = halfToFloat(dup.get()) != 0;
          out[i] = row;
        });
        flat = out;
        break;
      }
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }
    return assemble(shape, flat);
  }

  // Source kind codes for fillFlatFromBuffer -- deliberately not the same
  // alphabet as typeCode (which names a *target* Java primitive): a source
  // is fully described by signedness/float-ness plus width, not by which
  // of Java's 8 primitive types it happens to resemble. Kept to 3 letters
  // (signed/unsigned int, float -- half-precision is float at width 2)
  // rather than one-per-width so the read helpers below don't need a
  // combinatorial switch on both source and target type: every source
  // reduces to a `long` (int kinds) or `double` (float kinds), and every
  // target does one cast from that common intermediate, an O(kinds+targets)
  // shape instead of O(kinds*targets).
  private static final char SRC_SIGNED = 'i';
  private static final char SRC_UNSIGNED = 'u';
  private static final char SRC_FLOAT = 'f';

  /**
   * Flat (1-D) counterpart to {@link #fillFromBuffer} -- the fast path for
   * {@code JPConversionBuffer} (a buffer-protocol object, chiefly numpy,
   * passed as a Java method argument matching a flat primitive array
   * parameter, e.g. {@code int[]}).
   *
   * Two things this does that the multi-dim buffer-handoff path doesn't
   * need to: (1) takes an explicit {@code strideBytes} instead of requiring
   * a C-contiguous source, so a non-contiguous source (a strided/sliced
   * numpy column, `arr[::2]`, ...) still reaches this single-JNI-crossing
   * path -- there is no reshape here (output is always flat), so there is
   * no reason to demand contiguity the way the multi-dim case does; (2)
   * does the full dtype coercion (float64->int32, int16->double, ...) here
   * in Java rather than only handling the byte-identical "raw" cases and
   * falling back to a per-element {@code converter()} call on the C++ side
   * (`JPIntType::setArrayRange` and its 7 siblings) for everything else.
   * Byte-order handling is already "free" here (one {@code ByteBuffer.order()}
   * call covers the whole transfer, same as the multi-dim path), so
   * widening/narrowing/int-float coercion is the only genuinely new
   * per-element work versus a raw copy, and Java's own numeric cast
   * operators do that as cheaply as hand-written C++ ever could.
   *
   * Uses {@code ByteBuffer}'s absolute positional get methods
   * ({@code getInt(index)} etc., available since the buffer API's
   * introduction, unlike the bulk absolute get added in Java 13) so a
   * non-unit stride costs one extra multiply per element and nothing else
   * -- still zero further JNI/reflection calls.
   *
   * @param typeCode primitive type signature character of the *target*
   * array (Z/B/C/S/I/J/F/D).
   * @param srcKind one of SRC_SIGNED/SRC_UNSIGNED/SRC_FLOAT -- the source
   * element's own kind, as classified from the buffer's format string by
   * the C++ caller (mirrors {@code getConverter}'s own format parsing, see
   * {@code classifyBufferSource} in jp_convert.cpp).
   * @param srcSize the source element's width in bytes (1/2/4/8 for
   * int kinds, 4/8 for float, 2 for half-precision float).
   * @param swapped whether the source's declared byte order differs from
   * native.
   * @param src a direct buffer spanning exactly the accessed source bytes.
   * @param length the number of elements to read.
   * @param strideBytes the byte distance between consecutive source
   * elements; equal to {@code srcSize} for a contiguous source.
   * @return the assembled flat array (e.g. {@code int[]}).
   */
  public static Object fillFlatFromBuffer(char typeCode, char srcKind, int srcSize, boolean swapped,
          ByteBuffer src, int length, int strideBytes)
  {
    src.order(swapped ? swapped(ByteOrder.nativeOrder()) : ByteOrder.nativeOrder());

    // Fast bulk path: the overwhelmingly common case (a numpy array whose
    // dtype already matches the target primitive exactly, contiguous) --
    // one bulk typed-buffer get instead of a per-element read+cast.
    if (strideBytes == srcSize)
    {
      switch (typeCode)
      {
        case 'I':
          if (srcKind == SRC_SIGNED && srcSize == 4)
          {
            int[] out = new int[length];
            src.asIntBuffer().get(out, 0, length);
            return out;
          }
          break;
        case 'J':
          if (srcKind == SRC_SIGNED && srcSize == 8)
          {
            long[] out = new long[length];
            src.asLongBuffer().get(out, 0, length);
            return out;
          }
          break;
        case 'F':
          if (srcKind == SRC_FLOAT && srcSize == 4)
          {
            float[] out = new float[length];
            src.asFloatBuffer().get(out, 0, length);
            return out;
          }
          break;
        case 'D':
          if (srcKind == SRC_FLOAT && srcSize == 8)
          {
            double[] out = new double[length];
            src.asDoubleBuffer().get(out, 0, length);
            return out;
          }
          break;
        default:
          break;
      }
    }

    return srcKind == SRC_FLOAT
            ? fillFlatFromFloatSrc(typeCode, srcSize, src, length, strideBytes)
            : fillFlatFromIntSrc(typeCode, srcKind == SRC_UNSIGNED, srcSize, src, length, strideBytes);
  }

  /**
   * Bulk-read every source element into a sign/zero-extended {@code
   * long[]} in one pass -- the srcSize/unsignedSrc dispatch happens once
   * here, not per element: it's fixed for the whole call, so branching on
   * it inside a tight per-element loop would just be the same predictable
   * branch taken length times for no benefit. Used for every integer
   * target (Z/B/C/S/I/J); narrowing/widening to the eventual target
   * happens afterward via a plain per-element cast over this array, which
   * is the one loop left with per-element work (unavoidable -- it's the
   * actual data-dependent part) but no per-element *branching*.
   */
  private static long[] readLongs(ByteBuffer src, int length, int strideBytes,
          boolean unsignedSrc, int srcSize)
  {
    long[] out = new long[length];
    switch (srcSize)
    {
      case 1:
        if (unsignedSrc)
          readBytesUnsignedUnrolled(src, out, length, strideBytes);
        else
          readBytesSignedUnrolled(src, out, length, strideBytes);
        break;
      case 2:
        if (unsignedSrc)
          readShortsUnsignedUnrolled(src, out, length, strideBytes);
        else
          readShortsSignedUnrolled(src, out, length, strideBytes);
        break;
      case 4:
        if (unsignedSrc)
          readIntsUnsignedUnrolled(src, out, length, strideBytes);
        else
          readIntsSignedUnrolled(src, out, length, strideBytes);
        break;
      default: // 8 bytes, always read as a plain signed long -- see
        // readDoublesFromInt for why unsigned 64-bit needs separate
        // handling only when the *target* is float/double.
        readLongsSignedUnrolled(src, out, length, strideBytes);
        break;
    }
    return out;
  }

  // ---- Unrolled ByteBuffer readers, 8 elements per pass -- each backs
  // both readLongs and readDoublesFromInt (the widening int-to-double
  // reads below reuse the same shape, differing only in the array element
  // type and the sign-extension/mask applied), same loop-count-reduction
  // rationale as the write side.

  private static void readBytesSignedUnrolled(ByteBuffer src, long[] out, int length, int strideBytes)
  {
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.get(off);
      out[i + 1] = src.get(off + strideBytes);
      out[i + 2] = src.get(off + 2 * strideBytes);
      out[i + 3] = src.get(off + 3 * strideBytes);
      out[i + 4] = src.get(off + 4 * strideBytes);
      out[i + 5] = src.get(off + 5 * strideBytes);
      out[i + 6] = src.get(off + 6 * strideBytes);
      out[i + 7] = src.get(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.get(off);
  }

  private static void readBytesUnsignedUnrolled(ByteBuffer src, long[] out, int length, int strideBytes)
  {
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.get(off) & 0xFF;
      out[i + 1] = src.get(off + strideBytes) & 0xFF;
      out[i + 2] = src.get(off + 2 * strideBytes) & 0xFF;
      out[i + 3] = src.get(off + 3 * strideBytes) & 0xFF;
      out[i + 4] = src.get(off + 4 * strideBytes) & 0xFF;
      out[i + 5] = src.get(off + 5 * strideBytes) & 0xFF;
      out[i + 6] = src.get(off + 6 * strideBytes) & 0xFF;
      out[i + 7] = src.get(off + 7 * strideBytes) & 0xFF;
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.get(off) & 0xFF;
  }

  private static void readShortsSignedUnrolled(ByteBuffer src, long[] out, int length, int strideBytes)
  {
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getShort(off);
      out[i + 1] = src.getShort(off + strideBytes);
      out[i + 2] = src.getShort(off + 2 * strideBytes);
      out[i + 3] = src.getShort(off + 3 * strideBytes);
      out[i + 4] = src.getShort(off + 4 * strideBytes);
      out[i + 5] = src.getShort(off + 5 * strideBytes);
      out[i + 6] = src.getShort(off + 6 * strideBytes);
      out[i + 7] = src.getShort(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getShort(off);
  }

  private static void readShortsUnsignedUnrolled(ByteBuffer src, long[] out, int length, int strideBytes)
  {
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getShort(off) & 0xFFFF;
      out[i + 1] = src.getShort(off + strideBytes) & 0xFFFF;
      out[i + 2] = src.getShort(off + 2 * strideBytes) & 0xFFFF;
      out[i + 3] = src.getShort(off + 3 * strideBytes) & 0xFFFF;
      out[i + 4] = src.getShort(off + 4 * strideBytes) & 0xFFFF;
      out[i + 5] = src.getShort(off + 5 * strideBytes) & 0xFFFF;
      out[i + 6] = src.getShort(off + 6 * strideBytes) & 0xFFFF;
      out[i + 7] = src.getShort(off + 7 * strideBytes) & 0xFFFF;
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getShort(off) & 0xFFFF;
  }

  private static void readIntsSignedUnrolled(ByteBuffer src, long[] out, int length, int strideBytes)
  {
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getInt(off);
      out[i + 1] = src.getInt(off + strideBytes);
      out[i + 2] = src.getInt(off + 2 * strideBytes);
      out[i + 3] = src.getInt(off + 3 * strideBytes);
      out[i + 4] = src.getInt(off + 4 * strideBytes);
      out[i + 5] = src.getInt(off + 5 * strideBytes);
      out[i + 6] = src.getInt(off + 6 * strideBytes);
      out[i + 7] = src.getInt(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getInt(off);
  }

  private static void readIntsUnsignedUnrolled(ByteBuffer src, long[] out, int length, int strideBytes)
  {
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getInt(off) & 0xFFFFFFFFL;
      out[i + 1] = src.getInt(off + strideBytes) & 0xFFFFFFFFL;
      out[i + 2] = src.getInt(off + 2 * strideBytes) & 0xFFFFFFFFL;
      out[i + 3] = src.getInt(off + 3 * strideBytes) & 0xFFFFFFFFL;
      out[i + 4] = src.getInt(off + 4 * strideBytes) & 0xFFFFFFFFL;
      out[i + 5] = src.getInt(off + 5 * strideBytes) & 0xFFFFFFFFL;
      out[i + 6] = src.getInt(off + 6 * strideBytes) & 0xFFFFFFFFL;
      out[i + 7] = src.getInt(off + 7 * strideBytes) & 0xFFFFFFFFL;
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getInt(off) & 0xFFFFFFFFL;
  }

  private static void readLongsSignedUnrolled(ByteBuffer src, long[] out, int length, int strideBytes)
  {
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getLong(off);
      out[i + 1] = src.getLong(off + strideBytes);
      out[i + 2] = src.getLong(off + 2 * strideBytes);
      out[i + 3] = src.getLong(off + 3 * strideBytes);
      out[i + 4] = src.getLong(off + 4 * strideBytes);
      out[i + 5] = src.getLong(off + 5 * strideBytes);
      out[i + 6] = src.getLong(off + 6 * strideBytes);
      out[i + 7] = src.getLong(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getLong(off);
  }

  /**
   * Bulk-read every source element into a {@code double[]} in one pass,
   * same srcSize/unsignedSrc-hoisting rationale as {@link #readLongs} --
   * used only for a float/double *target*, where an unsigned 64-bit
   * source needs real handling {@link #readLongs} can't provide: its
   * magnitude can exceed {@code Long.MAX_VALUE}, so a plain {@code
   * (double) readLongs(...)[i]} would reinterpret the bit pattern as
   * negative. Every other combination (unsigned width < 8, or signed of
   * any width) already fits a `long` with its true value intact, so a
   * plain widening cast is exact there.
   */
  private static double unsignedLongBitsToDouble(long v)
  {
    // v's bit pattern is the true unsigned value; v >= 0 means it also
    // fits as a non-negative signed long, so the plain cast is exact.
    // v < 0 means the true value is 2^64 + v -- shift right one
    // (unsigned) bit to halve it into signed range, convert, then
    // reconstruct by doubling and adding back the dropped low bit.
    return v >= 0 ? (double) v : ((double) (v >>> 1)) * 2.0 + (v & 1L);
  }

  /**
   * Direct single-pass unrolled reads straight to {@code double}, one
   * branch per srcSize/unsignedSrc combination -- deliberately does NOT
   * delegate to {@link #readLongs} plus a widening pass: measured that
   * shortcut costing a full extra array allocation and pass (int32->double
   * slice assignment at 100,000 elements: ~210,000ns direct vs.
   * ~280,000ns via readLongs+widen), so the mechanical duplication of the
   * 7 branches below (as named helpers, same shape as {@link #readLongs}'s
   * own byte/short/int/long readers) is paying for something real, not
   * just tidiness.
   */
  private static double[] readDoublesFromInt(ByteBuffer src, int length, int strideBytes,
          boolean unsignedSrc, int srcSize)
  {
    if (unsignedSrc && srcSize == 8)
      return readUnsignedLongsAsDoublesUnrolled(src, length, strideBytes);
    switch (srcSize)
    {
      case 1:
        return unsignedSrc
                ? readBytesUnsignedAsDoublesUnrolled(src, length, strideBytes)
                : readBytesSignedAsDoublesUnrolled(src, length, strideBytes);
      case 2:
        return unsignedSrc
                ? readShortsUnsignedAsDoublesUnrolled(src, length, strideBytes)
                : readShortsSignedAsDoublesUnrolled(src, length, strideBytes);
      case 4:
        return unsignedSrc
                ? readIntsUnsignedAsDoublesUnrolled(src, length, strideBytes)
                : readIntsSignedAsDoublesUnrolled(src, length, strideBytes);
      default: // 8 bytes, signed (unsigned already handled above)
        return readLongsSignedAsDoublesUnrolled(src, length, strideBytes);
    }
  }

  private static double[] readUnsignedLongsAsDoublesUnrolled(ByteBuffer src, int length, int strideBytes)
  {
    double[] out = new double[length];
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = unsignedLongBitsToDouble(src.getLong(off));
      out[i + 1] = unsignedLongBitsToDouble(src.getLong(off + strideBytes));
      out[i + 2] = unsignedLongBitsToDouble(src.getLong(off + 2 * strideBytes));
      out[i + 3] = unsignedLongBitsToDouble(src.getLong(off + 3 * strideBytes));
      out[i + 4] = unsignedLongBitsToDouble(src.getLong(off + 4 * strideBytes));
      out[i + 5] = unsignedLongBitsToDouble(src.getLong(off + 5 * strideBytes));
      out[i + 6] = unsignedLongBitsToDouble(src.getLong(off + 6 * strideBytes));
      out[i + 7] = unsignedLongBitsToDouble(src.getLong(off + 7 * strideBytes));
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = unsignedLongBitsToDouble(src.getLong(off));
    return out;
  }

  private static double[] readBytesSignedAsDoublesUnrolled(ByteBuffer src, int length, int strideBytes)
  {
    double[] out = new double[length];
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.get(off);
      out[i + 1] = src.get(off + strideBytes);
      out[i + 2] = src.get(off + 2 * strideBytes);
      out[i + 3] = src.get(off + 3 * strideBytes);
      out[i + 4] = src.get(off + 4 * strideBytes);
      out[i + 5] = src.get(off + 5 * strideBytes);
      out[i + 6] = src.get(off + 6 * strideBytes);
      out[i + 7] = src.get(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.get(off);
    return out;
  }

  private static double[] readBytesUnsignedAsDoublesUnrolled(ByteBuffer src, int length, int strideBytes)
  {
    double[] out = new double[length];
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.get(off) & 0xFF;
      out[i + 1] = src.get(off + strideBytes) & 0xFF;
      out[i + 2] = src.get(off + 2 * strideBytes) & 0xFF;
      out[i + 3] = src.get(off + 3 * strideBytes) & 0xFF;
      out[i + 4] = src.get(off + 4 * strideBytes) & 0xFF;
      out[i + 5] = src.get(off + 5 * strideBytes) & 0xFF;
      out[i + 6] = src.get(off + 6 * strideBytes) & 0xFF;
      out[i + 7] = src.get(off + 7 * strideBytes) & 0xFF;
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.get(off) & 0xFF;
    return out;
  }

  private static double[] readShortsSignedAsDoublesUnrolled(ByteBuffer src, int length, int strideBytes)
  {
    double[] out = new double[length];
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getShort(off);
      out[i + 1] = src.getShort(off + strideBytes);
      out[i + 2] = src.getShort(off + 2 * strideBytes);
      out[i + 3] = src.getShort(off + 3 * strideBytes);
      out[i + 4] = src.getShort(off + 4 * strideBytes);
      out[i + 5] = src.getShort(off + 5 * strideBytes);
      out[i + 6] = src.getShort(off + 6 * strideBytes);
      out[i + 7] = src.getShort(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getShort(off);
    return out;
  }

  private static double[] readShortsUnsignedAsDoublesUnrolled(ByteBuffer src, int length, int strideBytes)
  {
    double[] out = new double[length];
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getShort(off) & 0xFFFF;
      out[i + 1] = src.getShort(off + strideBytes) & 0xFFFF;
      out[i + 2] = src.getShort(off + 2 * strideBytes) & 0xFFFF;
      out[i + 3] = src.getShort(off + 3 * strideBytes) & 0xFFFF;
      out[i + 4] = src.getShort(off + 4 * strideBytes) & 0xFFFF;
      out[i + 5] = src.getShort(off + 5 * strideBytes) & 0xFFFF;
      out[i + 6] = src.getShort(off + 6 * strideBytes) & 0xFFFF;
      out[i + 7] = src.getShort(off + 7 * strideBytes) & 0xFFFF;
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getShort(off) & 0xFFFF;
    return out;
  }

  private static double[] readIntsSignedAsDoublesUnrolled(ByteBuffer src, int length, int strideBytes)
  {
    double[] out = new double[length];
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getInt(off);
      out[i + 1] = src.getInt(off + strideBytes);
      out[i + 2] = src.getInt(off + 2 * strideBytes);
      out[i + 3] = src.getInt(off + 3 * strideBytes);
      out[i + 4] = src.getInt(off + 4 * strideBytes);
      out[i + 5] = src.getInt(off + 5 * strideBytes);
      out[i + 6] = src.getInt(off + 6 * strideBytes);
      out[i + 7] = src.getInt(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getInt(off);
    return out;
  }

  private static double[] readIntsUnsignedAsDoublesUnrolled(ByteBuffer src, int length, int strideBytes)
  {
    double[] out = new double[length];
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getInt(off) & 0xFFFFFFFFL;
      out[i + 1] = src.getInt(off + strideBytes) & 0xFFFFFFFFL;
      out[i + 2] = src.getInt(off + 2 * strideBytes) & 0xFFFFFFFFL;
      out[i + 3] = src.getInt(off + 3 * strideBytes) & 0xFFFFFFFFL;
      out[i + 4] = src.getInt(off + 4 * strideBytes) & 0xFFFFFFFFL;
      out[i + 5] = src.getInt(off + 5 * strideBytes) & 0xFFFFFFFFL;
      out[i + 6] = src.getInt(off + 6 * strideBytes) & 0xFFFFFFFFL;
      out[i + 7] = src.getInt(off + 7 * strideBytes) & 0xFFFFFFFFL;
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getInt(off) & 0xFFFFFFFFL;
    return out;
  }

  private static double[] readLongsSignedAsDoublesUnrolled(ByteBuffer src, int length, int strideBytes)
  {
    double[] out = new double[length];
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getLong(off);
      out[i + 1] = src.getLong(off + strideBytes);
      out[i + 2] = src.getLong(off + 2 * strideBytes);
      out[i + 3] = src.getLong(off + 3 * strideBytes);
      out[i + 4] = src.getLong(off + 4 * strideBytes);
      out[i + 5] = src.getLong(off + 5 * strideBytes);
      out[i + 6] = src.getLong(off + 6 * strideBytes);
      out[i + 7] = src.getLong(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getLong(off);
    return out;
  }

  /**
   * Bulk-read every source element into a {@code double[]} in one pass --
   * the float-kind sibling of {@link #readDoublesFromInt}, srcSize
   * (2 = half, 4 = float, 8 = double) dispatched once, not per element.
   */
  private static double[] readDoublesFromFloat(ByteBuffer src, int length, int strideBytes, int srcSize)
  {
    double[] out = new double[length];
    int i = 0, off = 0;
    switch (srcSize)
    {
      case 2:
        for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
        {
          out[i] = halfToFloat(src.getShort(off));
          out[i + 1] = halfToFloat(src.getShort(off + strideBytes));
          out[i + 2] = halfToFloat(src.getShort(off + 2 * strideBytes));
          out[i + 3] = halfToFloat(src.getShort(off + 3 * strideBytes));
          out[i + 4] = halfToFloat(src.getShort(off + 4 * strideBytes));
          out[i + 5] = halfToFloat(src.getShort(off + 5 * strideBytes));
          out[i + 6] = halfToFloat(src.getShort(off + 6 * strideBytes));
          out[i + 7] = halfToFloat(src.getShort(off + 7 * strideBytes));
        }
        for (; i < length; i++, off += strideBytes)
          out[i] = halfToFloat(src.getShort(off));
        break;
      case 4:
        for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
        {
          out[i] = src.getFloat(off);
          out[i + 1] = src.getFloat(off + strideBytes);
          out[i + 2] = src.getFloat(off + 2 * strideBytes);
          out[i + 3] = src.getFloat(off + 3 * strideBytes);
          out[i + 4] = src.getFloat(off + 4 * strideBytes);
          out[i + 5] = src.getFloat(off + 5 * strideBytes);
          out[i + 6] = src.getFloat(off + 6 * strideBytes);
          out[i + 7] = src.getFloat(off + 7 * strideBytes);
        }
        for (; i < length; i++, off += strideBytes)
          out[i] = src.getFloat(off);
        break;
      default:
        for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
        {
          out[i] = src.getDouble(off);
          out[i + 1] = src.getDouble(off + strideBytes);
          out[i + 2] = src.getDouble(off + 2 * strideBytes);
          out[i + 3] = src.getDouble(off + 3 * strideBytes);
          out[i + 4] = src.getDouble(off + 4 * strideBytes);
          out[i + 5] = src.getDouble(off + 5 * strideBytes);
          out[i + 6] = src.getDouble(off + 6 * strideBytes);
          out[i + 7] = src.getDouble(off + 7 * strideBytes);
        }
        for (; i < length; i++, off += strideBytes)
          out[i] = src.getDouble(off);
        break;
    }
    return out;
  }

  // ---- Cast-and-store writers -- one unrolled loop per (source-array-kind,
  // target-type) pair, each shared between the allocate-a-fresh-array path
  // (fillFlatFromIntSrc/fillFlatFromFloatSrc, called with destStart=0,
  // destStep=1 against a just-created array) and the write-into-an-existing
  // -array path (fillFlatFromIntSrcInto/fillFlatFromFloatSrcInto). Named
  // and separated out rather than left as inline case bodies repeated in
  // both callers -- same loop shape, same cast, previously duplicated once
  // per orchestrator.

  private static void writeBooleansFromLongs(long[] vals, boolean[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = vals[i] != 0;
      dest[di + destStep] = vals[i + 1] != 0;
      dest[di + 2 * destStep] = vals[i + 2] != 0;
      dest[di + 3 * destStep] = vals[i + 3] != 0;
      dest[di + 4 * destStep] = vals[i + 4] != 0;
      dest[di + 5 * destStep] = vals[i + 5] != 0;
      dest[di + 6 * destStep] = vals[i + 6] != 0;
      dest[di + 7 * destStep] = vals[i + 7] != 0;
    }
    for (; i < n; i++, di += destStep)
      dest[di] = vals[i] != 0;
  }

  private static void writeBytesFromLongs(long[] vals, byte[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (byte) vals[i];
      dest[di + destStep] = (byte) vals[i + 1];
      dest[di + 2 * destStep] = (byte) vals[i + 2];
      dest[di + 3 * destStep] = (byte) vals[i + 3];
      dest[di + 4 * destStep] = (byte) vals[i + 4];
      dest[di + 5 * destStep] = (byte) vals[i + 5];
      dest[di + 6 * destStep] = (byte) vals[i + 6];
      dest[di + 7 * destStep] = (byte) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (byte) vals[i];
  }

  private static void writeCharsFromLongs(long[] vals, char[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (char) vals[i];
      dest[di + destStep] = (char) vals[i + 1];
      dest[di + 2 * destStep] = (char) vals[i + 2];
      dest[di + 3 * destStep] = (char) vals[i + 3];
      dest[di + 4 * destStep] = (char) vals[i + 4];
      dest[di + 5 * destStep] = (char) vals[i + 5];
      dest[di + 6 * destStep] = (char) vals[i + 6];
      dest[di + 7 * destStep] = (char) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (char) vals[i];
  }

  private static void writeShortsFromLongs(long[] vals, short[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (short) vals[i];
      dest[di + destStep] = (short) vals[i + 1];
      dest[di + 2 * destStep] = (short) vals[i + 2];
      dest[di + 3 * destStep] = (short) vals[i + 3];
      dest[di + 4 * destStep] = (short) vals[i + 4];
      dest[di + 5 * destStep] = (short) vals[i + 5];
      dest[di + 6 * destStep] = (short) vals[i + 6];
      dest[di + 7 * destStep] = (short) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (short) vals[i];
  }

  private static void writeIntsFromLongs(long[] vals, int[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (int) vals[i];
      dest[di + destStep] = (int) vals[i + 1];
      dest[di + 2 * destStep] = (int) vals[i + 2];
      dest[di + 3 * destStep] = (int) vals[i + 3];
      dest[di + 4 * destStep] = (int) vals[i + 4];
      dest[di + 5 * destStep] = (int) vals[i + 5];
      dest[di + 6 * destStep] = (int) vals[i + 6];
      dest[di + 7 * destStep] = (int) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (int) vals[i];
  }

  private static void writeLongsFromLongs(long[] vals, long[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = vals[i];
      dest[di + destStep] = vals[i + 1];
      dest[di + 2 * destStep] = vals[i + 2];
      dest[di + 3 * destStep] = vals[i + 3];
      dest[di + 4 * destStep] = vals[i + 4];
      dest[di + 5 * destStep] = vals[i + 5];
      dest[di + 6 * destStep] = vals[i + 6];
      dest[di + 7 * destStep] = vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = vals[i];
  }

  private static void writeBooleansFromDoubles(double[] vals, boolean[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = vals[i] != 0;
      dest[di + destStep] = vals[i + 1] != 0;
      dest[di + 2 * destStep] = vals[i + 2] != 0;
      dest[di + 3 * destStep] = vals[i + 3] != 0;
      dest[di + 4 * destStep] = vals[i + 4] != 0;
      dest[di + 5 * destStep] = vals[i + 5] != 0;
      dest[di + 6 * destStep] = vals[i + 6] != 0;
      dest[di + 7 * destStep] = vals[i + 7] != 0;
    }
    for (; i < n; i++, di += destStep)
      dest[di] = vals[i] != 0;
  }

  private static void writeBytesFromDoubles(double[] vals, byte[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (byte) vals[i];
      dest[di + destStep] = (byte) vals[i + 1];
      dest[di + 2 * destStep] = (byte) vals[i + 2];
      dest[di + 3 * destStep] = (byte) vals[i + 3];
      dest[di + 4 * destStep] = (byte) vals[i + 4];
      dest[di + 5 * destStep] = (byte) vals[i + 5];
      dest[di + 6 * destStep] = (byte) vals[i + 6];
      dest[di + 7 * destStep] = (byte) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (byte) vals[i];
  }

  private static void writeCharsFromDoubles(double[] vals, char[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (char) vals[i];
      dest[di + destStep] = (char) vals[i + 1];
      dest[di + 2 * destStep] = (char) vals[i + 2];
      dest[di + 3 * destStep] = (char) vals[i + 3];
      dest[di + 4 * destStep] = (char) vals[i + 4];
      dest[di + 5 * destStep] = (char) vals[i + 5];
      dest[di + 6 * destStep] = (char) vals[i + 6];
      dest[di + 7 * destStep] = (char) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (char) vals[i];
  }

  private static void writeShortsFromDoubles(double[] vals, short[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (short) vals[i];
      dest[di + destStep] = (short) vals[i + 1];
      dest[di + 2 * destStep] = (short) vals[i + 2];
      dest[di + 3 * destStep] = (short) vals[i + 3];
      dest[di + 4 * destStep] = (short) vals[i + 4];
      dest[di + 5 * destStep] = (short) vals[i + 5];
      dest[di + 6 * destStep] = (short) vals[i + 6];
      dest[di + 7 * destStep] = (short) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (short) vals[i];
  }

  private static void writeIntsFromDoubles(double[] vals, int[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (int) vals[i];
      dest[di + destStep] = (int) vals[i + 1];
      dest[di + 2 * destStep] = (int) vals[i + 2];
      dest[di + 3 * destStep] = (int) vals[i + 3];
      dest[di + 4 * destStep] = (int) vals[i + 4];
      dest[di + 5 * destStep] = (int) vals[i + 5];
      dest[di + 6 * destStep] = (int) vals[i + 6];
      dest[di + 7 * destStep] = (int) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (int) vals[i];
  }

  private static void writeLongsFromDoubles(double[] vals, long[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (long) vals[i];
      dest[di + destStep] = (long) vals[i + 1];
      dest[di + 2 * destStep] = (long) vals[i + 2];
      dest[di + 3 * destStep] = (long) vals[i + 3];
      dest[di + 4 * destStep] = (long) vals[i + 4];
      dest[di + 5 * destStep] = (long) vals[i + 5];
      dest[di + 6 * destStep] = (long) vals[i + 6];
      dest[di + 7 * destStep] = (long) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (long) vals[i];
  }

  private static void writeFloatsFromDoubles(double[] vals, float[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = (float) vals[i];
      dest[di + destStep] = (float) vals[i + 1];
      dest[di + 2 * destStep] = (float) vals[i + 2];
      dest[di + 3 * destStep] = (float) vals[i + 3];
      dest[di + 4 * destStep] = (float) vals[i + 4];
      dest[di + 5 * destStep] = (float) vals[i + 5];
      dest[di + 6 * destStep] = (float) vals[i + 6];
      dest[di + 7 * destStep] = (float) vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = (float) vals[i];
  }

  private static void writeDoublesFromDoubles(double[] vals, double[] dest, int destStart, int destStep)
  {
    int n = vals.length;
    int i = 0, di = destStart;
    for (; i + 8 <= n; i += 8, di += 8 * destStep)
    {
      dest[di] = vals[i];
      dest[di + destStep] = vals[i + 1];
      dest[di + 2 * destStep] = vals[i + 2];
      dest[di + 3 * destStep] = vals[i + 3];
      dest[di + 4 * destStep] = vals[i + 4];
      dest[di + 5 * destStep] = vals[i + 5];
      dest[di + 6 * destStep] = vals[i + 6];
      dest[di + 7 * destStep] = vals[i + 7];
    }
    for (; i < n; i++, di += destStep)
      dest[di] = vals[i];
  }

  // ---- Matched-width direct readers -- used only when the source
  // element's own width already equals the target's (srcSize==4 for an
  // 'I'/'F' target), so no widen-then-narrow round trip through
  // readLongs/readDoublesFromFloat's long[]/double[] intermediate is
  // needed at all. Without these, a non-contiguous but otherwise
  // byte-identical source (e.g. a numpy int32 column slice into int[])
  // paid two full passes and a wasted length-sized long[]/double[]
  // allocation for zero actual coercion work -- see
  // fillFlatFromIntSrc/fillFlatFromFloatSrc below for where these are
  // selected. Source sign (unsignedSrc) doesn't matter for the int case:
  // narrowing a widened long back to int would discard the same high
  // bits a direct 4-byte read already omits, so both signed and
  // unsigned 4-byte sources land on this same path.

  private static void readIntsDirectUnrolled(ByteBuffer src, int[] out, int length, int strideBytes)
  {
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getInt(off);
      out[i + 1] = src.getInt(off + strideBytes);
      out[i + 2] = src.getInt(off + 2 * strideBytes);
      out[i + 3] = src.getInt(off + 3 * strideBytes);
      out[i + 4] = src.getInt(off + 4 * strideBytes);
      out[i + 5] = src.getInt(off + 5 * strideBytes);
      out[i + 6] = src.getInt(off + 6 * strideBytes);
      out[i + 7] = src.getInt(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getInt(off);
  }

  private static void readIntsDirectUnrolledInto(ByteBuffer src, int[] dest, int length, int strideBytes,
          int destStart, int destStep)
  {
    int i = 0, off = 0, di = destStart;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes, di += 8 * destStep)
    {
      dest[di] = src.getInt(off);
      dest[di + destStep] = src.getInt(off + strideBytes);
      dest[di + 2 * destStep] = src.getInt(off + 2 * strideBytes);
      dest[di + 3 * destStep] = src.getInt(off + 3 * strideBytes);
      dest[di + 4 * destStep] = src.getInt(off + 4 * strideBytes);
      dest[di + 5 * destStep] = src.getInt(off + 5 * strideBytes);
      dest[di + 6 * destStep] = src.getInt(off + 6 * strideBytes);
      dest[di + 7 * destStep] = src.getInt(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes, di += destStep)
      dest[di] = src.getInt(off);
  }

  private static void readFloatsDirectUnrolled(ByteBuffer src, float[] out, int length, int strideBytes)
  {
    int i = 0, off = 0;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes)
    {
      out[i] = src.getFloat(off);
      out[i + 1] = src.getFloat(off + strideBytes);
      out[i + 2] = src.getFloat(off + 2 * strideBytes);
      out[i + 3] = src.getFloat(off + 3 * strideBytes);
      out[i + 4] = src.getFloat(off + 4 * strideBytes);
      out[i + 5] = src.getFloat(off + 5 * strideBytes);
      out[i + 6] = src.getFloat(off + 6 * strideBytes);
      out[i + 7] = src.getFloat(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes)
      out[i] = src.getFloat(off);
  }

  private static void readFloatsDirectUnrolledInto(ByteBuffer src, float[] dest, int length, int strideBytes,
          int destStart, int destStep)
  {
    int i = 0, off = 0, di = destStart;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes, di += 8 * destStep)
    {
      dest[di] = src.getFloat(off);
      dest[di + destStep] = src.getFloat(off + strideBytes);
      dest[di + 2 * destStep] = src.getFloat(off + 2 * strideBytes);
      dest[di + 3 * destStep] = src.getFloat(off + 3 * strideBytes);
      dest[di + 4 * destStep] = src.getFloat(off + 4 * strideBytes);
      dest[di + 5 * destStep] = src.getFloat(off + 5 * strideBytes);
      dest[di + 6 * destStep] = src.getFloat(off + 6 * strideBytes);
      dest[di + 7 * destStep] = src.getFloat(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes, di += destStep)
      dest[di] = src.getFloat(off);
  }

  // 'J'/'D' matched-width (srcSize==8) have no non-Into counterpart here:
  // readLongsSignedUnrolled/readDoublesFromFloat's srcSize==8 branch
  // already read straight into the array that becomes the return value
  // for fillFlatFromIntSrc/fillFlatFromFloatSrc (case 'J'/'D' return that
  // array as-is, no narrowing pass) -- nothing to shortcut there. The
  // Into variants below are still worth it: fillFlatFromIntSrcInto/
  // fillFlatFromFloatSrcInto's generic path allocates that same array
  // and then copies it into dest with writeLongsFromLongs/
  // writeDoublesFromDoubles, a wasted second pass an Into call can skip
  // entirely by reading straight into dest.

  private static void readLongsDirectUnrolledInto(ByteBuffer src, long[] dest, int length, int strideBytes,
          int destStart, int destStep)
  {
    int i = 0, off = 0, di = destStart;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes, di += 8 * destStep)
    {
      dest[di] = src.getLong(off);
      dest[di + destStep] = src.getLong(off + strideBytes);
      dest[di + 2 * destStep] = src.getLong(off + 2 * strideBytes);
      dest[di + 3 * destStep] = src.getLong(off + 3 * strideBytes);
      dest[di + 4 * destStep] = src.getLong(off + 4 * strideBytes);
      dest[di + 5 * destStep] = src.getLong(off + 5 * strideBytes);
      dest[di + 6 * destStep] = src.getLong(off + 6 * strideBytes);
      dest[di + 7 * destStep] = src.getLong(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes, di += destStep)
      dest[di] = src.getLong(off);
  }

  private static void readDoublesDirectUnrolledInto(ByteBuffer src, double[] dest, int length, int strideBytes,
          int destStart, int destStep)
  {
    int i = 0, off = 0, di = destStart;
    for (; i + 8 <= length; i += 8, off += 8 * strideBytes, di += 8 * destStep)
    {
      dest[di] = src.getDouble(off);
      dest[di + destStep] = src.getDouble(off + strideBytes);
      dest[di + 2 * destStep] = src.getDouble(off + 2 * strideBytes);
      dest[di + 3 * destStep] = src.getDouble(off + 3 * strideBytes);
      dest[di + 4 * destStep] = src.getDouble(off + 4 * strideBytes);
      dest[di + 5 * destStep] = src.getDouble(off + 5 * strideBytes);
      dest[di + 6 * destStep] = src.getDouble(off + 6 * strideBytes);
      dest[di + 7 * destStep] = src.getDouble(off + 7 * strideBytes);
    }
    for (; i < length; i++, off += strideBytes, di += destStep)
      dest[di] = src.getDouble(off);
  }

  private static Object fillFlatFromIntSrc(char typeCode, boolean unsignedSrc, int srcSize,
          ByteBuffer src, int length, int strideBytes)
  {
    if (typeCode == 'F' || typeCode == 'D')
    {
      double[] vals = readDoublesFromInt(src, length, strideBytes, unsignedSrc, srcSize);
      if (typeCode == 'D')
        return vals;
      float[] out = new float[length];
      writeFloatsFromDoubles(vals, out, 0, 1);
      return out;
    }

    // Matched-width fast path -- see the direct-reader block above.
    if (typeCode == 'I' && srcSize == 4)
    {
      int[] out = new int[length];
      readIntsDirectUnrolled(src, out, length, strideBytes);
      return out;
    }

    long[] vals = readLongs(src, length, strideBytes, unsignedSrc, srcSize);
    switch (typeCode)
    {
      case 'Z':
      {
        boolean[] out = new boolean[length];
        writeBooleansFromLongs(vals, out, 0, 1);
        return out;
      }
      case 'B':
      {
        byte[] out = new byte[length];
        writeBytesFromLongs(vals, out, 0, 1);
        return out;
      }
      case 'C':
      {
        char[] out = new char[length];
        writeCharsFromLongs(vals, out, 0, 1);
        return out;
      }
      case 'S':
      {
        short[] out = new short[length];
        writeShortsFromLongs(vals, out, 0, 1);
        return out;
      }
      case 'I':
      {
        int[] out = new int[length];
        writeIntsFromLongs(vals, out, 0, 1);
        return out;
      }
      case 'J':
        return vals;
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }
  }

  private static Object fillFlatFromFloatSrc(char typeCode, int srcSize,
          ByteBuffer src, int length, int strideBytes)
  {
    // Matched-width fast path -- see the direct-reader block above
    // fillFlatFromIntSrc.
    if (typeCode == 'F' && srcSize == 4)
    {
      float[] out = new float[length];
      readFloatsDirectUnrolled(src, out, length, strideBytes);
      return out;
    }

    double[] vals = readDoublesFromFloat(src, length, strideBytes, srcSize);
    switch (typeCode)
    {
      case 'Z':
      {
        boolean[] out = new boolean[length];
        writeBooleansFromDoubles(vals, out, 0, 1);
        return out;
      }
      case 'B':
      {
        byte[] out = new byte[length];
        writeBytesFromDoubles(vals, out, 0, 1);
        return out;
      }
      case 'C':
      {
        char[] out = new char[length];
        writeCharsFromDoubles(vals, out, 0, 1);
        return out;
      }
      case 'S':
      {
        short[] out = new short[length];
        writeShortsFromDoubles(vals, out, 0, 1);
        return out;
      }
      case 'I':
      {
        int[] out = new int[length];
        writeIntsFromDoubles(vals, out, 0, 1);
        return out;
      }
      case 'J':
      {
        long[] out = new long[length];
        writeLongsFromDoubles(vals, out, 0, 1);
        return out;
      }
      case 'F':
      {
        float[] out = new float[length];
        writeFloatsFromDoubles(vals, out, 0, 1);
        return out;
      }
      case 'D':
        return vals;
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }
  }

  /**
   * Write-into sibling of {@link #fillFlatFromBuffer} -- the fast path for
   * {@code JPArray::setRange} (Python slice assignment, `javaArr[:] =
   * numpy_array`) and `JPArray::clone`, neither of which allocates a fresh
   * array the way an argument-conversion push does: both write into a
   * specific range of an *existing* Java array, generally with its own
   * destination start/step (a sliced/strided Java-array target). Writing
   * directly into a statically-typed Java array here (`(int[]) dest` etc.)
   * needs no JNI critical section at all -- it's the same plain array
   * store the JIT would emit for any other Java code touching that array,
   * unlike the C++ side's previous approach (`Get<Type>ArrayElements`,
   * which the JNI spec permits to copy the whole array on entry and exit).
   *
   * @param dest the destination array, already cast to its true
   * component type by the C++ caller's `typeCode` (an `int[]` for `'I'`,
   * etc.) -- passed as `Object` since one native method covers all 8
   * primitive types rather than 8 overloads.
   * @param destStart the first destination index to write.
   * @param destStep the destination stride in *elements* (not bytes,
   * unlike `strideBytes` which describes the source) -- may be negative
   * for a reversed destination slice; always safe as plain array indexing,
   * with no address-arithmetic bounds concern the way a negative source
   * stride would have against raw buffer memory.
   */
  public static void fillFlatIntoArray(char typeCode, char srcKind, int srcSize, boolean swapped,
          ByteBuffer src, int length, int strideBytes, Object dest, int destStart, int destStep)
  {
    src.order(swapped ? swapped(ByteOrder.nativeOrder()) : ByteOrder.nativeOrder());

    if (strideBytes == srcSize && destStep == 1)
    {
      switch (typeCode)
      {
        case 'I':
          if (srcKind == SRC_SIGNED && srcSize == 4)
          {
            src.asIntBuffer().get((int[]) dest, destStart, length);
            return;
          }
          break;
        case 'J':
          if (srcKind == SRC_SIGNED && srcSize == 8)
          {
            src.asLongBuffer().get((long[]) dest, destStart, length);
            return;
          }
          break;
        case 'F':
          if (srcKind == SRC_FLOAT && srcSize == 4)
          {
            src.asFloatBuffer().get((float[]) dest, destStart, length);
            return;
          }
          break;
        case 'D':
          if (srcKind == SRC_FLOAT && srcSize == 8)
          {
            src.asDoubleBuffer().get((double[]) dest, destStart, length);
            return;
          }
          break;
        default:
          break;
      }
    }

    if (srcKind == SRC_FLOAT)
      fillFlatFromFloatSrcInto(typeCode, srcSize, src, length, strideBytes, dest, destStart, destStep);
    else
      fillFlatFromIntSrcInto(typeCode, srcKind == SRC_UNSIGNED, srcSize, src, length, strideBytes,
              dest, destStart, destStep);
  }

  private static void fillFlatFromIntSrcInto(char typeCode, boolean unsignedSrc, int srcSize,
          ByteBuffer src, int length, int strideBytes, Object dest, int destStart, int destStep)
  {
    if (typeCode == 'F' || typeCode == 'D')
    {
      double[] vals = readDoublesFromInt(src, length, strideBytes, unsignedSrc, srcSize);
      if (typeCode == 'D')
        writeDoublesFromDoubles(vals, (double[]) dest, destStart, destStep);
      else
        writeFloatsFromDoubles(vals, (float[]) dest, destStart, destStep);
      return;
    }

    // Matched-width fast path -- see the direct-reader block above
    // fillFlatFromIntSrc.
    if (typeCode == 'I' && srcSize == 4)
    {
      readIntsDirectUnrolledInto(src, (int[]) dest, length, strideBytes, destStart, destStep);
      return;
    }

    // Matched-width fast path -- see the direct-reader block above
    // fillFlatFromIntSrc.
    if (typeCode == 'J' && srcSize == 8)
    {
      readLongsDirectUnrolledInto(src, (long[]) dest, length, strideBytes, destStart, destStep);
      return;
    }

    long[] vals = readLongs(src, length, strideBytes, unsignedSrc, srcSize);
    switch (typeCode)
    {
      case 'Z':
        writeBooleansFromLongs(vals, (boolean[]) dest, destStart, destStep);
        return;
      case 'B':
        writeBytesFromLongs(vals, (byte[]) dest, destStart, destStep);
        return;
      case 'C':
        writeCharsFromLongs(vals, (char[]) dest, destStart, destStep);
        return;
      case 'S':
        writeShortsFromLongs(vals, (short[]) dest, destStart, destStep);
        return;
      case 'I':
        writeIntsFromLongs(vals, (int[]) dest, destStart, destStep);
        return;
      case 'J':
        writeLongsFromLongs(vals, (long[]) dest, destStart, destStep);
        return;
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }
  }

  private static void fillFlatFromFloatSrcInto(char typeCode, int srcSize,
          ByteBuffer src, int length, int strideBytes, Object dest, int destStart, int destStep)
  {
    // Matched-width fast path -- see the direct-reader block above
    // fillFlatFromIntSrc.
    if (typeCode == 'F' && srcSize == 4)
    {
      readFloatsDirectUnrolledInto(src, (float[]) dest, length, strideBytes, destStart, destStep);
      return;
    }

    // Matched-width fast path -- see the direct-reader block above
    // fillFlatFromIntSrc.
    if (typeCode == 'D' && srcSize == 8)
    {
      readDoublesDirectUnrolledInto(src, (double[]) dest, length, strideBytes, destStart, destStep);
      return;
    }

    double[] vals = readDoublesFromFloat(src, length, strideBytes, srcSize);
    switch (typeCode)
    {
      case 'Z':
        writeBooleansFromDoubles(vals, (boolean[]) dest, destStart, destStep);
        return;
      case 'B':
        writeBytesFromDoubles(vals, (byte[]) dest, destStart, destStep);
        return;
      case 'C':
        writeCharsFromDoubles(vals, (char[]) dest, destStart, destStep);
        return;
      case 'S':
        writeShortsFromDoubles(vals, (short[]) dest, destStart, destStep);
        return;
      case 'I':
        writeIntsFromDoubles(vals, (int[]) dest, destStart, destStep);
        return;
      case 'J':
        writeLongsFromDoubles(vals, (long[]) dest, destStart, destStep);
        return;
      case 'F':
        writeFloatsFromDoubles(vals, (float[]) dest, destStart, destStep);
        return;
      case 'D':
        writeDoublesFromDoubles(vals, (double[]) dest, destStart, destStep);
        return;
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }
  }

  /**
   * Bulk-write a rectangular multi-dimensional primitive array's contents
   * into a caller-supplied direct buffer -- the pull-side half. `dest`'s
   * capacity must already match the total element count implied by
   * `collected`'s shape entry.
   *
   * @param typeCode primitive type signature character.
   * @param collected the result of {@link #collectRectangular}: [0] =
   * leaf component Class (unused here, typeCode is passed separately by
   * the C++ caller instead), [1] = int[] shape, [2..] = leaf arrays in
   * row-major order.
   * @param dest a direct, writable buffer of the right total byte
   * capacity.
   */
  public static void collectToBuffer(char typeCode, Object[] collected, ByteBuffer dest)
  {
    dest.order(ByteOrder.nativeOrder());
    int[] shape = (int[]) collected[1];
    int last = shape[shape.length - 1];
    int leaves = collected.length - 2;

    IntStream range = leafRange(leaves, last);

    switch (typeCode)
    {
      case 'I':
      {
        IntBuffer buf = dest.asIntBuffer();
        range.forEach(i ->
        {
          IntBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((int[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'D':
      {
        DoubleBuffer buf = dest.asDoubleBuffer();
        range.forEach(i ->
        {
          DoubleBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((double[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'J':
      {
        LongBuffer buf = dest.asLongBuffer();
        range.forEach(i ->
        {
          LongBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((long[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'F':
      {
        FloatBuffer buf = dest.asFloatBuffer();
        range.forEach(i ->
        {
          FloatBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((float[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'S':
      {
        ShortBuffer buf = dest.asShortBuffer();
        range.forEach(i ->
        {
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((short[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'C':
      {
        CharBuffer buf = dest.asCharBuffer();
        range.forEach(i ->
        {
          CharBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((char[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'B':
      {
        range.forEach(i ->
        {
          ByteBuffer dup = dest.duplicate();
          dup.position(i * last);
          dup.put((byte[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'Z':
      {
        range.forEach(i ->
        {
          boolean[] row = (boolean[]) collected[i + 2];
          ByteBuffer dup = dest.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            dup.put(row[j] ? (byte) 1 : (byte) 0);
        });
        break;
      }
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }
  }

  /**
   * Bulk-write a caller-supplied direct buffer's contents into a
   * rectangular multi-dimensional primitive array's *existing* leaf
   * arrays, in place -- the push-side mirror of {@link #collectToBuffer}
   * above, for {@code JArray.pushFrom}'s N-D case. Unlike {@link
   * #fillFromBuffer}, this never allocates a new array: {@code collected}
   * (from {@link #collectRectangular}) already holds references to the
   * target array's own leaf arrays, and this only overwrites their
   * contents, preserving the target array's identity exactly as
   * {@code pushFrom}'s existing flat/1D contract requires. `src`'s
   * capacity must already match the total element count implied by
   * `collected`'s shape entry.
   *
   * @param typeCode primitive type signature character.
   * @param collected the result of {@link #collectRectangular}: [0] =
   * leaf component Class (unused here, typeCode is passed separately by
   * the C++ caller instead), [1] = int[] shape, [2..] = leaf arrays in
   * row-major order.
   * @param src a direct, readable buffer of the right total byte
   * capacity.
   */
  public static void fillFromBufferIntoRectangular(char typeCode, Object[] collected, ByteBuffer src)
  {
    src.order(ByteOrder.nativeOrder());
    int[] shape = (int[]) collected[1];
    int last = shape[shape.length - 1];
    int leaves = collected.length - 2;

    IntStream range = leafRange(leaves, last);

    switch (typeCode)
    {
      case 'I':
      {
        IntBuffer buf = src.asIntBuffer();
        range.forEach(i ->
        {
          IntBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get((int[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'D':
      {
        DoubleBuffer buf = src.asDoubleBuffer();
        range.forEach(i ->
        {
          DoubleBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get((double[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'J':
      {
        LongBuffer buf = src.asLongBuffer();
        range.forEach(i ->
        {
          LongBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get((long[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'F':
      {
        FloatBuffer buf = src.asFloatBuffer();
        range.forEach(i ->
        {
          FloatBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get((float[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'S':
      {
        ShortBuffer buf = src.asShortBuffer();
        range.forEach(i ->
        {
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get((short[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'C':
      {
        CharBuffer buf = src.asCharBuffer();
        range.forEach(i ->
        {
          CharBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get((char[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'B':
      {
        range.forEach(i ->
        {
          ByteBuffer dup = src.duplicate();
          dup.position(i * last);
          dup.get((byte[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'Z':
      {
        range.forEach(i ->
        {
          boolean[] row = (boolean[]) collected[i + 2];
          ByteBuffer dup = src.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = dup.get() != 0;
        });
        break;
      }
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }
  }

  // ---- Ragged-native nested-list push.
  //
  // Counterpart to fillFromBuffer above, but for a *ragged* nested Python
  // list of int/long/float/double rather than a rectangular buffer-
  // protocol source -- there is no shape[] to drive the reshape, since a
  // ragged tree's shape isn't uniform per dimension. Instead the C++ side
  // (JPConversionRaggedSequence::convert, jp_classhints.cpp) writes one
  // int32 length marker per node, depth-first pre-order, uniformly at
  // every level including the outermost, followed by raw leaf values --
  // so this side reads the same shape back one marker at a time instead
  // of consulting a precomputed array. Still one JNI crossing total, then
  // pure Java (no further JNI/reflection round trips per node beyond the
  // Array.newInstance/Array.set already needed to build the result).

  /**
   * Reconstruct a ragged multi-dimensional primitive array from the
   * depth-first pre-order buffer JPConversionRaggedSequence::convert
   * wrote.
   *
   * @param typeCode primitive type signature character -- every
   * primitive type code (see isRaggedEligible in jp_classhints.cpp).
   * @param dims the array's static nesting depth (e.g. 3 for int[][][]),
   * known up front from the target class, not discovered from the data.
   * @param src a direct buffer positioned at the start of the encoded
   * tree.
   * @return the assembled array (e.g. int[][][] for dims == 3).
   */
  public static Object fillRaggedFromBuffer(char typeCode, int dims, ByteBuffer src)
  {
    src.order(ByteOrder.nativeOrder());
    return readRaggedNode(typeCode, dims, src, raggedArrayClassesByDepth(typeCode, dims));
  }

  private static Class<?> raggedLeafClass(char typeCode)
  {
    switch (typeCode)
    {
      case 'I':
        return int.class;
      case 'J':
        return long.class;
      case 'F':
        return float.class;
      case 'D':
        return double.class;
      case 'Z':
        return boolean.class;
      case 'B':
        return byte.class;
      case 'C':
        return char.class;
      case 'S':
        return short.class;
      default:
        throw new IllegalArgumentException("Unsupported ragged leaf type code: " + typeCode);
    }
  }

  /**
   * Element width on the wire for a ragged leaf run, in bytes -- mirrors
   * raggedItemSize in jp_classhints.cpp exactly (must match, since it
   * determines both sides' agreement on where the 4-byte padding after a
   * leaf run ends).
   */
  private static int raggedItemSize(char typeCode)
  {
    switch (typeCode)
    {
      case 'Z':
      case 'B':
        return Byte.BYTES;
      case 'C':
      case 'S':
        return Short.BYTES;
      case 'I':
      case 'F':
        return Integer.BYTES;
      default: // 'J'/'D'
        return Long.BYTES;
    }
  }

  /**
   * Skips the zero-padding JPConversionRaggedSequence::convert
   * (jp_classhints.cpp) inserted after a leaf run to restore 4-byte
   * alignment for the next length marker -- see raggedAlign4 there.
   * No-op for I/J/F/D, whose runs are always already a multiple of 4.
   */
  private static void skipRaggedPadding(ByteBuffer src, char typeCode, int n)
  {
    int rawBytes = n * raggedItemSize(typeCode);
    int pad = (4 - (rawBytes & 3)) & 3;
    if (pad != 0)
      src.position(src.position() + pad);
  }

  /**
   * {@code classesByDepth[d]} is the Class of a {@code d}-dimensional
   * array of typeCode's primitive ({@code classesByDepth[0]} is the
   * primitive Class itself, e.g. {@code int.class}; {@code
   * classesByDepth[1]} is {@code int[].class}, etc.) -- used by {@link
   * #readRaggedNode} to build each container level via {@code
   * Array.newInstance(componentClass, n)}, not read off an
   * already-materialized child (which would break on a legitimately
   * empty node, n == 0).
   *
   * Computed once per call to {@link #fillRaggedFromBuffer}, not once
   * per node: the naive version of this (recomputing the Class for the
   * current depth fresh at every node via its own {@code
   * Array.newInstance(c, 0).getClass()} loop) paid a throwaway
   * reflective array allocation per node per remaining depth level --
   * thousands of redundant allocations at depth 5 with ~100,000 leaf
   * elements, for a value that depends only on (typeCode, depth) and is
   * identical across every node at that depth in the same call.
   */
  private static Class<?>[] raggedArrayClassesByDepth(char typeCode, int dims)
  {
    Class<?>[] classesByDepth = new Class<?>[dims];
    Class<?> c = raggedLeafClass(typeCode);
    for (int d = 0; d < dims; d++)
    {
      classesByDepth[d] = c;
      c = Array.newInstance(c, 0).getClass();
    }
    return classesByDepth;
  }

  private static Object readRaggedNode(char typeCode, int remainingDepth, ByteBuffer src,
          Class<?>[] classesByDepth)
  {
    int n = src.getInt();
    if (remainingDepth == 1)
      return readRaggedLeaf(typeCode, n, src);
    Object arr = Array.newInstance(classesByDepth[remainingDepth - 1], n);
    for (int i = 0; i < n; i++)
      Array.set(arr, i, readRaggedNode(typeCode, remainingDepth - 1, src, classesByDepth));
    return arr;
  }

  private static Object readRaggedLeaf(char typeCode, int n, ByteBuffer src)
  {
    switch (typeCode)
    {
      case 'I':
      {
        int[] row = new int[n];
        src.asIntBuffer().get(row, 0, n);
        src.position(src.position() + n * Integer.BYTES);
        return row;
      }
      case 'J':
      {
        long[] row = new long[n];
        src.asLongBuffer().get(row, 0, n);
        src.position(src.position() + n * Long.BYTES);
        return row;
      }
      case 'F':
      {
        float[] row = new float[n];
        src.asFloatBuffer().get(row, 0, n);
        src.position(src.position() + n * Float.BYTES);
        return row;
      }
      case 'D':
      {
        double[] row = new double[n];
        src.asDoubleBuffer().get(row, 0, n);
        src.position(src.position() + n * Double.BYTES);
        return row;
      }
      case 'Z':
      {
        boolean[] row = new boolean[n];
        for (int i = 0; i < n; i++)
          row[i] = src.get() != 0;
        skipRaggedPadding(src, typeCode, n);
        return row;
      }
      case 'B':
      {
        byte[] row = new byte[n];
        src.get(row, 0, n);
        skipRaggedPadding(src, typeCode, n);
        return row;
      }
      case 'C':
      {
        char[] row = new char[n];
        src.asCharBuffer().get(row, 0, n);
        src.position(src.position() + n * Character.BYTES);
        skipRaggedPadding(src, typeCode, n);
        return row;
      }
      case 'S':
      {
        short[] row = new short[n];
        src.asShortBuffer().get(row, 0, n);
        src.position(src.position() + n * Short.BYTES);
        skipRaggedPadding(src, typeCode, n);
        return row;
      }
      default:
        throw new IllegalArgumentException("Unsupported ragged leaf type code: " + typeCode);
    }
  }

  // --- Ported from the old JPypeContext singleton -- these have no
  // array-transfer overlap with the methods above, just relocated here
  // as part of retiring JPypeContext's per-process singleton in favor of
  // NativeContext's per-context instance. ---

  @Exported
  public static long getExcClass(Throwable th)
  {
    if (th instanceof PyExceptionProxy)
      return ((PyExceptionProxy) th).cls;
    return 0;
  }

  @Exported
  public static long getExcValue(Throwable th)
  {
    if (th instanceof PyExceptionProxy)
      return ((PyExceptionProxy) th).value;
    return 0;
  }

  @Exported
  public static Exception createException(long l0, long l1)
  {
    return new PyExceptionProxy(l0, l1);
  }

  @Exported
  public static boolean order(Buffer b)
  {
    if (b instanceof java.nio.ByteBuffer)
      return ((java.nio.ByteBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.ShortBuffer)
      return ((java.nio.ShortBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.CharBuffer)
      return ((java.nio.CharBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.IntBuffer)
      return ((java.nio.IntBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.LongBuffer)
      return ((java.nio.LongBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.FloatBuffer)
      return ((java.nio.FloatBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.DoubleBuffer)
      return ((java.nio.DoubleBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    return true;
  }

  @Exported
  public static Object[] getStackTrace(Throwable th, Throwable enclosing)
  {
    StackTraceElement[] trace = th.getStackTrace();
    if (trace == null || enclosing == null)
      return toFrames(trace);
    StackTraceElement[] te = enclosing.getStackTrace();
    if (te == null)
      return toFrames(trace);
    for (int i = 0; i < trace.length; ++i)
    {
      if (trace[i].equals(te[0]))
      {
        return toFrames(Arrays.copyOfRange(trace, 0, i));
      }
    }
    return toFrames(trace);
  }

  private static Object[] toFrames(StackTraceElement[] stackTrace)
  {
    if (stackTrace == null)
      return null;
    Object[] out = new Object[4 * stackTrace.length];
    int i = 0;
    for (StackTraceElement fr : stackTrace)
    {
      out[i++] = fr.getClassName();
      out[i++] = fr.getMethodName();
      out[i++] = fr.getFileName();
      out[i++] = fr.getLineNumber();
    }
    return out;
  }

  @Exported
  public static long getTotalMemory()
  {
    return Runtime.getRuntime().totalMemory();
  }

  @Exported
  public static long getFreeMemory()
  {
    return Runtime.getRuntime().freeMemory();
  }

  @Exported
  public static long getMaxMemory()
  {
    return Runtime.getRuntime().maxMemory();
  }

  @Exported
  public static long getUsedMemory()
  {
    return Runtime.getRuntime().totalMemory() - Runtime.getRuntime().freeMemory();
  }

  @Exported
  public static long getHeapMemory()
  {
    java.lang.management.MemoryMXBean memoryBean = java.lang.management.ManagementFactory.getMemoryMXBean();
    return memoryBean.getHeapMemoryUsage().getUsed();
  }

}
