package org.jpype;

import java.lang.reflect.Method;

/**
 * Interface for invoking methods using reflection in JPype.
 *
 * The {@code JPypeReflector} interface provides a mechanism to call methods
 * dynamically using Java reflection. This is particularly useful for invoking
 * caller-sensitive methods while ensuring proper stack frame creation for
 * execution.
 */
public interface JPypeReflector
{

  /**
   * Invokes a method using reflection.
   *
   * This method dynamically calls the specified method on the provided object
   * (or class, if the method is static) using reflection. It ensures that
   * caller-sensitive methods execute correctly by creating the appropriate
   * stack frame during invocation.
   *
   * @param method is the method to call.
   * @param obj is the object to operate on, it will be null if the method is
   * static.
   * @param args the arguments to method.
   * @return the object that results form the invocation.
   * @throws java.lang.Throwable throws whatever type the called method
   * produces.
   */
  public Object callMethod(Method method, Object obj, Object[] args) throws Throwable;
}
