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

import java.lang.reflect.Modifier;
import java.util.EnumSet;

/**
 * Definitions for JPype modifiers.
 * 
 * These pretty much match Java plus a few codes we need.
 * 
 * @author nelson85
 */
public enum ModifierCodes
{
  // we need 
  //   fields: static, final
  //   methods: static, final, varargs, constructor
  //   class: interface, throwable, abstract, final  
  PUBLIC(Modifier.PUBLIC),
  PRIVATE(Modifier.PRIVATE),
  PROTECTED(Modifier.PROTECTED),
  STATIC(Modifier.STATIC),
  FINAL(Modifier.FINAL),
  VARARGS(128),
  ENUM(16384),
  
  // Special flags required for JPype
  SPECIAL(0x08000000),
  CTOR(0x10000000),
  THROWABLE(0x20000000),
  ABSTRACT(0x40000000);
  
  final public int value;
  ModifierCodes(int value)
  {
    this.value = value;
  }

  public int get(EnumSet<ModifierCodes> set)
  {
    int out = 0;
    for (ModifierCodes m : set)
    {
      out |= m.value;
    }
    return out;
  }
}
