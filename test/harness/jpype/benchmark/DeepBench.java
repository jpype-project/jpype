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
package jpype.benchmark;

import java.util.List;
import java.util.Random;

// Cross-library benchmark harness for deeper conversion-chain paths than
// project/benchmark/{jpype,jpy,jep,pyjnius}/int.py's simple
// Math.max/Integer/String cases: overload resolution across many
// candidates, and array/List argument conversion (which is inherently
// content-dependent -- see JPConversionSequence in jp_classhints.cpp --
// unlike the hint-list scan project/benchmark/jpype/classhints.py
// measures.
public class DeepBench
{

  // Fixed-seed, precomputed once at class load -- so make*IntArray/
  // make*LongArray's own fill loop stays a cheap bulk copy/index-read
  // inside the timed pull benchmark (same shape of cost as the sequential
  // a[i]=i it replaces), while the values themselves are spread across the
  // full int/long range instead of 0..n-1. Sequential 0..n-1 sits almost
  // entirely inside CPython's small-int cache (-5..256) for the n=100 row,
  // so any library boxing pulled elements via plain PyLong_FromLong (not
  // jpype's tagged JInt/JLong, which always allocates fresh regardless of
  // value) would get that row nearly for free -- not representative of
  // real, non-cached integer data. float/double aren't cached this way in
  // CPython, so their make*Array fill is left sequential.
  private static final int RANDOM_POOL_SIZE = 100_000;
  private static final int[] RANDOM_INT_POOL = makeRandomIntPool(RANDOM_POOL_SIZE, 0x5EEDL);
  private static final long[] RANDOM_LONG_POOL = makeRandomLongPool(RANDOM_POOL_SIZE, 0x5EEDL);

  private static int[] makeRandomIntPool(int size, long seed)
  {
    Random r = new Random(seed);
    int[] pool = new int[size];
    for (int i = 0; i < size; i++)
      pool[i] = r.nextInt();
    return pool;
  }

  private static long[] makeRandomLongPool(int size, long seed)
  {
    Random r = new Random(seed);
    long[] pool = new long[size];
    for (int i = 0; i < size; i++)
      pool[i] = r.nextLong();
    return pool;
  }

  public static class T0
  {
  }

  public static class T1
  {
  }

  public static class T2
  {
  }

  public static class T3
  {
  }

  public static class T4
  {
  }

  public static class T5
  {
  }

  public static class T6
  {
  }

  public static class T7
  {
  }

  public static class T8
  {
  }

  public static class T9
  {
  }

  public static class T10
  {
  }

  public static class T11
  {
  }

  public static class T12
  {
  }

  public static class T13
  {
  }

  public static class T14
  {
  }

  public static class T15
  {
  }

  public static int call(T0 a)
  {
    return 0;
  }

  public static int call(T1 a)
  {
    return 1;
  }

  public static int call(T2 a)
  {
    return 2;
  }

  public static int call(T3 a)
  {
    return 3;
  }

  public static int call(T4 a)
  {
    return 4;
  }

  public static int call(T5 a)
  {
    return 5;
  }

  public static int call(T6 a)
  {
    return 6;
  }

  public static int call(T7 a)
  {
    return 7;
  }

  public static int call(T8 a)
  {
    return 8;
  }

  public static int call(T9 a)
  {
    return 9;
  }

  public static int call(T10 a)
  {
    return 10;
  }

  public static int call(T11 a)
  {
    return 11;
  }

  public static int call(T12 a)
  {
    return 12;
  }

  public static int call(T13 a)
  {
    return 13;
  }

  public static int call(T14 a)
  {
    return 14;
  }

  // T15 -- the last of 16 overloads, worst case for a linear scan starting
  // from the first candidate.
  public static int call(T15 a)
  {
    return 15;
  }

  public static long sumIntArray(int[] a)
  {
    long s = 0;
    for (int x : a)
      s += x;
    return s;
  }

  // Identity passthrough for a 1D int array -- used by jep/array_multidim.py
  // to manually assemble a genuine multi-dimensional Java array one row at a
  // time (jep's numpy fast path only ever targets a flat int[]; there's no
  // way to bulk-load a multi-dim array in one call, so each leaf row is
  // bulk-converted via this method instead, and the nesting above that is
  // pure Python-side array construction -- see that file for details).
  public static int[] identityIntArray(int[] a)
  {
    return a;
  }

  // long/float/double counterparts of identityIntArray -- same role, for
  // sweeping element type through the non-contiguous-buffer benchmark.
  public static long[] identityLongArray(long[] a)
  {
    return a;
  }

  public static float[] identityFloatArray(float[] a)
  {
    return a;
  }

