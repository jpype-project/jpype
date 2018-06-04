/*
 * Copyright 2018, Karl Einar Nelson
 * All rights reserved
 * 
 */
package org.jpype;

import java.lang.reflect.Constructor;
import java.lang.reflect.Executable;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 *
 */
public class TypeManager
{
  private static TypeManager INSTANCE = new TypeManager();
  public boolean isShutdown = false;
  public HashMap<Class, ClassDescription> classMap = new HashMap<>();
  LinkedList<Long> destroy = new LinkedList<>();
  public TypeFactory typeFactory = new TypeFactory();

//<editor-fold desc="interface">
  public static TypeManager getInstance()
  {
    return TypeManager.INSTANCE;
  }

  public synchronized void init()
  {
    if (!this.destroy.isEmpty())
      throw new RuntimeException("Initialize must only be called once");
    isShutdown = false;

    // Create the required minimum classes
    createClass(-1, Object.class);
    createClass(-1, Class.class);

    // Create the boxed types
    // These require special rules in the C++ layer so we will tag them 
    // as being different.
    createClass(0, Void.class);
    createClass(1, Boolean.class);
    createClass(2, Byte.class);
    createClass(3, Character.class);
    createClass(4, Short.class);
    createClass(5, Integer.class);
    createClass(6, Long.class);
    createClass(7, Float.class);
    createClass(8, Double.class);
    createClass(9, String.class);

    // Create the primitive types
    // Link boxed and primitive types so that the wrappers can find them.
    createPrimitive(0, Void.TYPE, Void.class);
    createPrimitive(1, Boolean.TYPE, Boolean.class);
    createPrimitive(2, Byte.TYPE, Byte.class);
    createPrimitive(3, Character.TYPE, Character.class);
    createPrimitive(4, Short.TYPE, Short.class);
    createPrimitive(5, Integer.TYPE, Integer.class);
    createPrimitive(6, Long.TYPE, Long.class);
    createPrimitive(7, Float.TYPE, Float.class);
    createPrimitive(8, Double.TYPE, Double.class);
  }

  /**
   * Find a wrapper for a class.
   * <p>
   * Creates one if needed.
   *
   * @param cls
   * @return the class wrapper, or null it one cannot be created.
   */
  public synchronized long findClass(Class cls)
  {
    if (this.isShutdown)
      return 0;

    ClassDescription ptr = this.classMap.get(cls);
    if (ptr != null)
      return ptr.classPtr;

    return createClass(-1, cls);
  }

  public synchronized void shutdown()
  {
    final int BLOCK_SIZE = 64;
    this.isShutdown = true;
    long[] set = new long[BLOCK_SIZE];
    int i = 0;

    // Destroy all wrapper class/methods/overloads
    Iterator<Long> iter = this.destroy.iterator();
    while (iter.hasNext())
    {
      set[i++] = iter.next();
      if (i == BLOCK_SIZE)
      {
        typeFactory.destroy(set,i);
        i = 0;
      }
    }

    for (; i < BLOCK_SIZE; ++i)
      set[i] = 0;
    typeFactory.destroy(set,i);

    // Clear the maps
    destroy.clear();
    this.classMap.clear();
  }

  /**
   * Load the constructors for a class.
   *
   * @param cls
   */
  public synchronized long getConstructor(Class cls)
  {
    LinkedList<Constructor> constructors = new LinkedList<>(Arrays.asList(cls.getDeclaredConstructors()));
    ClassDescription desc = this.classMap.get(cls);
    if (desc.constructorPtr != 0)
      return desc.constructorPtr;

    // Finded all public constructors
    filterPublic(constructors);

    // Sort them by precedence order
    List<MethodOverloadResolution> overloads = MethodOverloadResolution.sortMethods(constructors);

    // Convert overload list to a list of overloads pointers
    long[] overloadPtrs = this.createConstructorOverloads(desc, overloads);
    desc.constructorPtr = typeFactory.defineConstructorContainer(desc.classPtr, "[init", overloadPtrs);
    return desc.constructorPtr;
  }

