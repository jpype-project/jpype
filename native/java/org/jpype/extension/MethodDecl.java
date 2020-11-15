/** ***************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * See NOTICE file for details.
 **************************************************************************** */
package org.jpype.extension;

/**
 *
 * @author nelson85
 */
public class MethodDecl
{

  final String name;
  final Class ret;
  final Class[] args;
  final Class[] exceptions;
  final int modifiers;
  
  public MethodDecl(String name, Class ret, Class[] args, Class[] exceptions, int modifiers)
  {
    this.name = name;
    this.ret = ret;
    this.args = args;
    this.exceptions = exceptions;
    this.modifiers = modifiers;
  }
}