  public static double[] identityDoubleArray(double[] a)
  {
    return a;
  }

  // 2D/3D/4D/5D counterparts of identityIntArray -- unlike sum*DIntArray
  // (which discards position: summing is invariant to any permutation of
  // the same values, so it can't catch a transposed axis or a misindexed
  // reshape), echoing the array back lets the Python side compare it
  // elementwise against the original and confirm every value actually
  // landed at its own position, not just that the right values arrived
  // somewhere.
  public static int[][] identity2DIntArray(int[][] a)
  {
    return a;
  }

  public static int[][][] identity3DIntArray(int[][][] a)
  {
    return a;
  }

  public static int[][][][] identity4DIntArray(int[][][][] a)
  {
    return a;
  }

  public static int[][][][][] identity5DIntArray(int[][][][][] a)
  {
    return a;
  }

  // byte/boolean/char 2D counterparts -- unlike int/short/long/float/double
  // (which get a sum2D*Array method for the dtype-mismatch push test), these
  // three primitive types have no natural numeric "sum" (or, for boolean, no
  // useful reduction at all), so an identity passthrough is used instead to
  // exercise the same JPConversionMultiArrayBuffer -> newMultiArrayObject
  // method-argument dtype-coercion path.
  public static byte[][] identity2DByteArray(byte[][] a)
  {
    return a;
  }

  public static boolean[][] identity2DBooleanArray(boolean[][] a)
  {
    return a;
  }

  public static char[][] identity2DCharArray(char[][] a)
  {
    return a;
  }

  // Zero-Java-side-compute push benchmarking targets: unlike sumXArray
  // (which does type-dependent O(elements) work, see
  // project/benchmark/RESULTS.md's push methodology note) and unlike
  // identityXArray above (which echoes the array back, adding a real if
  // small and type-uniform array-wrapping cost to the return path), these
  // do nothing at all with the argument and return nothing -- isolating
  // push/conversion cost from every other confound this file's methods
  // carry.
  public static void voidIntArray(int[] a)
  {
  }

  public static void voidLongArray(long[] a)
  {
  }

  public static void voidFloatArray(float[] a)
  {
  }

  public static void voidDoubleArray(double[] a)
  {
  }

  public static long sumIntList(List<Integer> a)
  {
    long s = 0;
    for (int x : a)
      s += x;
    return s;
  }

  // Overloads differing only in array leaf element type -- regression
  // coverage for ragged-native list-push overload matching: confirms
  // matches() correctly qualifies/disqualifies each candidate for a
  // ragged plain-int nested list (no buffer built for either candidate,
  // since that only happens in convert() for the winner). int[][] and
  // long[][] are unrelated Java types with no widening relationship, so
  // a plain-int list qualifies both at equal quality and
  // JPMethodDispatch::findOverload correctly raises Ambiguous, the same
  // as it would for any other pair of equally-qualified, unrelated
  // candidates.
  public static String overloadArrayType(int[][] a)
  {
    return "int";
  }

  public static String overloadArrayType(long[][] a)
  {
    return "long";
  }

  // 2D short array -- declared-parameter dispatch, not the JArray(...)
  // constructor, is required to actually reach findJavaConversionImpl's
  // own match/no-match branches; the constructor path short-circuits
  // invalid input before ever calling it. short/byte/char/boolean are
  // ragged-eligible the same as int/long/float/double (isRaggedEligible,
  // jp_classhints.cpp), so this goes through JPArrayClassNestedRagged
  // like sum2DIntArray et al. -- see void2DByteArray et al. below for the
  // 3D/4D/5D and zero-Java-side-compute push benchmarking counterparts
  // for all four of these narrower leaf types.
  public static long sum2DShortArray(short[][] a)
  {
    long s = 0;
    for (short[] row : a)
      for (short x : row)
        s += x;
    return s;
  }

  // 2D variant of sumIntArray -- component type is itself int[], so
  // conversion recurses through the array-conversion machinery once per
  // outer element in addition to the per-element work each inner array
  // already does.
  public static long sum2DIntArray(int[][] a)
  {
    long s = 0;
    for (int[] row : a)
      for (int x : row)
        s += x;
    return s;
  }

  // 3D/4D/5D variants of sum2DIntArray -- see project/benchmark/bench_arrays_*.py,
  // which sweeps nesting depth (holding total element count fixed) to isolate
  // per-dimension conversion overhead from raw element count.
  public static long sum3DIntArray(int[][][] a)
  {
    long s = 0;
    for (int[][] plane : a)
      for (int[] row : plane)
        for (int x : row)
          s += x;
    return s;
  }

