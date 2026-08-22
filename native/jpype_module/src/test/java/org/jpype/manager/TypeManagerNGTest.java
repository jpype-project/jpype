// --- file: org/jpype/manager/TypeManagerNGTest.java ---
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
package org.jpype.manager;

import org.jpype.manager.TypeFactoryHarness.DeletedResource;
import org.jpype.manager.TypeFactoryHarness.Resource;
import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertNotNull;
import static org.testng.Assert.assertNull;
import static org.testng.Assert.assertTrue;
import static org.testng.Assert.fail;

/**
 * Exercises {@code TypeManager}'s class-creation/reflection bookkeeping
 * entirely off the native bridge, using {@link TypeFactoryHarness} - a
 * fake {@code TypeFactory} built for exactly this purpose (see its own
 * Javadoc) but, until now, only wired up to a manual {@code main()}
 * driver ({@link TestTypeManager}) that never ran as part of the
 * automated suite. {@code new TypeManager(null, ...)} is TypeManager's
 * own documented unit-testing constructor path (see its Javadoc: "This is
 * used in unittesting, but it avoids all methods that call context").
 */
public class TypeManagerNGTest
{

  public interface MyFunction
  {

    Object apply(Object o);
  }

  public static class MyBase
  {

    void run()
    {
    }
  }

  @Test
  public void testInitCreateFindShutdownNoLeaks()
  {
    TypeManager tm = new TypeManager(null, null);
    TypeFactoryHarness tf = new TypeFactoryHarness(tm);
    tm.typeFactory = tf;

    assertFalse(tm.isStarted);
    tm.init();
    assertTrue(tm.isStarted);
    assertFalse(tm.isShutdown);

    // Multi-dimensional primitive array.
    assertTrue(tm.findClass(int[][][].class) != 0);

    // A lambda (synthetic functional-interface implementation).
    MyFunction f = (Object o) -> o;
    assertTrue(tm.findClass(f.getClass()) != 0);

    // An anonymous class implementing an interface.
    MyFunction f2 = new MyFunction()
    {
      @Override
      public Object apply(Object t)
      {
        return t;
      }
    };
    assertTrue(tm.findClass(f2.getClass()) != 0);

    // An anonymous class extending a concrete base.
    MyBase f3 = new MyBase()
    {
      void run()
      {
      }
    };
    assertTrue(tm.findClass(f3.getClass()) != 0);

    tm.shutdown();
    assertTrue(tm.isShutdown);

    int leaked = 0;
    for (Resource entry : tf.resourceMap.values())
    {
      if (entry instanceof DeletedResource)
        continue;
      leaked++;
    }
    assertEquals(leaked, 0, "TypeManager leaked resources on shutdown");
  }

  @Test
  public void testFindClassCachesOnSecondCall()
  {
    TypeManager tm = new TypeManager(null, null);
    TypeFactoryHarness tf = new TypeFactoryHarness(tm);
    tm.typeFactory = tf;
    tm.init();

    long first = tm.findClass(String.class);
    long second = tm.findClass(String.class);

    assertEquals(second, first);
  }

  @Test
  public void testInitTwiceThrows()
  {
    TypeManager tm = new TypeManager(null, null);
    TypeFactoryHarness tf = new TypeFactoryHarness(tm);
    tm.typeFactory = tf;
    tm.init();

    try
    {
      tm.init();
      fail("Expected RuntimeException on double init");
    } catch (RuntimeException expected)
    {
      assertTrue(expected.getMessage().contains("Cannot be restarted"));
    }
  }

  // --- lookupByName: pure reflection, no typeFactory/context needed ------
  @Test
  public void testLookupByNameDirectClass()
  {
    TypeManager tm = new TypeManager(null, null);
    assertEquals(tm.lookupByName("java.lang.String"), String.class);
  }

  @Test
  public void testLookupByNameArrayDims()
  {
    TypeManager tm = new TypeManager(null, null);
    assertEquals(tm.lookupByName("int[]"), int[].class);
    assertEquals(tm.lookupByName("int[][][]"), int[][][].class);
    assertEquals(tm.lookupByName("java.lang.String[]"), String[].class);
  }

