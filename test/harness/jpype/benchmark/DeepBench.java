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

// Cross-library benchmark harness for deeper conversion-chain paths than
// project/benchmark/bench_*.py's simple Math.max/Integer/String cases:
// overload resolution across many candidates, and array/List argument
// conversion (which is inherently content-dependent -- see
// JPConversionSequence in jp_classhints.cpp -- so never cacheable, unlike
// the hint-list scan this session's work targeted).
public class DeepBench
{

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

  public static long sumIntList(List<Integer> a)
  {
    long s = 0;
    for (int x : a)
      s += x;
    return s;
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
