// --- file: org/jpype/ScriptNGTest.java ---
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
package org.jpype;

import org.testng.annotations.Test;
import python.lang.PyDict;
import python.lang.PyObject;
import python.lang.PyTestHarness;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertSame;

/**
 * Exercises {@code Script} directly. Most of the class is already covered
 * indirectly through {@code PyTestHarness}'s own {@code context} field (a
 * single-arg {@code Script}), but the two-arg constructor (explicit
 * globals/locals) and {@code importModule(module, as)} were previously only
 * reachable through subinterpreter tests, which skip on a native library
 * built against Python &lt; 3.12 (see SubInterpreterNGTest). Neither actually
 * requires a subinterpreter - the main interpreter works just as well for
 * exercising this class's own logic.
 */
public class ScriptNGTest extends PyTestHarness
{

  @Test
  public void testTwoArgConstructorSeparateGlobalsAndLocals()
  {
    PyDict globals = context.getBackend().newDict();
    PyDict locals = context.getBackend().newDict();
    Script script = new Script(MainInterpreter.getInstance(), globals, locals);
    assertSame(script.globals(), globals);
    assertSame(script.locals(), locals);

    script.exec("x = 1");
    assertEquals(locals.get(context.str("x")).toString(), "1");
  }

  @Test
  public void testImportModule()
  {
    PyDict globals = context.getBackend().newDict();
    Script script = new Script(MainInterpreter.getInstance(), globals, globals);
    script.importModule("math");
    assertEquals(script.eval("math.floor(3.7)").toString(), "3");
  }

  @Test
  public void testImportModuleAs()
  {
    PyDict globals = context.getBackend().newDict();
    Script script = new Script(MainInterpreter.getInstance(), globals, globals);
    script.importModule("math", "m");
    assertEquals(script.eval("m.floor(3.7)").toString(), "3");
  }

  @Test
  public void testLocalsReturnsConfiguredLocals()
  {
    PyDict globals = context.getBackend().newDict();
    PyObject locals = context.getBackend().newDict();
    Script script = new Script(MainInterpreter.getInstance(), globals, locals);
    assertSame(script.locals(), locals);
  }
}
