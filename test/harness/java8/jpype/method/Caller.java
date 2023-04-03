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
package jpype.method;

/**
 * Caller sensitive methods need special handling.
 *
 * So we need to test each possible path (different return types and arguments).
 *
 * We will pretend our methods are also CallerSensitive.
 */
public class Caller
{

  public static Object callObjectStatic()
  {
    return new Caller();
  }

  public Object callObjectMember()
  {
    return new Caller();
  }

  public static void callVoidStatic()
  {
  }

  public void callVoidMember()
  {
  }

  public static int callIntegerStatic()
  {
    return 123;
  }

  public int callIntegerMember()
  {
    return 123;
  }

  public Object callArgs(Object a, Object b)
  {
    return b;
  }

  public int callArg1(int a)
  {
    return a;
  }

  public Object callVarArgs(Object a, Object... b)
  {
    return b;
  }

  public Class callStackWalker1()
  {
    return StackWalker.getInstance(StackWalker.Option.RETAIN_CLASS_REFERENCE).getCallerClass();
  }

  public Class callStackWalker2()
  {
    return StackWalker.getInstance(StackWalker.Option.RETAIN_CLASS_REFERENCE).getCallerClass();
  }

};
