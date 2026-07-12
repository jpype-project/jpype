// --- file: runner/HelloWorldMain.java ---
package runner;

import java.nio.file.Path;
import java.nio.file.Paths;
import org.jpype.Script;
import org.jpype.MainInterpreter;
import python.lang.PyObject;

public class HelloWorldMain
{

  public static void main(String[] args)
  {
    try
    {
      Path here = Paths.get(System.getProperty("user.dir")).toAbsolutePath();
      Path root = here.resolve("../..").normalize();
      System.setProperty("python.module.path", root.toString());
      MainInterpreter.getInstance().start(new String[0]);
      Script context = new Script(MainInterpreter.getInstance());
      context.exec("msg = 'Hello World from Python'");
      PyObject msg = context.eval("msg");
      System.out.println("Python returned: " + msg);

      context.exec(
              "class BuiltinAttrTest:\n"
              + "    pass\n"
              + "obj_default = BuiltinAttrTest()\n");
      PyObject obj = context.eval("obj_default");
      PyObject fallback = context.str("fallback");

      PyObject result = context.getattrDefault(obj, "missing", fallback);

      System.out.println(result.equals(fallback));
    } catch (Throwable ex)
    {
      ex.printStackTrace();
      System.exit(1);
    }
  }
}
