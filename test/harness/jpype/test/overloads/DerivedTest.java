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

/** Test to see that using a derived type rather than a base class will 
 * not alter the method resolution process.
 */
public class DerivedTest
{
	public static class Base {}
        public static class Derived extends Base {}

	public static int testStatic(boolean a, Base b) {
		return 1;
	}

	public static int testStatic(Object a, Base b) {
		return 2;
	}

	public int testMember(boolean a, Base b) {
		return 3;
	}

	public int testMember(Object a, Base b) {
		return 4;
	}

	public static class Sub extends DerivedTest {}

}
