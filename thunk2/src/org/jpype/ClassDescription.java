/*
 * Copyright 2018, Karl Einar Nelson
 * All rights reserved.
 * 
 */
package org.jpype;

import java.lang.reflect.Method;
import java.util.TreeMap;

/**
 * A list of resources associated with this class.
 * 
 * These can be accessed within JPype using the org.jpype.TypeManager.
 * 
 */
public class ClassDescription
{
  public Class cls;
  /** The C++ jp_classtype pointer for this class. */
  
  public long classPtr;
  
  /** The jp_method wrapper for the constructor. */
  public long constructorPtr;
  
  /** The list of all jp_method for this class. */
  public long[] methodPtrs;
  
  /** The map of all method names to C++ method overload wrappers. */
  public TreeMap<Method, Long> methodOverloadMap;
  
  /** The map of all method names to C++ method container wrappers. */
  public TreeMap<String, Long> methodContainerMap;

  ClassDescription(Class cls, long classPtr)
  {
    this.cls = cls;
    this.classPtr = classPtr;
  }
}