  public static long sum4DIntArray(int[][][][] a)
  {
    long s = 0;
    for (int[][][] cube : a)
      for (int[][] plane : cube)
        for (int[] row : plane)
          for (int x : row)
            s += x;
    return s;
  }

  public static long sum5DIntArray(int[][][][][] a)
  {
    long s = 0;
    for (int[][][][] hcube : a)
      for (int[][][] cube : hcube)
        for (int[][] plane : cube)
          for (int[] row : plane)
            for (int x : row)
              s += x;
    return s;
  }

  // 2D/3D/4D/5D counterparts of voidIntArray -- zero-Java-side-compute push
  // benchmarking at depth: unlike sum{2,3,4,5}DIntArray, these do no
  // per-element work and return nothing at all, isolating the conversion
  // cost itself from the (type-dependent -- int/long's simple-accumulator
  // sum auto-vectorizes, float/double's does not, see
  // project/benchmark/RESULTS.md's push methodology note) cost of actually
  // summing the elements afterward, and from identityIntArray's own
  // array-wrapping return cost.
  public static void void2DIntArray(int[][] a)
  {
  }

  public static void void3DIntArray(int[][][] a)
  {
  }

  public static void void4DIntArray(int[][][][] a)
  {
  }

  public static void void5DIntArray(int[][][][][] a)
  {
  }

  // 2D/3D/4D/5D zero-Java-side-compute push benchmarking targets for the
  // four leaf types added to isRaggedEligible (jp_classhints.cpp)
  // alongside int/long/float/double -- byte/boolean/char/short. Same
  // rationale as void2DIntArray et al. above: no per-element work, no
  // return value, isolating push/conversion cost (including the 4-byte
  // length-marker padding these 1-/2-byte-wide leaf types require and
  // int/long/float/double don't -- see raggedAlign4, jp_classhints.cpp)
  // from everything else.
  public static void void2DByteArray(byte[][] a)
  {
  }

  public static void void3DByteArray(byte[][][] a)
  {
  }

  public static void void4DByteArray(byte[][][][] a)
  {
  }

  public static void void5DByteArray(byte[][][][][] a)
  {
  }

  public static void void2DBooleanArray(boolean[][] a)
  {
  }

  public static void void3DBooleanArray(boolean[][][] a)
  {
  }

  public static void void4DBooleanArray(boolean[][][][] a)
  {
  }

  public static void void5DBooleanArray(boolean[][][][][] a)
  {
  }

  public static void void2DCharArray(char[][] a)
  {
  }

  public static void void3DCharArray(char[][][] a)
  {
  }

  public static void void4DCharArray(char[][][][] a)
  {
  }

  public static void void5DCharArray(char[][][][][] a)
  {
  }

  public static void void2DShortArray(short[][] a)
  {
  }

  public static void void3DShortArray(short[][][] a)
  {
  }

  public static void void4DShortArray(short[][][][] a)
  {
  }

  public static void void5DShortArray(short[][][][][] a)
  {
  }

  // "make*IntArray" -- the Java-side counterpart of sum*IntArray, for
  // benchmarking the opposite direction (a Java array's contents flowing
  // back into Python as the method return value) at the same sizes/depths.
  // Filled (not left zeroed) so a bulk buffer-protocol readback path can't
  // be short-circuited by an all-zeroes special case on either side.
  public static int[] makeIntArray(int n)
  {
    int[] a = new int[n];
    System.arraycopy(RANDOM_INT_POOL, 0, a, 0, n);
    return a;
  }

  public static int[][] make2DIntArray(int n)
  {
    int[][] a = new int[n][n];
    int idx = 0;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        a[i][j] = RANDOM_INT_POOL[idx++ % RANDOM_POOL_SIZE];
    return a;
  }

  public static int[][][] make3DIntArray(int n)
  {
    int[][][] a = new int[n][n][n];
    int idx = 0;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          a[i][j][k] = RANDOM_INT_POOL[idx++ % RANDOM_POOL_SIZE];
    return a;
  }

  public static int[][][][] make4DIntArray(int n)
  {
    int[][][][] a = new int[n][n][n][n];
    int idx = 0;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            a[i][j][k][l] = RANDOM_INT_POOL[idx++ % RANDOM_POOL_SIZE];
    return a;
  }

  public static int[][][][][] make5DIntArray(int n)
  {
    int[][][][][] a = new int[n][n][n][n][n];
    int idx = 0;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            for (int m = 0; m < n; m++)
              a[i][j][k][l][m] = RANDOM_INT_POOL[idx++ % RANDOM_POOL_SIZE];
    return a;
  }

