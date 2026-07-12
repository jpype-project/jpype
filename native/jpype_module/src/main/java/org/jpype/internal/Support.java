// --- file: org/jpype/internal/Support.java ---
package org.jpype.internal;

import java.net.URISyntaxException;
import java.nio.file.Path;
import java.lang.reflect.Array;
import java.nio.Buffer;
import java.nio.ByteOrder;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.jpype.annotation.Exported;

/** 
 * Utiltiies to support the C++ classes accessed exclusively through JNI.
 * 
 * @author nelson85
 */
@SuppressWarnings("unchecked")
class Support
{

  private Support()
  {
  }

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
   * Helper function for collect rectangular array.
   */
  @Exported
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
   * @param o
   * @return
   */
  @Exported
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

  @Exported
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
      // Instantiate cleanly at the start of every segment pass
      a2 = Array.newInstance(c, size);

      for (int j = 0; j < size; j++, k++)
      {
        Object o = Array.get(parts, k);
        Array.set(a2, j, o);
      }

      // a1 safely stores a unique, freshly populated array instance
      Array.set(a1, i, a2);
    }
    return a1;
  }

  @Exported
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

  @Exported
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
