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
package org.jpype.manager;

import java.lang.reflect.Executable;
import java.lang.reflect.Method;

/**
 * A list of resources associated with this class.
 * <p>
 * These can be accessed within JPype using the org.jpype.manager.TypeManager.
 * <p>
 */
public class ClassDescriptor
{

  public Class<?> cls;
  /**
   * JPClass pointer for this class.
   */
  public long classPtr;
  /**
   * JPMethodDispatch for the constructor.
   */
  public long constructorDispatch;
  public long[] constructors;
  /**
   * Resources needed by the class
   */
  public long[] methodDispatch;
  public Executable[] methodIndex;
  public long[] methods;
  public int methodCounter = 0;
  public long[] fields;
  public long anonymous;
  public int functional_interface_parameter_count;

  ClassDescriptor(Class cls, long classPtr, Method method)
  {
    this.cls = cls;
    this.classPtr = classPtr;
    if (this.classPtr == 0)
      throw new NullPointerException("Class pointer is null for " + cls);
    if (method != null)
      functional_interface_parameter_count = method.getParameterCount();
    else
      functional_interface_parameter_count = -1;
  }

  long getMethod(Method requestedMethod)
  {
    for (int i = 0; i < methods.length; ++i)
      if (this.methodIndex[i].equals(requestedMethod))
        return this.methods[i];
    return 0;

  }
}
