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
package jpype.exc;

public class ExceptionTest
{

  public static void throwRuntime()
  {
    throw new RuntimeException("Foo");
  }

  public static void throwIOException() throws java.io.IOException
  {
    throw new java.io.IOException("Bar");
  }

  public static boolean delegateThrow(ExceptionThrower th)
  {
    try
    {
      th.throwIOException();
    } catch (java.io.IOException ex)
    {
      return true;
    } catch (Throwable ex)
    {
      System.out.println("Unexpected Exception during delegateThrow");
      ex.printStackTrace();
    }
    System.out.println("Failed");
    return false;
  }

  public static void throwChildTestException() throws ParentTestException
  {
    throw new ChildTestException();
  }

  public static void throwChain()
  {
    method1();
  }

  static void method1()
  {
    method2();
  }

  static void method2()
  {
    throw new RuntimeException("Inner");
  }
}