  @Test
  public void testLookupByNameJniStyleSlashes()
  {
    TypeManager tm = new TypeManager(null, null);
    assertEquals(tm.lookupByName("java/lang/String"), String.class);
  }

  @Test
  public void testLookupByNamePrimitives()
  {
    TypeManager tm = new TypeManager(null, null);
    assertEquals(tm.lookupByName("boolean"), Boolean.TYPE);
    assertEquals(tm.lookupByName("byte"), Byte.TYPE);
    assertEquals(tm.lookupByName("char"), Character.TYPE);
    assertEquals(tm.lookupByName("short"), Short.TYPE);
    assertEquals(tm.lookupByName("long"), Long.TYPE);
    assertEquals(tm.lookupByName("int"), Integer.TYPE);
    assertEquals(tm.lookupByName("float"), Float.TYPE);
    assertEquals(tm.lookupByName("double"), Double.TYPE);
  }

  @Test
  public void testLookupByNameInnerClassDotNotation()
  {
    TypeManager tm = new TypeManager(null, null);
    // Class.forName("org.jpype.manager.TypeManagerNGTest.MyBase") fails
    // directly (dots, not the real binary $ name) - lookupByName must
    // fall back to probing "$"-joined suffixes.
    assertEquals(tm.lookupByName("org.jpype.manager.TypeManagerNGTest.MyBase"), MyBase.class);
  }

  @Test
  public void testLookupByNameNotFoundReturnsNull()
  {
    TypeManager tm = new TypeManager(null, null);
    assertNull(tm.lookupByName("no.such.package.NoSuchClass"));
  }

