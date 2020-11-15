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

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;

/**
 *
 * @author nelson85
 */
public class Factory
{
  public static ClassDecl newClass(String name, Class[] bases)
  {
    return new ClassDecl(name, bases);
  }

  public static Class createClass(Class[] bases, String[] overrides)
  {
    HashSet<String> hash = new HashSet<>(Arrays.asList(overrides));
    Class base = null;
    List<Class> interfaces = new ArrayList<>();
    List<Method> methods = new ArrayList<>();

    for (Class cls : bases)
    {
      if (cls.isInterface())
      {
        interfaces.add(cls);
        continue;
      }
      
      // There can only be one base
      if (base != null)
        throw new RuntimeException("Multiple bases not allowed");
      
      // Base must not be final
      if (Modifier.isFinal(base.getModifiers()))
        throw new RuntimeException("Cannot extend final class");
      
      // Select this as the base
      base = cls;
    }

    if (base == null)
    {
      base = Object.class;
    }

    for (Class i : bases)
    {
      for (Method m : i.getMethods())
      {
        if (Modifier.isAbstract(m.getModifiers()))
        {
          if (!hash.contains(m.getName()))
          {
            throw new RuntimeException("Method " + m + " must be overriden");
          }
          methods.add(m);
        }
      }
    }
    
    return null;
  }

  /** Hook to create a new instance of the object.
   * 
   * This is called by the ctor of object to invoke __init__(self)
   */
  static native long _create(long context, long self );

  //
  static native void _call(long context, String name, long pyObject,
          long returnType, long[] argsTypes, Object[] args);

  static native long _getDict(long context, long self);
}

// FIXME how do we define what arguments are passed to the ctor?
// FIXME how do we define ctors
// FIXME how do we get to TypeError rather that RuntimeException?
// FIXME how do we keep from clobbering.
// FIXME how does the type system know that this is an extension class?
