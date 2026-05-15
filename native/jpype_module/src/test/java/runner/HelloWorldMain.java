// --- file: runner/HelloWorldMain.java ---
package runner;

import java.nio.file.Path;
import java.nio.file.Paths;
import org.jpype.bridge.Context;
import org.jpype.bridge.Interpreter;
import python.lang.PyBuiltIn;
import python.lang.PyObject;
import python.lang.PyString;

public class HelloWorldMain
{

  public static void main(String[] args)
  {
    try
    {
      Path here = Paths.get(System.getProperty("user.dir")).toAbsolutePath();
      Path root = here.resolve("../..").normalize();
      System.setProperty("python.module.path", root.toString());
      Interpreter.getInstance().start(new String[0]);
      Context context = new Context();
      context.exec("msg = 'Hello World from Python'");
      PyObject msg = context.eval("msg");
      System.out.println("Python returned: " + msg);

      context.exec(
              "class BuiltinAttrTest:\n"
              + "    pass\n"
              + "obj_default = BuiltinAttrTest()\n");
      PyObject obj = (PyObject) context.eval("obj_default");
      PyObject fallback = PyString.from("fallback");

      PyObject result = PyBuiltIn.getattrDefault(obj, "missing", fallback);

      System.out.println(result.equals(fallback));
    } catch (Throwable ex)
    {
      ex.printStackTrace();
      System.exit(1);
    }
  }
}
