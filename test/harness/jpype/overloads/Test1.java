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
package jpype.overloads;

import java.util.List;

public class Test1
{

  public static class A
  {
  }

  public static class B extends A
  {
  }

  public static class C extends B
  {
  }

  public String testMostSpecific(A a)
  {
    return "A";
  }

  public String testMostSpecific(B b)
  {
    return "B";
  }

  public String testVarArgs(A a, A... as)
  {
    return "A,A...";
  }

  public String testVarArgs(A a, B... bs)
  {
    return "A,B...";
  }

  public String testVarArgs(B a, B... bs)
  {
    return "B,B...";
  }

  public String testPrimitive(byte v)
  {
    return "byte";
  }

  public String testPrimitive(Byte v)
  {
    return "Byte";
  }

  public String testPrimitive(short v)
  {
    return "short";
  }

  public String testPrimitive(Short v)
  {
    return "Short";
  }

  public String testPrimitive(int v)
  {
    return "int";
  }

  public String testPrimitive(Integer v)
  {
    return "Integer";
  }

  public String testPrimitive(long v)
  {
    return "long";
  }

  public String testPrimitive(Long v)
  {
    return "Long";
  }

  public String testPrimitive(float v)
  {
    return "float";
  }

  public String testPrimitive(Float v)
  {
    return "Float";
  }

  public String testPrimitive(double v)
  {
    return "double";
  }

  public String testPrimitive(Double v)
  {
    return "Double";
  }

  public String testPrimitive(boolean v)
  {
    return "boolean";
  }

  public String testPrimitive(Boolean v)
  {
    return "Boolean";
  }

  public String testPrimitive(char v)
  {
    return "char";
  }

  public String testPrimitive(Character v)
  {
    return "Character";
  }

  public static String testInstanceVsClass(B b)
  {
    return "static B";
  }

  public String testInstanceVsClass(A a)
  {
    return "instance A";
  }

  public static String testJavaInstanceVsClass()
  {
    return new Test1().testInstanceVsClass(new C());
  }

  /*
                      I1
                    /   \
                   I2    I3
                    \   /  \
                      I4   I5
                      |  \  |
                      I6   I7
                         \  |
                           I8
   */
  public static interface I1
  {
  }

  public static interface I2 extends I1
  {
  }

  public static interface I3 extends I1
  {
  }

  public static interface I4 extends I2, I3
  {
  }

  public static interface I5 extends I3
  {
  }

  public static interface I6 extends I4
  {
  }

  public static interface I7 extends I4, I5
  {
  }

  public static interface I8 extends I6, I7
  {
  }

  public static class I1Impl implements I1
  {
  }

  public static class I2Impl implements I2
  {
  }

  public static class I3Impl implements I3
  {
  }

  public static class I4Impl implements I4
  {
  }

  public static class I5Impl implements I5
  {
  }

  public static class I6Impl implements I6
  {
  }

  public static class I7Impl implements I7
  {
  }

  public static class I8Impl implements I8
  {
  }

  public String testInterfaces1(I2 v)
  {
    return "I2";
  }

  public String testInterfaces1(I3 v)
  {
    return "I3";
  }

  public String testInterfaces2(I2 v)
  {
    return "I2";
  }

  public String testInterfaces2(I3 v)
  {
    return "I3";
  }

  public String testInterfaces2(I4 v)
  {
    return "I4";
  }

  public String testInterfaces3(I4 v)
  {
    return "I4";
  }

  public String testInterfaces3(I5 v)
  {
    return "I5";
  }

  public String testInterfaces4(I1 v)
  {
    return "I1";
  }

  public String testInterfaces4(I2 v)
  {
    return "I2";
  }

  public String testInterfaces4(I3 v)
  {
    return "I3";
  }

  public String testInterfaces4(I4 v)
  {
    return "I4";
  }

  public String testInterfaces4(I5 v)
  {
    return "I5";
  }

  public String testInterfaces4(I6 v)
  {
    return "I6";
  }

  public String testInterfaces4(I7 v)
  {
    return "I7";
  }

  public String testInterfaces4(I8 v)
  {
    return "I8";
  }

  public String testClassVsObject(Object v)
  {
    return "Object";
  }

  public String testClassVsObject(Class v)
  {
    return "Class";
  }

  public String testStringArray(Object v)
  {
    return "Object";
  }

  public String testStringArray(String v)
  {
    return "String";
  }

  public String testStringArray(String[] v)
  {
    return "String[]";
  }

  public String testListVSArray(String[] v)
  {
    return "String[]";
  }

  public String testListVSArray(List<String> v)
  {
    return "List<String>";
  }

  /* tests for java 1.8 default methods, commented out for travis environment
    public interface IDefaultA {
        public default String defaultMethod() {
            return "A";
        }
    }
    public interface IDefaultB extends IDefaultA {
        public default String defaultMethod() {
            return "B";
        }
    }
    public interface IDefaultC extends IDefaultA, IDefaultB {
    }
    public static class DefaultC implements IDefaultC {
    }
    //*/
}
