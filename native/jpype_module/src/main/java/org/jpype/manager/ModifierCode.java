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

import java.lang.reflect.Modifier;
import java.util.EnumSet;

/**
 * Definitions for JPype modifiers.
 * <p>
 * These pretty much match Java plus a few codes we need.
 *
 * @author nelson85
 */
public enum ModifierCode
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
  VARARGS(0x0080),
  ENUM(0x4000),
  ABSTRACT(0x0400),
  // Special flags for classes required for JPype
  SPECIAL(0x00010000),
  THROWABLE(0x00020000),
  SERIALIZABLE(0x00040000),
  ANONYMOUS(0x00080000),
  FUNCTIONAL(0x00100000),
  CALLER_SENSITIVE(0x00200000),
  PRIMITIVE_ARRAY(0x00400000),
  COMPARABLE(0x00800000),
  BUFFER(0x01000000),
  CTOR(0x10000000),
  BEAN_ACCESSOR(0x20000000),
  BEAN_MUTATOR(0x40000000);
  final public int value;

  ModifierCode(int value)
  {
    this.value = value;
  }

  public static int get(EnumSet<ModifierCode> set)
  {
    int out = 0;
    for (ModifierCode m : set)
    {
      out |= m.value;
    }
    return out;
  }

  public static EnumSet<ModifierCode> decode(long modifiers)
  {
    EnumSet<ModifierCode> out = EnumSet.noneOf(ModifierCode.class);
    for (ModifierCode code : ModifierCode.values())
    {
      if ((modifiers & code.value) == code.value)
        out.add(code);
    }
    return out;
  }
}
