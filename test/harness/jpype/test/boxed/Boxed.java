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
package jpype.boxed;

public class Boxed
{

  // Call each type
  static public boolean callBoolean(boolean i)
  {
    return i;
  }

  static public byte callByte(byte i)
  {
    return i;
  }

  static public char callChar(char i)
  {
    return i;
  }

  static public short callShort(short i)
  {
    return i;
  }

  static public int callInteger(int i)
  {
    return i;
  }

  static public long callLong(long i)
  {
    return i;
  }

  static public float callFloat(float i)
  {
    return i;
  }

  static public double callDouble(double i)
  {
    return i;
  }

  // Create a boxed type
  static public Short newShort(short i)
  {
    return i;
  }

  static public Integer newInteger(int i)
  {
    return i;
  }

  static public Long newLong(long i)
  {
    return i;
  }

  static public Float newFloat(float i)
  {
    return i;
  }

  static public Double newDouble(double i)
  {
    return i;
  }

  // Check which is called
  static public int whichShort(short i)
  {
    return 1;
  }

  static public int whichShort(Short i)
  {
    return 2;
  }

  static public int whichInteger(int i)
  {
    return 1;
  }

  static public int whichInteger(Integer i)
  {
    return 2;
  }

  static public int whichLong(long i)
  {
    return 1;
  }

  static public int whichLong(Long i)
  {
    return 2;
  }

  static public int whichFloat(float i)
  {
    return 1;
  }

  static public int whichFloat(Float i)
  {
    return 2;
  }

  static public int whichDouble(double i)
  {
    return 1;
  }

  static public int whichDouble(Double i)
  {
    return 2;
  }
}
