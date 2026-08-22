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

import java.io.StringReader;
import javax.script.Bindings;
import javax.script.Invocable;
import javax.script.ScriptContext;
import javax.script.ScriptEngine;
import javax.script.ScriptEngineManager;
import javax.script.ScriptException;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertNotNull;
import static org.testng.Assert.assertTrue;
import static org.testng.Assert.fail;

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

  @Test
  public void testGetFactory()
  {
    assertTrue(engine().getFactory() instanceof JPypeScriptEngineFactory);
  }

  @Test
  public void testCreateBindings()
  {
    Bindings bindings = engine().createBindings();
    assertNotNull(bindings);
    bindings.put("k", "v");
    assertEquals(bindings.get("k"), "v");
  }

  @Test
  public void testEvalReader() throws Exception
  {
    Object result = engine().eval(new StringReader("3 + 4"));
    assertEquals(result.toString(), "7");
  }

  @Test
  public void testEvalRuntimeErrorWrapsInScriptException()
  {
    try
    {
      engine().eval("this_name_does_not_exist_zzz");
      fail("Expected a ScriptException");
    } catch (ScriptException ex)
    {
      // expected - a real Python runtime error, not the syntax-error
      // fallback-to-exec path.
    }
  }

  @Test
  public void testGlobalScopeIsShadowedByEngineScope() throws Exception
  {
    ScriptEngine engine = engine();
    Bindings globalScope = engine.createBindings();
    globalScope.put("shadow_test_var", "from-global");
    engine.getContext().setBindings(globalScope, ScriptContext.GLOBAL_SCOPE);
    try
    {
      // Not set in ENGINE_SCOPE - global should be visible.
      Object viaGlobal = engine.eval("shadow_test_var");
      assertEquals(viaGlobal.toString(), "from-global");

      // Now shadow it in ENGINE_SCOPE (higher precedence).
      engine.put("shadow_test_var", "from-engine");
      Object viaEngine = engine.eval("shadow_test_var");
      assertEquals(viaEngine.toString(), "from-engine");
    } finally
    {
      engine.getContext().setBindings(engine.createBindings(), ScriptContext.GLOBAL_SCOPE);
    }
  }

  @Test
  public void testToNativeUnwrapsJavaObjectRoundTrip() throws Exception
  {
    java.util.ArrayList<String> javaObj = new java.util.ArrayList<>();
    javaObj.add("marker");
    engine().put("java_round_trip_obj", javaObj);
    Object result = engine().eval("java_round_trip_obj");
    assertTrue(result instanceof java.util.ArrayList);
    assertEquals(result, javaObj);
  }

  @Test
  public void testInvokeMethodOnPyObject() throws Exception
  {
    ScriptEngine engine = engine();
    // The class def is a statement (falls back to exec, no return value) -
    // the instantiation must be its own eval() call to get the instance
    // itself back rather than null.
    engine.eval(
            "class ScriptEngineTestFoo:\n"
            + "    def bar(self, x):\n"
            + "        return x + 1\n");
    Object instance = engine.eval("ScriptEngineTestFoo()");
    Object result = ((Invocable) engine).invokeMethod(instance, "bar", 5);
    assertEquals(result.toString(), "6");
  }

  @Test(expectedExceptions = IllegalArgumentException.class)
  public void testInvokeMethodOnNonPyObjectThrows() throws Exception
  {
    ((Invocable) engine()).invokeMethod("not a PyObject", "anything");
  }

  @Test(expectedExceptions = NoSuchMethodException.class)
  public void testInvokeMethodMissingAttributeThrows() throws Exception
  {
    ScriptEngine engine = engine();
    engine.eval("class ScriptEngineTestEmpty:\n    pass\n");
    Object instance = engine.eval("ScriptEngineTestEmpty()");
    ((Invocable) engine).invokeMethod(instance, "no_such_method");
  }

  public interface Squarer
  {

    Object square(int n);
  }

  @Test
  public void testGetInterfaceOnEngine() throws Exception
  {
    ScriptEngine engine = engine();
    engine.eval("def square(n):\n    return n * n\n");
    Squarer squarer = ((Invocable) engine).getInterface(Squarer.class);
    assertEquals(squarer.square(7).toString(), "49");
  }

  @Test
  public void testGetInterfaceOnObject() throws Exception
  {
    ScriptEngine engine = engine();
    engine.eval(
            "class ScriptEngineTestSquarer:\n"
            + "    def square(self, n):\n"
            + "        return n * n\n");
    Object instance = engine.eval("ScriptEngineTestSquarer()");
    Squarer squarer = ((Invocable) engine).getInterface(instance, Squarer.class);
    assertEquals(squarer.square(8).toString(), "64");
  }

  @Test(expectedExceptions = IllegalArgumentException.class)
  public void testGetInterfaceNonInterfaceThrows() throws Exception
  {
    ((Invocable) engine()).getInterface(String.class);
  }

  @Test(expectedExceptions = IllegalArgumentException.class)
  public void testGetInterfaceOnObjectNonPyObjectThrows() throws Exception
  {
    ((Invocable) engine()).getInterface("not a PyObject", Squarer.class);
  }
}
