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

public class Test2
{
    @FunctionalInterface
    public interface NoArg {
        public String apply();
    }

    @FunctionalInterface
    public interface SingleArg {
        public String apply(int a);
    }

    @FunctionalInterface
    public interface TwoArg {
        public String apply(int a, int b);
    }

    public String testFunctionalInterfaces(NoArg noArg) {
        return "NoArgs";
    }

    public String testFunctionalInterfaces(SingleArg singleArg) {
        return "SingleArg";
    }

    public String testFunctionalInterfaces(TwoArg twoArg) {
        return "TwoArg";
    }
}