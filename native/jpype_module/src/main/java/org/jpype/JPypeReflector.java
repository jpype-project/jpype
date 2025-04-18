package org.jpype;

import java.lang.reflect.Method;

/**
 *
 * @author nelson85
 */
public interface JPypeReflector
{

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
          throws Throwable;
}
