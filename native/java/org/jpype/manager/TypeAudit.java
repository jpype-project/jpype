/*
 *    Copyright 2019 Karl Einar Nelson
 *   
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *  
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
package org.jpype.manager;

import java.lang.reflect.Method;

/**
 * Auditing class for TypeManager used during testing.
 * 
 * This is not used during operation.
 *
 * @author nelson85
 */
public interface TypeAudit
{
  void dump(ClassDescriptor desc);

  void verifyMembers(ClassDescriptor desc);

  public void failFindMethod(ClassDescriptor desc, Method method);
}