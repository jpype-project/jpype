// --- file: org/jpype/manager/ModifierCodeNGTest.java ---
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
package org.jpype.manager;

import java.lang.reflect.Modifier;
import java.util.EnumSet;
import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

/**
 * Pure bitflag encode/decode round-trip for {@code ModifierCode}, no
 * bridge needed.
 */
public class ModifierCodeNGTest
{

  @Test
  public void testGetEncodesEnumSet()
  {
    int encoded = ModifierCode.get(EnumSet.of(ModifierCode.PUBLIC, ModifierCode.STATIC, ModifierCode.FINAL));
    assertEquals(encoded, Modifier.PUBLIC | Modifier.STATIC | Modifier.FINAL);
  }

  @Test
  public void testDecodeRoundTripsGet()
  {
    EnumSet<ModifierCode> original = EnumSet.of(ModifierCode.PRIVATE, ModifierCode.ABSTRACT, ModifierCode.SPECIAL);
    int encoded = ModifierCode.get(original);
    EnumSet<ModifierCode> decoded = ModifierCode.decode(encoded);
    assertEquals(decoded, original);
  }

  @Test
  public void testDecodeEmpty()
  {
    assertTrue(ModifierCode.decode(0).isEmpty());
  }

  @Test
  public void testDecodeAllSpecialFlags()
  {
    EnumSet<ModifierCode> original = EnumSet.of(
            ModifierCode.THROWABLE, ModifierCode.SERIALIZABLE, ModifierCode.ANONYMOUS,
            ModifierCode.FUNCTIONAL, ModifierCode.CALLER_SENSITIVE, ModifierCode.PRIMITIVE_ARRAY,
            ModifierCode.COMPARABLE, ModifierCode.BUFFER, ModifierCode.PYTHON, ModifierCode.PROXY,
            ModifierCode.CTOR, ModifierCode.BEAN_ACCESSOR, ModifierCode.BEAN_MUTATOR, ModifierCode.VARARGS,
            ModifierCode.ENUM);
    int encoded = ModifierCode.get(original);
    assertEquals(ModifierCode.decode(encoded), original);
  }
}
