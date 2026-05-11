package runner;


import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import org.jpype.bridge.Context;
import org.jpype.bridge.Interpreter;
import python.lang.PyObject;

public class HelloWorldMain
{
  public static void main(String[] args)
  {
    try
    {
      
       Path here = Paths.get(System.getProperty("user.dir")).toAbsolutePath();
      Path root = here.resolve("../..").normalize();

      System.out.println("user.dir       = " + here);
      System.out.println("repo root      = " + root);

      // Adjust these names to match your actual files
      Path jpypeModule = root.resolve("_jpype.pyd");
      Path jpyneModule = root.resolve("_jpyne.pyd");

      System.out.println("jpype module   = " + jpypeModule);
      System.out.println("bootstrap module   = " + jpyneModule);

      // If Python itself is not easily found by PATH, set this too
      // System.setProperty("python.executable", "/usr/bin/python3");

      // If your Python-side modules are rooted at repo top
      System.setProperty("python.module.path", root.toString());

      
      Interpreter.getInstance().start(new String[0]);

      Context context = new Context();
      context.exec("msg = 'Hello World from Python'");
      PyObject result = context.eval("msg");

      System.out.println("Python returned: " + result);
      System.out.println("Bridge startup successful.");
    } catch (Throwable ex)
    {
      ex.printStackTrace();
      System.exit(1);
    }
  }
}