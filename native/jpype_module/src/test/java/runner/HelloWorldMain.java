package runner;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import org.jpype.bridge.Context;
import org.jpype.bridge.Interpreter;
import python.lang.PyBuiltIn;
import python.lang.PyFloat;
import python.lang.PyInt;
import python.lang.PyList;
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
      Interpreter.getInstance().start(new String[0]);
      Context context = new Context();
      context.exec("msg = 'Hello World from Python'");
      PyObject result = context.eval("msg");
      System.out.println("Python returned: " + result);

      java.util.List<String> jList = Arrays.asList("a", "b");
      PyList pList = PyBuiltIn.list(jList);
      System.out.println(PyBuiltIn.len(pList));
      System.out.println(pList.toString());
      PyObject item = pList.get(0);
      System.out.println(item.toString());
    } catch (Throwable ex)
    {
      ex.printStackTrace();
      System.exit(1);
    }
  }
}
