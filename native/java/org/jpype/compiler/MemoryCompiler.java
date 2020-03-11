/* ****************************************************************************
  Copyright 2019, Karl Einar Nelson

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 *****************************************************************************/
package org.jpype.compiler;

import java.util.Arrays;
import java.util.List;
import javax.tools.DiagnosticCollector;
import javax.tools.JavaCompiler;
import javax.tools.JavaFileManager;
import javax.tools.JavaFileObject;
import javax.tools.ToolProvider;
import org.jpype.classloader.JPypeClassLoader;

/**
 * Simple in memory compiler front end for the JavaCompiler API.
 */
public class MemoryCompiler
{
  public DiagnosticCollector<JavaFileObject> diagnostics;

  /**
   * Compile code from memory into a class.
   * <p>
   * Diagnostics contains the reports of any errors in the class.
   *
   * @param name is the name of the class to compile
   * @param source is the class source
   * @return the created class or null on a failure.
   */
  public Class<?> compile(String name, String source)
  {
    try
    {
      // Create a new source code object
      List<MemorySourceObject> sources = Arrays.asList(
              new MemorySourceObject(name, source));

      JavaCompiler javac = ToolProvider.getSystemJavaCompiler();
      this.diagnostics = new DiagnosticCollector<>();

      // We need a standard file manager to serve as our base.
      JavaFileManager stdFileManager = javac.getStandardFileManager(null, null, null);

      // Create our own file manager for collecting the resulting byte code
      MemoryFileManager memoryFileManager = new MemoryFileManager(stdFileManager);

      // Create a compiler
      JavaCompiler.CompilationTask task = javac.getTask(null, memoryFileManager, null, null, null, sources);
      boolean success = task.call();
      if (!success)
      {
        return null;
      }

      // Collect up all the code objects and transfer them to the dynamic class loader.
      JPypeClassLoader cl = JPypeClassLoader.getInstance();
      for (MemoryCodeObject code : memoryFileManager.getCode())
      {
        cl.importClass(code.getName(), code.toByteArray());
      }

      // Retreive the resulting class from the dynamic loader.
      return cl.loadClass(name);
    } catch (Exception ex)
    {
      throw new RuntimeException(ex);
    }
  }

  /**
   * Get the diagnostics for the last code compiled.
   *
   * @return the diagnostics collector from the run.
   */
  public DiagnosticCollector<JavaFileObject> getDiagnostics()
  {
    return diagnostics;
  }
}
