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
import java.lang.reflect.Method;

/**
 * A list of resources associated with this class.
 * 
 * These can be accessed within JPype using the org.jpype.manager.TypeManager.
 * 
 */
public class ClassDescriptor
{
  public Class cls;
  public ClassDescriptor[] parents;
  
  /** JPClass pointer for this class. */
  public long classPtr;
  
  /** JPMethodDispatch for the constructor. */
  public long constructorDispatch;
  public long[] constructors;
  
  /** Resources needed by the class */
  public long[]       methodDispatch;  
  public Executable[] methodIndex;
  public long[]       methods;
  public int          methodCounter = 0;
  public long[]       fields;
  
  ClassDescriptor(Class cls, long classPtr)
  {
    this.cls = cls;
    this.classPtr = classPtr;
  }

  long getMethod(Method method)
  {
    for (int i = 0; i< methods.length; ++i)
      if (this.methodIndex[i] == method)
        return this.methods[i];
    throw new RuntimeException("method note found");
  }
}
