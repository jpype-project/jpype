// --- file: org/jpype/Runner.java ---
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

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Supplier;
import python.lang.PyBuiltIn;
import python.lang.PyDict;
import python.lang.PyObject;
import python.lang.PyString;

/**
 * Runs Python code the way the real {@code python} command line does: as a
 * module ({@code -m}), a script file, or an inline command ({@code -c}).
 *
 * Module and file execution are built on {@code runpy}, the same machinery
 * the real {@code python -m}/script modes use themselves, so {@code sys.path}
 * handling, {@code __main__} naming, and package/zipapp support all match
 * real Python behavior for free.
 *
 * Constructed from an {@link Interpreter}, so it works against a
 * {@link SubInterpreter} as well as {@link MainInterpreter}.
 *
 * Each call temporarily replaces {@code sys.argv} for the duration of the
 * run and always restores the previous value afterward, success or
 * exception. A real {@code python} process never needs to do this because
 * the process exits right after; an {@code Interpreter} is long-lived, so
 * leaving {@code sys.argv} mutated after a one-off call would be a footgun
 * for whatever code keeps using it afterward.
 *
 * A run that raises surfaces like any other {@code Script.exec}/{@code eval}
 * failure - as the matching {@code python.exceptions.*} type. In particular,
 * {@code SystemExit} (as raised by e.g. {@code pip} on completion) surfaces
 * as {@code python.exceptions.PySystemExit}, whose {@link
 * python.exceptions.PyBaseException#get()} gives access to the underlying
 * exception, including its {@code code} attribute - the same translation
 * every other exception already gets, nothing Runner-specific.
 */
public class Runner
{
  private final Interpreter interpreter;

  public Runner(Interpreter interpreter)
  {
    this.interpreter = interpreter;
  }

  /**
   * Equivalent to {@code python -m <module> <args...>}.
   *
   * @param module the module name, e.g. {@code "pip"}.
   * @param args arguments made available as {@code sys.argv[1:]}.
   * @return the executed module's global namespace, as returned by
   * {@code runpy.run_module}.
   */
  public PyDict runModule(String module, String... args)
  {
    PyBuiltIn b = interpreter.getBuiltIn();
    PyDict scope = b.getBackend().newDict();
    scope.putAny("_jp_target", b.str(module));
    return (PyDict) withArgv(module, args, scope, () -> {
      b.exec("import runpy as _jp_runpy", scope, scope);
      b.exec("_jp_result = _jp_runpy.run_module("
              + "_jp_target, run_name='__main__', alter_sys=True)", scope, scope);
      return b.eval("_jp_result", scope, scope);
    });
  }

  /**
   * Equivalent to {@code python <script> <args...>}.
   *
   * @param script the script file to run.
   * @param args arguments made available as {@code sys.argv[1:]}.
   * @return the executed script's global namespace, as returned by
   * {@code runpy.run_path}.
   */
  public PyDict runFile(Path script, String... args)
  {
    PyBuiltIn b = interpreter.getBuiltIn();
    PyDict scope = b.getBackend().newDict();
    String target = script.toString();
    scope.putAny("_jp_target", b.str(target));
    return (PyDict) withArgv(target, args, scope, () -> {
      b.exec("import runpy as _jp_runpy", scope, scope);
      b.exec("_jp_result = _jp_runpy.run_path(_jp_target, run_name='__main__')", scope, scope);
      return b.eval("_jp_result", scope, scope);
    });
  }

  /**
   * Equivalent to {@code python -c <command> <args...>}.
   *
   * @param command the inline Python source to execute.
   * @param args arguments made available as {@code sys.argv[1:]}.
   * @return the namespace the command executed in (its {@code __main__}-style
   * globals), so the caller can retrieve anything the command defined.
   */
  public PyDict runCommand(String command, String... args)
  {
    PyBuiltIn b = interpreter.getBuiltIn();
    PyDict scope = b.getBackend().newDict();
    scope.putAny("_jp_source", b.str(command));
    scope.putAny("__name__", b.str("__main__"));
    return (PyDict) withArgv("-c", args, scope, () -> {
      b.exec("exec(_jp_source)", scope, scope);
      return scope;
    });
  }

  /**
   * Equivalent to {@code python -m pip <pipArgs...>}, e.g.
   * {@code pipInstall("install", "somepkg")} or
   * {@code pipInstall("uninstall", "somepkg")}. Deliberately doesn't
   * hardcode a subcommand - pip's own subcommand is just the first
   * argument, same as on the real command line.
   *
   * @param pipArgs the full pip subcommand and its arguments.
   */
  public void pipInstall(String... pipArgs)
  {
    runModule("pip", pipArgs);
  }

  /**
   * Sets {@code sys.argv} to {@code [argv0] + args} for the duration of
   * {@code body}, restoring the previous {@code sys.argv} afterward
   * regardless of whether {@code body} completes or throws.
   */
  private PyObject withArgv(String argv0, String[] args, PyDict scope, Supplier<PyObject> body)
  {
    PyBuiltIn b = interpreter.getBuiltIn();
    List<PyString> argv = new ArrayList<>();
    argv.add(b.str(argv0));
    for (String arg : args)
      argv.add(b.str(arg));
    scope.putAny("_jp_argv", b.listFromItems(argv));

    PyObject savedArgv = b.eval("__import__('sys').argv", scope, scope);
    b.exec("import sys; sys.argv = list(_jp_argv)", scope, scope);
    try
    {
      return body.get();
    } finally
    {
      scope.putAny("_jp_saved_argv", savedArgv);
      b.exec("import sys; sys.argv = _jp_saved_argv", scope, scope);
    }
  }

}
