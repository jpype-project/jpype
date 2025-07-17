package org.jpype;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Instance of JPypeReflector loaded during bootstrapping.
 *
 * Due to restrictions in the Java module system it is necessary that the
 * Reflector by loaded in the unnamed module category with a special
 * classloader. As such this class may not appear in the jar as a loaded class.
 * Instead the class is stored in the META-INF directory as if it were class for
 * Java version 0. When JPype loads it fetches this class from the jar and loads
 * it with the required security privilages.
 */
public class Reflector0 implements JPypeReflector
{

  public Reflector0()
  {
  }

  /**
   * Call a method using reflection.
   *
   * This method creates a stackframe so that caller sensitive methods will
   * execute properly.
   *
   * @param method is the method to call.
   * @param obj is the object to operate on, it will be null if the method is
   * static.
   * @param args the arguments to method.
   * @return the object that results form the invocation.
   * @throws java.lang.Throwable throws whatever type the called method
   * produces.
   */
  public Object callMethod(Method method, Object obj, Object[] args)
          throws Throwable
  {
    try
    {
      return method.invoke(obj, args);
    } catch (InvocationTargetException ex)
    {
      throw ex.getCause();
    }
  }
}