  // long/float/double counterparts of the sum*IntArray/make*IntArray family
  // above -- same shapes, same role, so the array conversion benchmarks can
  // sweep element type (4/8-byte int, 4/8-byte float) as well as size/depth.

  public static long sumLongArray(long[] a)
  {
    long s = 0;
    for (long x : a)
      s += x;
    return s;
  }

  public static long sum2DLongArray(long[][] a)
  {
    long s = 0;
    for (long[] row : a)
      for (long x : row)
        s += x;
    return s;
  }

  public static long sum3DLongArray(long[][][] a)
  {
    long s = 0;
    for (long[][] plane : a)
      for (long[] row : plane)
        for (long x : row)
          s += x;
    return s;
  }

  public static long sum4DLongArray(long[][][][] a)
  {
    long s = 0;
    for (long[][][] cube : a)
      for (long[][] plane : cube)
        for (long[] row : plane)
          for (long x : row)
            s += x;
    return s;
  }

  public static long sum5DLongArray(long[][][][][] a)
  {
    long s = 0;
    for (long[][][][] hcube : a)
      for (long[][][] cube : hcube)
        for (long[][] plane : cube)
          for (long[] row : plane)
            for (long x : row)
              s += x;
    return s;
  }

  // See void2DIntArray etc. for why these exist.
  public static void void2DLongArray(long[][] a)
  {
  }

  public static void void3DLongArray(long[][][] a)
  {
  }

  public static void void4DLongArray(long[][][][] a)
  {
  }

  public static void void5DLongArray(long[][][][][] a)
  {
  }

  public static long[] makeLongArray(int n)
  {
    long[] a = new long[n];
    System.arraycopy(RANDOM_LONG_POOL, 0, a, 0, n);
    return a;
  }

  public static long[][] make2DLongArray(int n)
  {
    long[][] a = new long[n][n];
    int idx = 0;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        a[i][j] = RANDOM_LONG_POOL[idx++ % RANDOM_POOL_SIZE];
    return a;
  }

  public static long[][][] make3DLongArray(int n)
  {
    long[][][] a = new long[n][n][n];
    int idx = 0;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          a[i][j][k] = RANDOM_LONG_POOL[idx++ % RANDOM_POOL_SIZE];
    return a;
  }

  public static long[][][][] make4DLongArray(int n)
  {
    long[][][][] a = new long[n][n][n][n];
    int idx = 0;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            a[i][j][k][l] = RANDOM_LONG_POOL[idx++ % RANDOM_POOL_SIZE];
    return a;
  }

  public static long[][][][][] make5DLongArray(int n)
  {
    long[][][][][] a = new long[n][n][n][n][n];
    int idx = 0;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            for (int m = 0; m < n; m++)
              a[i][j][k][l][m] = RANDOM_LONG_POOL[idx++ % RANDOM_POOL_SIZE];
    return a;
  }

  public static double sumFloatArray(float[] a)
  {
    double s = 0;
    for (float x : a)
      s += x;
    return s;
  }

  public static double sum2DFloatArray(float[][] a)
  {
    double s = 0;
    for (float[] row : a)
      for (float x : row)
        s += x;
    return s;
  }

  public static double sum3DFloatArray(float[][][] a)
  {
    double s = 0;
    for (float[][] plane : a)
      for (float[] row : plane)
        for (float x : row)
          s += x;
    return s;
  }

  public static double sum4DFloatArray(float[][][][] a)
  {
    double s = 0;
    for (float[][][] cube : a)
      for (float[][] plane : cube)
        for (float[] row : plane)
          for (float x : row)
            s += x;
    return s;
  }

  public static double sum5DFloatArray(float[][][][][] a)
  {
    double s = 0;
    for (float[][][][] hcube : a)
      for (float[][][] cube : hcube)
        for (float[][] plane : cube)
          for (float[] row : plane)
            for (float x : row)
              s += x;
    return s;
  }

  // See void2DIntArray etc. for why these exist.
  public static void void2DFloatArray(float[][] a)
  {
  }

  public static void void3DFloatArray(float[][][] a)
  {
  }

  public static void void4DFloatArray(float[][][][] a)
  {
  }

  public static void void5DFloatArray(float[][][][][] a)
  {
  }

  public static float[] makeFloatArray(int n)
  {
    float[] a = new float[n];
    for (int i = 0; i < n; i++)
      a[i] = i;
    return a;
  }