  /**
   * Load the methods for a class.
   *
   * @param cls
   * @return a list of method wrappers.
   */
  public synchronized long[] getMethods(Class cls)
  {
    ClassDescription desc = this.classMap.get(cls);
    if (desc == null)
      throw new RuntimeException("Class not defined");

    // We may already be loaded
    if (desc.methodPtrs != null)
      return desc.methodPtrs;

    desc.methodContainerMap = new TreeMap<>();
    desc.methodOverloadMap = new TreeMap<>();

    // Collect a list of parents
    LinkedList<ClassDescription> parents = new LinkedList<>();
    ClassDescription superDesc = this.classMap.get(cls.getSuperclass());
    if (superDesc != null)
    {
      getMethods(cls.getSuperclass());
      parents.add(superDesc);
    }
    for (Class intr : cls.getInterfaces())
    {
      getMethods(intr);
      parents.add(this.classMap.get(intr));
    }

    // Figure out what we have to resolve by comparing the declared list 
    TreeMap<String, Long> resolve = new TreeMap<>();

    for (Method method : cls.getDeclaredMethods())
    {
      if (Modifier.isPublic(method.getModifiers()))
        resolve.put(method.getName(), 0l);
    }

    // Anything that is not overriden can be inherited.
    for (ClassDescription p : parents)
    {
      for (Map.Entry<String, Long> e : p.methodContainerMap.entrySet())
      {
        if (resolve.containsKey(e.getKey()))
        {
          resolve.put(e.getKey(), 0l);
        } else
        {
          resolve.put(e.getKey(), e.getValue());
        }
      }
    }

    // Get the list of all methods we will process
    LinkedList<Method> methods = new LinkedList<>(Arrays.asList(cls.getMethods()));

    // Remove overridden and non-public methods so we don't have to deal with them.
    filterOverridden(cls, methods);

    // Everything with a 0 needs to be resolved with a new method container
    // wrapper.  Everything with a non-zero number can be inherited.
    for (Map.Entry<String, Long> e : resolve.entrySet())
    {
      if (e.getValue() != 0)
      {
        // inherit the executable container
        desc.methodContainerMap.put(e.getKey(), e.getValue());
      } else
      {
        desc.methodContainerMap.put(e.getKey(), this.createMethodContainer(desc, e.getKey(), methods));
      }
    }

    // Collect a list from the map
    long[] methodPtrs = new long[desc.methodContainerMap.size()];
    int i = 0;
    for (long v : desc.methodContainerMap.values())
    {
      methodPtrs[i++] = v;
    }

    // Cache for later
    desc.methodPtrs = methodPtrs;
    return methodPtrs;
  }

//</editor-fold>
//<editor-fold desc="ctors" defaultstate="classes">
  /**
   * Allocate a new wrapper for a java class.
   * <p>
   * Boxed types require special handlers, as does java.lang.String
   *
   * @param code is >=0 for boxed types and -1 otherwise.
   * @param cls
   * @return a C++ wrapper handle for a jp_classtype
   */
  private long createClass(int code, Class cls)
  {
    if (cls.isArray())
    {
      // Array classes are simple, we just need the component type
      Class componentType = cls.getComponentType();
      long componentTypePtr = this.findClass(componentType);

      long classPtr = typeFactory.defineArrayClass(cls, cls.getSimpleName(), componentTypePtr);
      this.classMap.put(cls, new ClassDescription(cls, classPtr));
      this.destroy.add(classPtr);
      return classPtr;
    }

    // Object classes are more work as we need the super information as well.
    // Make sure all base classes are loaded
    Class superClass = cls.getSuperclass();
    long superClassPtr = 0;
    if (superClass != null)
    {
      superClassPtr = this.findClass(superClass);
    }

    // Make sure all interfaces are loaded.
    Class[] interfaces = cls.getInterfaces();
    long[] interfacesPtr = new long[interfaces.length];
    for (int i = 0; i < interfaces.length; ++i)
    {
      interfacesPtr[i] = this.findClass(interfaces[i]);
    }

    long classPtr = typeFactory.defineObjectClass(
            code,
            cls,
            cls.getSimpleName(),
            superClassPtr,
            interfacesPtr,
            cls.isInterface());
    this.classMap.put(cls, new ClassDescription(cls, classPtr));
    this.destroy.add(classPtr);
    return classPtr;
  }

