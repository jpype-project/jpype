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
package jpype.badinittop;

// Static initializer always fails.  Each test uses its own class and package
// because a failed initializer is permanent and packages are shared.
public class BadInitA
{

  static
  {
    if (Boolean.TRUE)
      throw new RuntimeException("BadInitA initializer failed");
  }

  public static int value = 1;
}
