/*
 * Copyright 2018, Karl Nelson
 *
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
 */
package org.jpype;

import java.lang.reflect.Proxy;
import org.jpype.proxy.JPypeInvocationHandler;

/**
 *
 * @author Karl Einar Nelson
 */
public class Utility
{
  /**
   * Get the class for an object.
   * <p>
   * This method is somewhat misplaced, but we don't have a good place for it in
   * JPype 0.7. This prevents generation of pointless wrappers for lambda and
   * other synthetic classes. This functionality moves to TypeManager in 0.8.
   *
   * @param obj is the object to probe.
   * @return the class to use for JPype
   */
  public static Class<?> getClassFor(Object obj)
  {
    Class cls = obj.getClass();
    if (Proxy.isProxyClass(cls) && (Proxy.getInvocationHandler(obj) instanceof org.jpype.proxy.JPypeInvocationHandler))
    {
      return JPypeInvocationHandler.class;
    }

    if (cls.isSynthetic())
    {
      // We may be a lambda or a Proxy.

      // We are a lambda (Lambda can only have one interface.
      return cls.getInterfaces()[0];
    }
    return cls;
  }
}
