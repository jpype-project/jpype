// --- file: python/lang/PyMemoryViewNumpyNGTest.java ---
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

import org.testng.SkipException;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * Proves (or disproves) {@code plan/NumpyBufferBench.md}'s claim: jpype's
 * existing buffer-protocol surface ({@link PyMemoryView}/{@link PyBuffer})
 * is already sufficient for numpy interop via {@code np.frombuffer}/
 * {@code np.asarray} - no numpy-specific glue code needed in jpype itself,
 * since PEP 3118 buffers natively carry format/shape/strides.
 *
 * <p>
 * Kept in its own file, isolated from {@link PyMemoryViewNGTest}, so numpy
 * stays an optional/skippable dependency for the rest of the suite - skips
 * the whole class rather than failing if numpy isn't importable.</p>
 */
public class PyMemoryViewNumpyNGTest extends PyTestHarness
{

  @BeforeClass
  public static void requireNumpy()
  {
    try
    {
      context.exec("import numpy as np");
    } catch (Throwable ex)
    {
      throw new SkipException("numpy not importable in this environment - skipping numpy interop bench", ex);
    }
  }

  @Test
  public void test1DUint8ZeroCopyRoundTrip()
  {
    // A writable bytearray's memoryview, handed to np.frombuffer, is a real
    // zero-copy view: mutating through numpy must be visible back on the
    // Python-side bytearray.
    context.exec(
            "data = bytearray(range(8))\n"
            + "view = memoryview(data)\n"
            + "arr = np.frombuffer(view, dtype=np.uint8)\n"
            + "assert list(arr) == list(range(8)), arr\n"
            + "arr[0] = 99\n"
            + "assert data[0] == 99, 'mutation through numpy did not reach the underlying bytearray - not zero-copy'\n"
    );
  }

  @Test
  public void testReadOnlyBytesProducesNonWritableArray()
  {
    // memoryview of an immutable `bytes` object must produce a non-writeable
    // numpy array, not a silent copy that would mask a real gap.
    context.exec(
            "data = bytes(range(8))\n"
            + "view = memoryview(data)\n"
            + "arr = np.frombuffer(view, dtype=np.uint8)\n"
            + "assert arr.flags.writeable is False, 'numpy array over an immutable bytes buffer should not be writeable'\n"
            + "try:\n"
            + "    arr[0] = 1\n"
            + "    raise AssertionError('writing to a non-writeable array should have raised')\n"
            + "except ValueError:\n"
            + "    pass\n"
    );

    // Sanity check the Java-side PyMemoryView.isReadOnly() surface agrees:
    // a memoryview of PyBytes must report readonly, unlike PyByteArray's.
    PyBytes readOnlySource = (PyBytes) context.eval("data");
    PyMemoryView readOnlyView = context.memoryview(readOnlySource);
    org.testng.Assert.assertTrue(readOnlyView.isReadOnly());
  }

  @Test
  public void testMultiDimCContiguousReshape()
  {
    // A flat buffer reshaped to 2-D on the Python side before being handed
    // across: confirm PyMemoryView.getShape()/getStrides() report the real
    // multi-dimensional, C-contiguous shape/strides once cast to Java.
    context.exec(
            "data = bytearray(range(12))\n"
            + "arr2d = np.frombuffer(data, dtype=np.uint8).reshape(3, 4)\n"
            + "view = memoryview(arr2d)\n"
            + "assert view.shape == (3, 4), view.shape\n"
            + "assert view.strides == (4, 1), view.strides\n"
    );

    PyMemoryView view = (PyMemoryView) context.eval("view");
    PyTuple shape = view.getShape();
    PyTuple strides = view.getStrides();
    org.testng.Assert.assertEquals(shape.size(), 2);
    org.testng.Assert.assertEquals(shape.get(0).toString(), "3");
    org.testng.Assert.assertEquals(shape.get(1).toString(), "4");
    org.testng.Assert.assertEquals(strides.get(0).toString(), "4");
    org.testng.Assert.assertEquals(strides.get(1).toString(), "1");
  }

  @Test
  public void testNonContiguousStridesFromSlicedArray()
  {
    // Slicing a numpy array with a step produces a non-contiguous buffer;
    // confirm the resulting memoryview's strides are the real (doubled)
    // stride, not a naive contiguous reinterpretation, and that np.asarray
    // reconstructs the correct logical values from it.
    context.exec(
            "base = np.arange(10, dtype=np.int32)\n"
            + "sliced = base[::2]\n"
            + "view = memoryview(sliced)\n"
            + "assert view.strides == (8,), view.strides\n"
            + "reconstructed = np.asarray(view)\n"
            + "assert list(reconstructed) == [0, 2, 4, 6, 8], list(reconstructed)\n"
    );

    PyMemoryView view = (PyMemoryView) context.eval("view");
    PyTuple strides = view.getStrides();
    org.testng.Assert.assertEquals(strides.size(), 1);
    org.testng.Assert.assertEquals(strides.get(0).toString(), "8");
  }

  @Test
  public void testCopyVsViewSemantics()
  {
    // np.asarray(view) is a true zero-copy view; np.array(view) forces a
    // copy. A future regression that accidentally starts copying in the
    // "view" path (silently losing zero-copy performance) should fail this.
    context.exec(
            "data = bytearray(range(4))\n"
            + "view = memoryview(data)\n"
            + "as_view = np.asarray(view)\n"
            + "as_copy = np.array(view)\n"
            + "as_view[0] = 55\n"
            + "assert data[0] == 55, 'np.asarray(view) should be zero-copy, mutation did not propagate'\n"
            + "as_copy[1] = 77\n"
            + "assert data[1] != 77, 'np.array(view) should have forced a copy, mutation leaked into the underlying bytearray'\n"
    );
  }

  @Test
  public void testFloat64FormatRoundTrip()
  {
    // A non-uint8 dtype case: confirm PyMemoryView.getFormat()'s PEP 3118
    // format string is dtype-correct (not just byte-granularity), and that
    // np.frombuffer with the matching dtype round-trips real float values.
    context.exec(
            "import array\n"
            + "data = array.array('d', [1.5, 2.5, 3.5])\n"
            + "view = memoryview(data)\n"
            + "assert view.format == 'd', view.format\n"
            + "arr = np.frombuffer(view, dtype=np.float64)\n"
            + "assert list(arr) == [1.5, 2.5, 3.5], list(arr)\n"
    );

    PyMemoryView view = (PyMemoryView) context.eval("view");
    org.testng.Assert.assertEquals(view.getFormat(), "d");
  }

  @Test
  public void testGetSliceRoundTrip()
  {
    // A PyMemoryView.getSlice(...)-derived sub-view, taken on the Java side,
    // should still produce a correct numpy array with the right offset -
    // not just the full view.
    context.exec("data = bytearray(range(20))");
    PyByteArray data = (PyByteArray) context.eval("data");
    PyMemoryView view = context.memoryview(data);
    PyMemoryView slice = view.getSlice(5, 15);

    context.globals().putAny("slice_view", slice);
    context.exec(
            "arr = np.frombuffer(slice_view, dtype=np.uint8)\n"
            + "assert list(arr) == list(range(5, 15)), list(arr)\n"
    );
  }
}
