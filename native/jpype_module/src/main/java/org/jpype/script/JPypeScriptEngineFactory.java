// --- file: org/jpype/script/JPypeScriptEngineFactory.java ---
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

import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import javax.script.ScriptEngine;
import javax.script.ScriptEngineFactory;

/**
 * JSR-223 {@link ScriptEngineFactory} for JPype's embedded Python
 * interpreter. Registered via {@code META-INF/services/javax.script.ScriptEngineFactory}
 * (and the module's own {@code provides} declaration) so
 * {@code new ScriptEngineManager().getEngineByName("python")} discovers it
 * with no further setup.
 */
public class JPypeScriptEngineFactory implements ScriptEngineFactory
{

  private final Map<String, Object> parameters;

  public JPypeScriptEngineFactory()
  {
    parameters = new HashMap<>();
    parameters.put(ScriptEngine.NAME, getNames().get(0));
    parameters.put(ScriptEngine.ENGINE, getEngineName());
    parameters.put(ScriptEngine.ENGINE_VERSION, getEngineVersion());
    parameters.put(ScriptEngine.LANGUAGE, getLanguageName());
    parameters.put(ScriptEngine.LANGUAGE_VERSION, getLanguageVersion());
  }

  @Override
  public String getEngineName()
  {
    return "JPype Python Engine";
  }

  @Override
  public String getEngineVersion()
  {
    return "1.0";
  }

  @Override
  public List<String> getExtensions()
  {
    return Collections.unmodifiableList(Arrays.asList("py"));
  }

  @Override
  public List<String> getMimeTypes()
  {
    return Collections.unmodifiableList(Arrays.asList(
            "text/x-python", "text/python", "application/x-python"));
  }

  @Override
  public List<String> getNames()
  {
    return Collections.unmodifiableList(Arrays.asList("python", "jpype", "cpython"));
  }

  @Override
  public String getLanguageName()
  {
    return "python";
  }

  @Override
  public String getLanguageVersion()
  {
    return System.getProperty("python.config.python_version", "3.x");
  }

  @Override
  public Object getParameter(String key)
  {
    return parameters.get(key);
  }

  @Override
  public String getMethodCallSyntax(String obj, String m, String... args)
  {
    return String.format("%s.%s(%s)", obj, m, String.join(", ", args));
  }

  @Override
  public String getOutputStatement(String toDisplay)
  {
    return "print(" + toDisplay + ")";
  }

  @Override
  public String getProgram(String... statements)
  {
    return String.join("\n", statements);
  }

  @Override
  public ScriptEngine getScriptEngine()
  {
    return new JPypeScriptEngine(this);
  }
}
