// --- file: org/jpype/internal/Functional.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 * 
 *  See NOTICE file for details.
 */
package org.jpype.internal;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandleProxies;
import java.lang.invoke.MethodHandles;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Arrays;
import java.util.function.Predicate;

/**
 *
 * @author nelson85
 */
@SuppressWarnings("unchecked")
public class Functional
{

  // a functional interface can only re-declare a public non-final method from Object
  // this should end up being an array of equals, hashCode and toString
  private static final Method[] OBJECT_METHODS
          = Arrays.stream(Object.class.getMethods())
                  .filter(m -> !Modifier.isFinal(m.getModifiers()))
                  .toArray(Method[]::new);

  private static final Predicate<Class<?>> isSealed;

  static
  {
    Predicate<Class<?>> result = null;
    try
    {
      Method m = Class.class.getMethod("isSealed");
      MethodHandle handle = MethodHandles.publicLookup().unreflect(m);
      result = MethodHandleProxies.asInterfaceInstance(Predicate.class, handle);
    } catch (IllegalAccessException e)
    {
      // it's a public method so this should never occur
      throw new IllegalAccessError(e.getMessage());
    } catch (NoSuchMethodException e)
    {
      // if isSealed doesn't exist then neither do sealed classes
      result = c -> false;
    }
    isSealed = result;
  }

  private static boolean equals(Method a, Method b)
  {
    // this should be the fastest possible short circuit
    if (a.getParameterCount() != b.getParameterCount())
      return false;
    if (!a.getName().equals(b.getName()))
      return false;
    // if the return types are different it wouldn't compile
    // parameters must be exactly the same and may not be an extended class
    if (!Arrays.equals(a.getParameterTypes(), b.getParameterTypes()))
      return false;
    // if declared exceptions were different it wouldn't compile
    // if it did compile then it is an override
    return true;
  }

  public static Method getFunctionalInterfaceMethod(Class<?> cls)
  {
    if (!cls.isInterface() || cls.isAnnotation() || isSealed.test(cls))
      return null;
    Method result = null;
    for (Method m : cls.getMethods())
    {
      if (Modifier.isAbstract(m.getModifiers()))
      {
        if (isObjectMethodOverride(m))
          continue;
        if (result != null && !equals(m, result))
          return null;
        if (result == null || cls.equals(m.getDeclaringClass()))
          result = m;
      }
    }
    return result;
  }

  private static boolean isObjectMethodOverride(Method m)
  {
    for (Method objectMethod : OBJECT_METHODS)
    {
      if (equals(m, objectMethod))
        return true;
    }
    return false;
  }

}
