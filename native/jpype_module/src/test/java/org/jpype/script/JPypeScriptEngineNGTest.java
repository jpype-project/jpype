// --- file: org/jpype/script/JPypeScriptEngineNGTest.java ---
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
package org.jpype.script;

import javax.script.Invocable;
import javax.script.ScriptEngine;
import javax.script.ScriptEngineManager;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertNotNull;
import static org.testng.Assert.assertTrue;

/**
 * Ports the shape of jpy's {@code Jsr223Test} (plan/JSR223.md): engine
 * discovery via {@link ScriptEngineManager}, expression vs. statement
 * {@code eval}, a bindings round-trip in both directions, and
 * {@link Invocable#invokeFunction}.
 */
public class JPypeScriptEngineNGTest extends PyTestHarness
{

  private static ScriptEngine sharedEngine;

  private ScriptEngine engine()
  {
    if (sharedEngine == null)
    {
      ScriptEngineManager manager = new ScriptEngineManager();
      sharedEngine = manager.getEngineByName("python");
      assertNotNull(sharedEngine, "JPypeScriptEngineFactory was not discovered via ServiceLoader");
      assertTrue(sharedEngine instanceof JPypeScriptEngine);
    }
    return sharedEngine;
  }

  @Test
  public void testEngineDiscovery()
  {
    engine();
  }

  @Test
  public void testEvalExpressionReturnsValue() throws Exception
  {
    Object result = engine().eval("1 + 1");
    assertNotNull(result);
    assertEquals(result.toString(), "2");
  }

  @Test
  public void testEvalStatementFallsBackToExec() throws Exception
  {
    ScriptEngine engine = engine();
    Object result = engine.eval("x = 1 + 1");
    assertEquals(result, null);
    assertEquals(engine.get("x").toString(), "2");
  }

  @Test
  public void testBindingsRoundTrip() throws Exception
  {
    ScriptEngine engine = engine();

    // Python defines a variable; Java reads it back via Bindings.
    engine.eval("answer = 40 + 2");
    Object answer = engine.get("answer");
    assertNotNull(answer);
    assertEquals(answer.toString(), "42");

    // Java puts that value back under a new name; Python reads it.
    engine.put("carried", answer);
    Object result = engine.eval("carried + 1");
    assertEquals(result.toString(), "43");
  }

  @Test
  public void testInvokeFunction() throws Exception
  {
    ScriptEngine engine = engine();
    engine.eval("def square(n):\n    return n * n\n");
    Object result = ((Invocable) engine).invokeFunction("square", 6);
    assertEquals(result.toString(), "36");
  }

  @Test(expectedExceptions = NoSuchMethodException.class)
  public void testInvokeUndefinedFunctionThrows() throws Exception
  {
    ((Invocable) engine()).invokeFunction("does_not_exist");
  }
}
