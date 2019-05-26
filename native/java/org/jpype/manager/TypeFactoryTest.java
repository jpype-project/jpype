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

import java.lang.reflect.Executable;
import java.lang.reflect.Field;
import java.util.HashMap;

/**
 * A special version of the TypeFactory for debugging.
 *
 * @author nelson85
 */
public class TypeFactoryTest implements TypeFactory
{
  long value = 0;
  HashMap<Long, Object> resourceMap = new HashMap<>();

  @Override
  public long defineArrayClass(Class cls, long superClass, String name, long componentPtr)
  {
    value++;
    System.out.println("defineArrayClass " + cls.getName() + " " + value);
    resourceMap.put(value, "array class " + cls.getName());
    return value;
  }

  @Override
  public long defineObjectClass(Class cls, long superClass, long[] interfaces, int modifiers, String name)
  {
    value++;
    System.out.println("defineObjectClass " + cls.getName() + " " + value);
    resourceMap.put(value, "object class " + cls.getName());
    return value;
  }
  
  @Override
  public long definePrimitive(int code, Class cls, long boxedPtr)
  {
    //throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    return 0;
  }
  
  @Override
  public void assignMembers(long cls, long ctorMethod, long[] methodList, long[] fieldList)
  {
    System.out.println("assignClass");
  }

  @Override
  public long defineField(long cls, String name, Field field, long fieldType, int modifiers)
  {
    value++;
    System.out.println("defineField "+field.getName()+ " "+value);
    resourceMap.put(value, "field " + field.getName());
    return value;
  }

  @Override
  public long defineMethod(long cls, String name, Executable method, long returnType, long[] argumentTypes, long[] overloadList, int modifiers)
  {
    value++;
    System.out.println("defineMethod "+method.getName()+ " "+value);
    resourceMap.put(value, "method " + method.getName());
    return value;
  }

  @Override
  public long defineMethodDispatch(long cls, String name, long[] overloadList, int modifiers)
  {
    System.out.println("defineMethodDispatch "+name+" "+value);
    value++;
    resourceMap.put(value, "dispatch " + name);
    return value;
  }

  @Override
  public void destroy(long[] resources, int sz)
  {
    for (int i = 0; i < sz; ++i)
    {
      long r = resources[i];
      if (this.resourceMap.containsKey(r))
      {
        System.out.println("destroy " + this.resourceMap.get(r));
        this.resourceMap.remove(r);
        continue;
      }

      throw new RuntimeException("repeat delete " + r);
    }
  }


}