  /**
   * Drives {@code TypeManager.Destroyer}'s block-overflow paths
   * ({@code add(long)}'s {@code index == BLOCK_SIZE} flush and
   * {@code add(long[])}'s proactive
   * {@code index + v.length > BLOCK_SIZE} flush, {@code BLOCK_SIZE ==
   * 1024}) deterministically. These only fire once enough distinct
   * native resources (class pointers, method/field/constructor dispatch
   * arrays) accumulate in one {@code shutdown()} call - wrapping a wide
   * enough swath of the JDK's own class library is a reliable way to get
   * there without fabricating fake resource ids (the harness's
   * {@code destroy()} validates every id it's asked to destroy really
   * exists).
   */
  @Test
  public void testShutdownFlushesDestroyerQueueOverflow()
  {
    Class<?>[] classes =
    {
      Object.class, String.class, StringBuilder.class, StringBuffer.class,
      Integer.class, Long.class, Short.class, Byte.class, Character.class,
      Boolean.class, Float.class, Double.class, Number.class, Math.class,
      System.class, Thread.class, ThreadGroup.class, Runtime.class,
      Class.class, ClassLoader.class, Throwable.class, Exception.class,
      RuntimeException.class, Error.class, Enum.class, Comparable.class,
      Iterable.class, CharSequence.class, Runnable.class, Cloneable.class,
      java.util.ArrayList.class, java.util.LinkedList.class,
      java.util.HashMap.class, java.util.TreeMap.class,
      java.util.LinkedHashMap.class, java.util.HashSet.class,
      java.util.TreeSet.class, java.util.LinkedHashSet.class,
      java.util.Vector.class, java.util.Stack.class,
      java.util.ArrayDeque.class, java.util.PriorityQueue.class,
      java.util.Collections.class, java.util.Arrays.class,
      java.util.Objects.class, java.util.Optional.class,
      java.util.Iterator.class, java.util.ListIterator.class,
      java.util.Map.class, java.util.Map.Entry.class,
      java.util.List.class, java.util.Set.class, java.util.Collection.class,
      java.util.Queue.class, java.util.Deque.class, java.util.Comparator.class,
      java.util.Random.class, java.util.Scanner.class, java.util.UUID.class,
      java.util.Date.class, java.util.Calendar.class, java.util.Locale.class,
      java.util.Properties.class, java.util.StringTokenizer.class,
      java.util.BitSet.class, java.util.EnumMap.class, java.util.EnumSet.class,
      java.util.concurrent.atomic.AtomicInteger.class,
      java.util.concurrent.atomic.AtomicLong.class,
      java.util.concurrent.atomic.AtomicBoolean.class,
      java.util.concurrent.atomic.AtomicReference.class,
      java.util.concurrent.ConcurrentHashMap.class,
      java.util.concurrent.CopyOnWriteArrayList.class,
      java.util.concurrent.CountDownLatch.class,
      java.util.concurrent.Semaphore.class,
      java.util.concurrent.locks.ReentrantLock.class,
      java.util.concurrent.TimeUnit.class,
      java.util.concurrent.Executors.class,
      java.util.concurrent.ExecutorService.class,
      java.util.concurrent.Future.class,
      java.util.concurrent.Callable.class,
      java.util.function.Function.class, java.util.function.Supplier.class,
      java.util.function.Consumer.class, java.util.function.Predicate.class,
      java.util.function.BiFunction.class,
      java.util.stream.Stream.class, java.util.stream.Collectors.class,
      java.util.stream.IntStream.class,
      java.io.File.class, java.io.InputStream.class, java.io.OutputStream.class,
      java.io.Reader.class, java.io.Writer.class,
      java.io.ByteArrayInputStream.class, java.io.ByteArrayOutputStream.class,
      java.io.BufferedReader.class, java.io.BufferedWriter.class,
      java.io.PrintStream.class, java.io.PrintWriter.class,
      java.io.Serializable.class, java.io.IOException.class,
      java.nio.ByteBuffer.class, java.nio.CharBuffer.class,
      java.nio.file.Path.class, java.nio.file.Paths.class,
      java.nio.file.Files.class,
      java.lang.reflect.Method.class, java.lang.reflect.Field.class,
      java.lang.reflect.Constructor.class, java.lang.reflect.Array.class,
      java.lang.reflect.Modifier.class, java.lang.reflect.Proxy.class,
      java.lang.invoke.MethodHandle.class, java.lang.invoke.MethodType.class,
      java.math.BigInteger.class, java.math.BigDecimal.class,
      java.text.SimpleDateFormat.class, java.text.DecimalFormat.class,
      java.net.URL.class, java.net.URI.class, java.net.InetAddress.class,
      java.security.MessageDigest.class, java.util.regex.Pattern.class,
      java.util.regex.Matcher.class,
    };

    TypeManager tm = new TypeManager(null, null);
    TypeFactoryHarness tf = new TypeFactoryHarness(tm);
    tm.typeFactory = tf;
    tm.init();

    for (Class<?> cls : classes)
    {
      tm.findClass(cls);
      tm.populateMembers(cls);
    }

    tm.shutdown();

    int leaked = 0;
    for (Resource entry : tf.resourceMap.values())
      if (!(entry instanceof DeletedResource))
        leaked++;
    assertEquals(leaked, 0, "TypeManager leaked resources after a large shutdown batch");
  }

  @Test
  public void testClassDescriptorRejectsNullPointer()
  {
    try
    {
      new ClassDescriptor(Object.class, 0L, null);
      fail("Expected NullPointerException for a zero class pointer");
    } catch (NullPointerException expected)
    {
      assertTrue(expected.getMessage().contains("Class pointer is null"));
    }
  }

  @Test
  public void testClassDescriptorGetMethodNotFoundReturnsZero() throws NoSuchMethodException
  {
    ClassDescriptor desc = new ClassDescriptor(String.class, 1L, null);
    desc.methodIndex = new java.lang.reflect.Executable[]
    {
      String.class.getMethod("length")
    };
    desc.methods = new long[]
    {
      42L
    };
    assertEquals(desc.getMethod(String.class.getMethod("length")), 42L);
    assertEquals(desc.getMethod(String.class.getMethod("trim")), 0L);
  }

  @Test
  public void testFindClassByNameUsesLookup()
  {
    TypeManager tm = new TypeManager(null, null);
    TypeFactoryHarness tf = new TypeFactoryHarness(tm);
    tm.typeFactory = tf;
    tm.init();

    long found = tm.findClassByName("java.lang.String");
    assertTrue(found != 0);
    assertEquals(tm.findClassByName("java.lang.String"), found);
    assertEquals(tm.findClassByName("no.such.package.NoSuchClass"), 0);
  }
}
