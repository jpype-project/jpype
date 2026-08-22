// --- file: org/jpype/internal/SupportNGTest.java ---
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
package org.jpype.internal;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Path;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Support is package-private ("used exclusively through JNI" per its
 * Javadoc) but its methods are plain deterministic Java with no native
 * dependency, so they can be called directly from a same-package test
 * without going through the interpreter bridge at all.
 */
public class SupportNGTest
{

  @Test
  public void testGetJarPath()
  {
    Path path = Support.getJarPath(SupportNGTest.class);
    assertNotNull(path);
  }

  @Test
  public void testCollectRectangular1D()
  {
    int[] a = new int[]
    {
      1, 2, 3
    };
    Object[] result = Support.collectRectangular(a);
    assertNotNull(result);
    assertEquals(result[0], int.class);
    assertEquals((int[]) result[1], new int[]
    {
      3
    });
  }

  @Test
  public void testCollectRectangular2D()
  {
    int[][] a = new int[][]
    {
      {
        1, 2
      },
      {
        3, 4
      }
    };
    Object[] result = Support.collectRectangular(a);
    assertNotNull(result);
    assertEquals(result[0], int.class);
    assertEquals((int[]) result[1], new int[]
    {
      2, 2
    });
  }

  @Test
  public void testCollectRectangularNull()
  {
    assertNull(Support.collectRectangular(null));
  }

  @Test
  public void testCollectRectangularNonArray()
  {
    assertNull(Support.collectRectangular("not an array"));
  }

  @Test
  public void testCollectRectangularObjectArrayRejected()
  {
    // Only primitive leaf arrays are supported.
    assertNull(Support.collectRectangular(new String[]
    {
      "a", "b"
    }));
  }

  @Test
  public void testCollectRectangularRaggedRejected()
  {
    int[][] a = new int[][]
    {
      {
        1, 2
      },
      {
        3
      }
    };
    assertNull(Support.collectRectangular(a));
  }

  @Test
  public void testCollectRectangularFiveDimensionsRejected()
  {
    int[][][][][] a = new int[1][1][1][1][1];
    assertNull(Support.collectRectangular(a));
  }

  @Test
  public void testAssembleOneDimension()
  {
    // 1D is the degenerate case: collectRectangular's single leaf *is*
    // the original array, so assemble just hands it back unwrapped.
    Object parts = new Object[]
    {
      "only"
    };
    assertEquals(Support.assemble(new int[]
    {
      1
    }, parts), "only");
  }

  @Test
  public void testAssembleTwoDimensions()
  {
    // dims=[rows,cols], parts=one leaf int[cols] row per element - the
    // shape collectRectangular(int[rows][cols]) would have produced.
    int[] row0 = new int[]
    {
      1, 2, 3
    };
    int[] row1 = new int[]
    {
      4, 5, 6
    };
    Object[] parts = new Object[]
    {
      row0, row1
    };
    Object result = Support.assemble(new int[]
    {
      2, 3
    }, parts);
    assertEquals((int[][]) result, new int[][]
    {
      {
        1, 2, 3
      },
      {
        4, 5, 6
      }
    });
  }

  @Test
  public void testCollectRectangularThenAssembleRoundTrip()
  {
    int[][][] original = new int[][][]
    {
      {
        {
          1, 2
        },
        {
          3, 4
        }
      },
      {
        {
          5, 6
        },
        {
          7, 8
        }
      }
    };
    Object[] collected = Support.collectRectangular(original);
    assertNotNull(collected);
    int[] shape = (int[]) collected[1];
    Object[] leaves = java.util.Arrays.copyOfRange(collected, 2, collected.length);
    Object reassembled = Support.assemble(shape, leaves);
    assertEquals((int[][][]) reassembled, original);
  }

  @Test
  public void testOrderLittleEndian()
  {
    ByteBuffer b = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN);
    assertTrue(Support.order(b));
  }

  @Test
  public void testOrderBigEndian()
  {
    ByteBuffer b = ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN);
    assertFalse(Support.order(b));
  }

  @Test
  public void testGetStackTraceWithoutEnclosing()
  {
    Throwable th = new RuntimeException("boom");
    Object[] frames = Support.getStackTrace(th, null);
    assertNotNull(frames);
    assertTrue(frames.length >= 4);
    assertEquals(frames[0], SupportNGTest.class.getName());
  }

  @Test
  public void testGetStackTraceWithEnclosingTruncates() throws Exception
  {
    Throwable enclosing = new RuntimeException("enclosing");
    Throwable th;
    try
    {
      throw new RuntimeException("inner");
    } catch (RuntimeException ex)
    {
      th = ex;
    }
    Object[] framesWithout = Support.getStackTrace(th, null);
    Object[] framesWith = Support.getStackTrace(th, enclosing);
    assertNotNull(framesWith);
    // Truncated (or equal, if no common frame is found) - never longer.
    assertTrue(framesWith.length <= framesWithout.length);
  }

  @Test
  public void testMemoryAccessors()
  {
    assertTrue(Support.getTotalMemory() > 0);
    assertTrue(Support.getMaxMemory() > 0);
    assertTrue(Support.getFreeMemory() >= 0);
    assertTrue(Support.getUsedMemory() >= 0);
    assertTrue(Support.getHeapMemory() >= 0);
  }

}
