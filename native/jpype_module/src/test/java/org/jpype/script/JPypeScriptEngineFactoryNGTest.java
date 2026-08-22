// --- file: org/jpype/script/JPypeScriptEngineFactoryNGTest.java ---
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

import javax.script.ScriptEngine;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Every getter here is plain, deterministic Java with no interpreter
 * dependency, so it's tested directly without going through the bridge
 * (see python.lang.PyTestHarness-based JPypeScriptEngineNGTest for the
 * one method, getScriptEngine(), that does need it).
 */
public class JPypeScriptEngineFactoryNGTest
{

  private final JPypeScriptEngineFactory factory = new JPypeScriptEngineFactory();

  @Test
  public void testEngineName()
  {
    assertEquals(factory.getEngineName(), "JPype Python Engine");
  }

  @Test
  public void testEngineVersion()
  {
    assertEquals(factory.getEngineVersion(), "1.0");
  }

  @Test
  public void testExtensions()
  {
    assertEquals(factory.getExtensions(), java.util.Arrays.asList("py"));
  }

  @Test
  public void testMimeTypes()
  {
    assertTrue(factory.getMimeTypes().contains("text/x-python"));
  }

  @Test
  public void testNames()
  {
    assertEquals(factory.getNames(), java.util.Arrays.asList("python", "jpype", "cpython"));
  }

  @Test
  public void testLanguageName()
  {
    assertEquals(factory.getLanguageName(), "python");
  }

  @Test
  public void testLanguageVersion()
  {
    assertNotNull(factory.getLanguageVersion());
  }

  @Test
  public void testGetParameter()
  {
    assertEquals(factory.getParameter(ScriptEngine.NAME), "python");
    assertEquals(factory.getParameter(ScriptEngine.ENGINE), "JPype Python Engine");
    assertEquals(factory.getParameter(ScriptEngine.ENGINE_VERSION), "1.0");
    assertEquals(factory.getParameter(ScriptEngine.LANGUAGE), "python");
    assertEquals(factory.getParameter(ScriptEngine.LANGUAGE_VERSION), factory.getLanguageVersion());
    assertNull(factory.getParameter("not-a-real-key"));
  }

  @Test
  public void testGetMethodCallSyntax()
  {
    assertEquals(factory.getMethodCallSyntax("obj", "method", "a", "b"), "obj.method(a, b)");
    assertEquals(factory.getMethodCallSyntax("obj", "method"), "obj.method()");
  }

  @Test
  public void testGetOutputStatement()
  {
    assertEquals(factory.getOutputStatement("'hi'"), "print('hi')");
  }

  @Test
  public void testGetProgram()
  {
    assertEquals(factory.getProgram("a = 1", "b = 2"), "a = 1\nb = 2");
  }

}
