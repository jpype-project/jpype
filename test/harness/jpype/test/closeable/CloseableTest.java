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
package jpype.closeable;

class CloseableTest implements java.io.Closeable
{

  public static boolean closed = false;
  public static String printed = "";
  public static boolean willfail = false;
  public static boolean failed = false;

  public static void reset()
  {
    closed = false;
    willfail = false;
    failed = false;
    printed = "";
  }

  public CloseableTest()
  {
  }

  public void print(String value)
  {
    printed = value;
  }

  public void throwException()
  {
    throw new RuntimeException("oh no!");
  }

  public void close() throws java.io.IOException
  {
    closed = true;
    if (willfail)
    {
      failed = true;
      throw new java.io.IOException("oh my?");
    }
  }

}
