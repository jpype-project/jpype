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
package jpype.enumtest;

/**
 * Enum that implements Enumerator interface
 * Mimics Eclipse Modeling Framework generated enums
 */
public enum RoleType implements Enumerator {
    MASTER(0, "Master", "Master"),
    SLAVE(1, "Slave", "Slave");

    private final int value;
    private final String name;
    private final String literal;

    private RoleType(int value, String name, String literal) {
        this.value = value;
        this.name = name;
        this.literal = literal;
    }

    @Override
    public int getValue() {
        return value;
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public String getLiteral() {
        return literal;
    }

    @Override
    public String toString() {
        return literal;
    }
}