  private void createPrimitive(int code, Class cls, Class boxed)
  {
    long classPtr = typeFactory.definePrimitive(code, cls, this.findClass(boxed));
    this.classMap.put(cls, new ClassDescription(cls, classPtr));
    this.destroy.add(classPtr);
  }

//</editor-fold>
//<editor-fold desc="containers" defaultstate="collapsed">
  private long createMethodContainer(
          ClassDescription desc,
          String key,
          LinkedList<Method> candidates)
  {
    // Find all the methods that match the key 
    LinkedList<Method> methods = new LinkedList<>();
    Iterator<Method> iter = candidates.iterator();
    boolean hasStatic = false;
    while (iter.hasNext())
    {
      Method next = iter.next();
      if (next.getName().equals(key))
      {
        iter.remove();
        methods.add(next);
      } else if (Modifier.isStatic(next.getModifiers()))
        hasStatic = true;
    }

    List<MethodOverloadResolution> overloads = MethodOverloadResolution.sortMethods(methods);

    // Convert overload list to a list of overloads pointers
    long[] overloadPtrs = this.createMethodOverloads(desc, overloads);
    long methodContainer = typeFactory.defineMethodContainer(desc.classPtr, key, overloadPtrs, hasStatic);

    // Place in the list for shutdown
    this.destroy.add(methodContainer);
    return methodContainer;
  }

//</editor-fold>
//<editor-fold desc="overloads" defaultstate="collapsed">
  /**
   * Construct a set of constructor overloads for an OverloadResolution.
   * <p>
   * These will be added to the shutdown destruction list.
   *
   * @param desc
   * @param overloads
   * @return
   */
  private long[] createConstructorOverloads(ClassDescription desc,
          List<MethodOverloadResolution> overloads)
  {
    int n = overloads.size();
    long[] overloadPtrs = new long[overloads.size()];
    for (MethodOverloadResolution ov : overloads)
    {
      Constructor constructor = (Constructor) ov.executable;
      Class<?>[] params = constructor.getParameterTypes();
      long[] paramPtrs = new long[params.length];
      int i = 0;
      for (Class<?> p : params)
      {
        paramPtrs[i++] = this.findClass(p);
      }
      i = 0;
      long[] precedencePtrs = new long[ov.children.size()];
      for (MethodOverloadResolution ch : ov.children)
      {
        precedencePtrs[i++] = ch.ptr;
      }
      ov.ptr = typeFactory.defineConstructorOverload(
              desc.classPtr,
              constructor,
              constructor.toString(),
              paramPtrs,
              precedencePtrs,
              constructor.isVarArgs());
      overloadPtrs[--n] = ov.ptr;

      // Place this on the clean up list
      this.destroy.add(ov.ptr);
    }
    return overloadPtrs;
  }

  /**
   * Convert a list of executable overload resolutions into a executable
   * overload list.
   * <p>
   * These will be added to the shutdown destruction list.
   *
   * @param desc
   * @param overloads
   * @return a list of method overload wrappers.
   */
  private long[] createMethodOverloads(
          ClassDescription desc,
          List<MethodOverloadResolution> overloads)
  {
    int n = overloads.size();
    long[] overloadPtrs = new long[overloads.size()];
    for (MethodOverloadResolution ov : overloads)
    {
      Method method = (Method) ov.executable;

      // We may already have built a methodoverload for this
      Class<?> decl = method.getDeclaringClass();
      if (method.getDeclaringClass() != desc.cls)
      {
        ov.ptr = this.classMap.get(decl).methodOverloadMap.get(method);
        overloadPtrs[--n] = ov.ptr;
        continue;
      }

      long returnPtr = findClass(method.getReturnType());

      // Convert the executable parameters
      Class<?>[] params = method.getParameterTypes();
      long[] paramPtrs = new long[params.length];
      int i = 0;
      for (Class<?> p : params)
      {
        paramPtrs[i++] = this.findClass(p);
      }

      // Determine what takes precedence
      i = 0;
      long[] precedencePtrs = new long[ov.children.size()];
      for (MethodOverloadResolution ch : ov.children)
      {
        precedencePtrs[i++] = ch.ptr;
      }

      ov.ptr = typeFactory.defineMethodOverload(
              desc.classPtr,
              method,
              method.toString(),
              returnPtr,
              paramPtrs,
              precedencePtrs,
              Modifier.isStatic(method.getModifiers()),
              method.isVarArgs());
      overloadPtrs[--n] = ov.ptr;

      // Place this on the clean up list
      this.destroy.add(ov.ptr);
    }
    return overloadPtrs;
  }
//</editor-fold>
//<editor-fold desc="filters" defaultstate="collapsed">

  /**
   * Remove any methods that are not public from a list.
   *
   * @param <T>
   * @param methods
   */
  public static <T extends Executable> void filterPublic(LinkedList<T> methods)
  {
    Iterator<T> iter = methods.iterator();
    while (iter.hasNext())
    {
      Executable next = iter.next();
      if (!Modifier.isPublic(next.getModifiers()))
      {
        iter.remove();
      }
    }
  }

  /**
   * Remove any methods that are not public and have been overridden from a
   * list.
   *
   * @param cls
   * @param methods
   */
  public static void filterOverridden(Class cls, LinkedList<Method> methods)
  {
    Iterator<Method> iter = methods.iterator();
    while (iter.hasNext())
    {
      Method next = iter.next();
      if (Modifier.isPublic(next.getModifiers()) && !isOverridden(cls, next))
        continue;
      iter.remove();
    }
  }

//</editor-fold>
//<editor-fold desc="utilities" defaultstate="collapsed">
  /**
   * Determines if a method is masked by another in a class.
   *
   * @param cls is the class to investigate.
   * @param method is a method that applies to the class.
   * @return true if the method is not overridden in the true, or false if this
   * method is overridden.
   */
  public static boolean isOverridden(Class cls, Method method)
  {
    try
    {
      return !method.equals(cls.getMethod(method.getName(), method.getParameterTypes()));
    } catch (NoSuchMethodException | SecurityException ex)
    {
      return false;
    }
  }
//</editor-fold>
}
