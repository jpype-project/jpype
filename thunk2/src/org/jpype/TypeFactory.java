/*
 * Copyright 2018, Karl Einar Nelson
 * All rights reserved.
 * 
 */
package org.jpype;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

/**
 * This is the interface for creating C++ object in JPype.
 * <p>
 * These methods are all native.
 *
 */
public class TypeFactory
{
  public native long definePrimitive(int code, Class cls, long boxedPtr);

  public native long defineObjectClass(
          int code,
          Class cls,
          String name,
          long superClassPtr,
          long[] superInterfaceClassPtrs,
          boolean isInterface
  );

  public native long defineArrayClass(
          Class cls,
          String name,
          long componentPtr);

  /**
   *
   * @param cls
   * @param name
   * @param overloads
   * @return
   */
  public native long defineConstructorContainer(
          long classPtr,
          String name,
          long[] overloads);

  /**
   * Creates a C++ wrapper for a method overload.
   *
   * @param classPtr is the C++ type wrapper for the defining class.
   * @param method is the method to be called.
   * @param name is the toString description of the method.
   * @param paramPtrs is a list of C++ type wrappers for arguments.
   * @param precedenceMethodOverloadPtr is a list of method overloads that have
   * precedence over this overload.
   * @param isVarArgs is true if the method takes variable arguments.
   * @return a pointer to the C++ wrapper.
   */
  public native long defineConstructorOverload(
          long classPtr,
          Constructor method,
          String name,
          long[] paramPtrs,
          long[] precedenceMethodOverloadPtr,
          boolean isVarArgs
  );

  public native long defineMethodContainer(
          long classPtr,
          String name,
          long[] overloads,
          boolean hasStatic);

  /**
   * Creates a C++ wrapper for a method overload.
   *
   * @param classPtr is the C++ type wrapper for the defining class.
   * @param method is the method to be called.
   * @param name is the toString description of the method.
   * @param retTypePtr is a C++ type wrapper for the return.
   * @param paramPtrs is a list of C++ type wrappers for arguments.
   * @param precedenceMethodOverloadPtr is a list of method overloads that have
   * precedence over this overload.
   * @param isStatic is true for a static method.
   * @param isVarArgs is true if the method takes variable arguments.
   * @return a pointer to the C++ wrapper.
   */
  public native long defineMethodOverload(
          long classPtr,
          Method method,
          String name,
          long retTypePtr,
          long[] paramPtrs,
          long[] precedenceMethodOverloadPtr,
          boolean isStatic,
          boolean isVarArgs
  );

  public native void destroy(long[] resources, int sz);
}
