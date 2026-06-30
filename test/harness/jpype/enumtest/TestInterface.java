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
 * Test class with a method that takes an enum parameter
 * Mimics the InterfaceImpl.setRole(InterfaceRoleType) from bug report
 */
public class TestInterface {
    private RoleType role;

    public void setRole(RoleType newRole) {
        this.role = newRole;
    }

    public RoleType getRole() {
        return role;
    }

    public String getRoleAsString() {
        return role != null ? role.toString() : "null";
    }
}