  public static float[][] make2DFloatArray(int n)
  {
    float[][] a = new float[n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        a[i][j] = i * n + j;
    return a;
  }

  public static float[][][] make3DFloatArray(int n)
  {
    float[][][] a = new float[n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          a[i][j][k] = (i * n + j) * n + k;
    return a;
  }

  public static float[][][][] make4DFloatArray(int n)
  {
    float[][][][] a = new float[n][n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            a[i][j][k][l] = ((i * n + j) * n + k) * n + l;
    return a;
  }

  public static float[][][][][] make5DFloatArray(int n)
  {
    float[][][][][] a = new float[n][n][n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            for (int m = 0; m < n; m++)
              a[i][j][k][l][m] = (((i * n + j) * n + k) * n + l) * n + m;
    return a;
  }

  public static double sumDoubleArray(double[] a)
  {
    double s = 0;
    for (double x : a)
      s += x;
    return s;
  }

  public static double sum2DDoubleArray(double[][] a)
  {
    double s = 0;
    for (double[] row : a)
      for (double x : row)
        s += x;
    return s;
  }

  public static double sum3DDoubleArray(double[][][] a)
  {
    double s = 0;
    for (double[][] plane : a)
      for (double[] row : plane)
        for (double x : row)
          s += x;
    return s;
  }

  public static double sum4DDoubleArray(double[][][][] a)
  {
    double s = 0;
    for (double[][][] cube : a)
      for (double[][] plane : cube)
        for (double[] row : plane)
          for (double x : row)
            s += x;
    return s;
  }

  public static double sum5DDoubleArray(double[][][][][] a)
  {
    double s = 0;
    for (double[][][][] hcube : a)
      for (double[][][] cube : hcube)
        for (double[][] plane : cube)
          for (double[] row : plane)
            for (double x : row)
              s += x;
    return s;
  }

  // See void2DIntArray etc. for why these exist.
  public static void void2DDoubleArray(double[][] a)
  {
  }

  public static void void3DDoubleArray(double[][][] a)
  {
  }

  public static void void4DDoubleArray(double[][][][] a)
  {
  }

  public static void void5DDoubleArray(double[][][][][] a)
  {
  }

  public static double[] makeDoubleArray(int n)
  {
    double[] a = new double[n];
    for (int i = 0; i < n; i++)
      a[i] = i;
    return a;
  }

  public static double[][] make2DDoubleArray(int n)
  {
    double[][] a = new double[n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        a[i][j] = i * n + j;
    return a;
  }

  public static double[][][] make3DDoubleArray(int n)
  {
    double[][][] a = new double[n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          a[i][j][k] = (i * n + j) * n + k;
    return a;
  }

  public static double[][][][] make4DDoubleArray(int n)
  {
    double[][][][] a = new double[n][n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            a[i][j][k][l] = ((i * n + j) * n + k) * n + l;
    return a;
  }

  public static double[][][][][] make5DDoubleArray(int n)
  {
    double[][][][][] a = new double[n][n][n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            for (int m = 0; m < n; m++)
              a[i][j][k][l][m] = (((i * n + j) * n + k) * n + l) * n + m;
    return a;
  }

  // "object" category: argument matching + return-value wrapping for a
  // plain Object, as opposed to a primitive/boxed/array/string value.
  public static Object identity(Object o)
  {
    return o;
  }

  // "proxy" category: Java calling back into Python through an interface
  // a Python object implements. Each library has its own mechanism for
  // exposing a Python object as this interface (see
  // project/benchmark/README.md); invokeCallback itself is the same call
  // for all of them once that binding exists.
  public interface Callback
  {
    int run(int x);
  }

  public static int invokeCallback(Callback cb, int x)
  {
    return cb.run(x);
  }

  // Loop-on-the-Java-side variant: needed for libraries (jpy) whose
  // Python-side binding can't reliably call methods on a proxy object
  // directly -- see project/benchmark/README.md. Divide the result's
  // wall-clock time by iterations for a per-call figure.
  public static long invokeCallbackLoop(Callback cb, int iterations)
  {
    long sum = 0;
    for (int i = 0; i < iterations; i++)
      sum += cb.run(i);
    return sum;
  }

  // Regression coverage for jp_proxy.cpp's getArgs(): a proxy callback
  // argument's runtime class isn't always the declared one -- covers both
  // a genuinely null argument (GetObjectClass/IsSameObject must not be
  // called on it) and a covariant one (declared Object, actual T15).
  public interface ObjectCallback
  {
    Object handle(Object o);
  }

  public static Object invokeObjectCallbackWithNull(ObjectCallback cb)
  {
    return cb.handle(null);
  }

  public static Object invokeObjectCallbackWithSubtype(ObjectCallback cb)
  {
    return cb.handle(new T15());
  }

  public static Object invokeObjectCallback(ObjectCallback cb, Object o)
  {
    return cb.handle(o);
  }
}
