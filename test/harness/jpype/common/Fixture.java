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
package jpype.common;

public class Fixture
{
  // Tests for access

  private static Object static_private_object_field
          = new String("private static object field");
  private Object private_object_field
          = new String("private object field");
  public final static Object final_static_object_field
          = new String("final static object field");
  public final Object final_object_field
          = new String("final object field");

  public final static int final_static_int_field = 12345;
  public final int final_int_field = 67890;

  public static boolean static_bool_field;
  public static char static_char_field;
  public static byte static_byte_field;
  public static short static_short_field;
  public static int static_int_field;
  public static long static_long_field;
  public static float static_float_field;
  public static double static_double_field;
  public static Object static_object_field;

  public static boolean getStaticBool()
  {
    return static_bool_field;
  }

  public static char getStaticChar()
  {
    return static_char_field;
  }

  public static byte getStaticByte()
  {
    return static_byte_field;
  }

  public static short getStaticShort()
  {
    return static_short_field;
  }

  public static int getStaticInt()
  {
    return static_int_field;
  }

  public static long getStaticLong()
  {
    return static_long_field;
  }

  public static float getStaticFloat()
  {
    return static_float_field;
  }

  public static double getStaticDouble()
  {
    return static_double_field;
  }

  public static Object getStaticObject()
  {
    return static_object_field;
  }

  public static void setStaticBool(boolean i)
  {
    static_bool_field = i;
  }

  public static void setStaticChar(char i)
  {
    static_char_field = i;
  }

  public static void setStaticByte(byte i)
  {
    static_byte_field = i;
  }

  public static void setStaticShort(short i)
  {
    static_short_field = i;
  }

  public static void setStaticInt(int i)
  {
    static_int_field = i;
  }

  public static void setStaticLong(long i)
  {
    static_long_field = i;
  }

  public static void setStaticFloat(float i)
  {
    static_float_field = i;
  }

  public static void setStaticDouble(double i)
  {
    static_double_field = i;
  }

  public static void setStaticObject(Object i)
  {
    static_object_field = i;
  }

  public boolean bool_field;
  public char char_field;
  public byte byte_field;
  public short short_field;
  public int int_field;
  public long long_field;
  public float float_field;
  public double double_field;
  public Object object_field;

  public boolean getBool()
  {
    return bool_field;
  }

  public char getChar()
  {
    return char_field;
  }

  public byte getByte()
  {
    return byte_field;
  }

  public short getShort()
  {
    return short_field;
  }

  public int getInt()
  {
    return int_field;
  }

  public long getLong()
  {
    return long_field;
  }

  public float getFloat()
  {
    return float_field;
  }

  public double getDouble()
  {
    return double_field;
  }

  public Object getObject()
  {
    return object_field;
  }

  public void setBool(boolean i)
  {
    bool_field = i;
  }

  public void setChar(char i)
  {
    char_field = i;
  }

  public void setByte(byte i)
  {
    byte_field = i;
  }

  public void setShort(short i)
  {
    short_field = i;
  }

  public void setInt(int i)
  {
    int_field = i;
  }

  public void setLong(long i)
  {
    long_field = i;
  }

  public void setFloat(float i)
  {
    float_field = i;
  }

  public void setDouble(double i)
  {
    double_field = i;
  }

  public void setObject(Object i)
  {
    object_field = i;
  }

  public static boolean throwStaticBool()
  {
    throw new RuntimeException("bool");
  }

  public static char throwStaticChar()
  {
    throw new RuntimeException("char");
  }

  public static byte throwStaticByte()
  {
    throw new RuntimeException("byte");
  }

  public static short throwStaticShort()
  {
    throw new RuntimeException("short");
  }

  public static int throwStaticInt()
  {
    throw new RuntimeException("int");
  }

  public static long throwStaticLong()
  {
    throw new RuntimeException("long");
  }

  public static float throwStaticFloat()
  {
    throw new RuntimeException("float");
  }

  public static double throwStaticDouble()
  {
    throw new RuntimeException("double");
  }

  public static Object throwStaticObject()
  {
    throw new RuntimeException("object");
  }

  public boolean throwBool()
  {
    throw new RuntimeException("bool");
  }

  public char throwChar()
  {
    throw new RuntimeException("char");
  }

  public byte throwByte()
  {
    throw new RuntimeException("byte");
  }

  public short throwShort()
  {
    throw new RuntimeException("short");
  }

  public int throwInt()
  {
    throw new RuntimeException("int");
  }

  public long throwLong()
  {
    throw new RuntimeException("long");
  }

  public float throwFloat()
  {
    throw new RuntimeException("float");
  }

  public double throwDouble()
  {
    throw new RuntimeException("double");
  }

  public Object throwObject()
  {
    throw new RuntimeException("object");
  }

  public boolean callBoolean(boolean i)
  {
    return i;
  }

  public byte callByte(byte i)
  {
    return i;
  }

  public char callChar(char i)
  {
    return i;
  }

  public short callShort(short i)
  {
    return i;
  }

  public int callInt(int i)
  {
    return i;
  }

  public long callLong(long i)
  {
    return i;
  }

  public float callFloat(float i)
  {
    return i;
  }

  public double callDouble(double i)
  {
    return i;
  }

  public String callString(String i)
  {
    return i;
  }

  public java.lang.Boolean callBoxedBoolean(java.lang.Boolean i)
  {
    return i;
  }

  public java.lang.Byte callBoxedByte(java.lang.Byte i)
  {
    return i;
  }

  public java.lang.Character callBoxedChar(java.lang.Character i)
  {
    return i;
  }

  public java.lang.Short callBoxedShort(java.lang.Short i)
  {
    return i;
  }

  public java.lang.Integer callBoxedInt(java.lang.Integer i)
  {
    return i;
  }

  public java.lang.Long callBoxedLong(java.lang.Long i)
  {
    return i;
  }

  public java.lang.Float callBoxedFloat(java.lang.Float i)
  {
    return i;
  }

  public java.lang.Double callBoxedDouble(java.lang.Double i)
  {
    return i;
  }

  public static boolean callStaticBoolean(boolean i)
  {
    return i;
  }

  public static byte callStaticByte(byte i)
  {
    return i;
  }

  public static char callStaticChar(char i)
  {
    return i;
  }

  public static short callStaticShort(short i)
  {
    return i;
  }

  public static int callStaticInt(int i)
  {
    return i;
  }

  public static long callStaticLong(long i)
  {
    return i;
  }

  public static float callStaticFloat(float i)
  {
    return i;
  }

  public static double callStaticDouble(double i)
  {
    return i;
  }

  public static String callStaticString(String i)
  {
    return i;
  }

  public static Object callStaticObject(Object i)
  {
    return i;
  }

  private static Object callPrivateStaticObject(Object i)
  {
    return i;
  }

  public Object callObject(Object i)
  {
    return i;
  }

  private Object callPrivateObject(Object i)
  {
    return i;
  }

  protected Object callProtectedObject(Object i)
  {
    return i;
  }

  public static Number callNumber(Number n)
  {
    return n;
  }

  public Object callSupplier(java.util.function.Supplier s)
  {
    return s.get();
  }

  public Object callCharArray(char[] c)
  {
    return c;
  }

  public Object callByteArray(byte[] c)
  {
    return c;
  }

  public Object callClass(Class c)
  {
    return c;
  }

  public Iterable callIterable(Iterable c)
  {
    return c;
  }

}
