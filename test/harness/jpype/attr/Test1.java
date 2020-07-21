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
package jpype.attr;

public class Test1
{

  private String mBigString;

  public Test1()
  {
    StringBuffer sb = new StringBuffer(4001);
    for (int i = 0; i < 4000; i++)
    {
      sb.append("A");
    }
    mBigString = sb.toString();
  }

  public String getBigString()
  {
    return mBigString;
  }

  public String toString()
  {
    return "aaa";
  }

  public static String[] testStaticString(String s1, String s2)
  {
    return new String[]
    {
      s1, s2
    };
  }

  public static String testStaticHolder(Holder h)
  {
    return h.f;
  }

  public String[] testString(String s1, String s2)
  {
    return new String[]
    {
      s1, s2
    };
  }

  public String[] testStringArray(String[] vals)
  {
    return vals;
  }

  public String stringValue = "Foo";
  public char charValue = 'a';
  public static Object objectValue = (int) 234;

  public static void reset()
  {
    objectValue = (int) 234;
  }

  public Object getSubClass()
  {
    return new SubHolder();
  }

  public void callWithClass(Class c)
  {
  }

  public void test1Method()
  {

  }

  public boolean mBooleanValue = false;

  public void setBoolean(boolean b)
  {
    mBooleanValue = b;
  }

  public byte mByteValue;

  public void setByte(byte b)
  {
    mByteValue = b;
  }

  public short mShortValue = 0;

  public void setShort(short s)
  {
    mShortValue = s;
  }

  public int mIntValue = 0;

  public void setInt(int i)
  {
    mIntValue = i;
  }

  public long mLongValue = 0;

  public void setLong(long l)
  {
    mLongValue = l;
  }

  public String callWithSomething(Object obj)
  {
    return "Object";
  }

  public String callWithSomething(Class obj)
  {
    return "Class";
  }

  public Test1 delete(String arg1, String arg2)
  {
    System.out.println("Overloaded test 1 called");
    return null;
  }

}
