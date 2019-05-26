/*
 *    Copyright 2019 Karl Einar Nelson
 *   
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *  
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
package org.jpype.manager;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.TreeSet;

/**
 *
 */
public class TypeManager
{
//  private static TypeManager INSTANCE = new TypeManager();
  public boolean isStarted = false;
  public boolean isShutdown = false;
  public HashMap<Class, ClassDescriptor> classMap = new HashMap<>();
  public TypeFactory typeFactory = null;
  public List<Action> deferred = new LinkedList<>();
  public TypeAudit audit = null;
  private ClassDescriptor java_lang_Object;

//<editor-fold desc="interface">
  public synchronized void init()
  {
    if (isStarted)
      throw new RuntimeException("Cannot be restarted");
    isStarted = true;
    isShutdown = false;

    // Create the required minimum classes
    this.java_lang_Object = createClass(Object.class, true);
    createClass(Class.class, true);

    // Create the boxed types
    // These require special rules in the C++ layer so we will tag them 
    // as being different.
    createClass(Void.class, true);
    createClass(Boolean.class, true);
    createClass(Byte.class, true);
    createClass(Character.class, true);
    createClass(Short.class, true);
    createClass(Integer.class, true);
    createClass(Long.class, true);
    createClass(Float.class, true);
    createClass(Double.class, true);
    createClass(String.class, true);

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
    this.executeDeferred();
  }

  /**
   * Find a wrapper for a class.
   * <p>
   * Creates one if needed. This a front end used by JPype.
   *
   * @param cls
   * @return the JPClass, or 0 it one cannot be created.
   */
  public synchronized long findClass(Class cls)
  {
    if (this.isShutdown)
      return 0;

    ClassDescriptor out = getClass(cls);

    // Before we can leave, we need to make sure all classes that were 
    // loaded in the process have methods.
    executeDeferred();

    return out.classPtr;
  }

  /**
   * Called to delete all C++ resources
   */
  public synchronized void shutdown()
  {
    // First and most important, we can't operate from this 
    // point forward.
    this.isShutdown = true;

    // Next set up a block for deleting resources
    Destroyer dest = new Destroyer();

    // Destroy all the resources held in C++
    for (ClassDescriptor entry : this.classMap.values())
    {
      if (entry.constructorDispatch != 0)
        dest.add(entry.constructorDispatch);
      dest.add(entry.constructors);
      dest.add(entry.methodDispatch);
      dest.add(entry.methods);
      dest.add(entry.fields);
      dest.add(entry.classPtr);
    }
    dest.flush();

    // FIXME. If someone was to try the wild ass stunt of 
    // shutting down the JVM from within a python proxy method
    // it would likely all go to hell. We would lose the class
    // that is calling things and the ability to throw exceptions.
    // Most likely this will go splat. We need to catch this 
    // from within JPype and hard fault our way to safety.
    this.classMap.clear();
  }
//</editor-fold>
//<editor-fold desc="classes" defaultstate="defaultstate">

  private ClassDescriptor getClass(Class cls)
  {
    if (cls == null)
      return null;

    // Look up the current description
    ClassDescriptor ptr = this.classMap.get(cls);
    if (ptr != null)
      return ptr;

    // If we can't find it create a new class
    return createClass(cls, false);
  }

  private void executeDeferred()
  {
    while (!this.deferred.isEmpty())
    {
      Action next = this.deferred.remove(0);

      // This may trigger even more classes to get loaded
      next.execute();
    }
  }

  /**
   * Allocate a new wrapper for a java class.
   * <p>
   * Boxed types require special handlers, as does java.lang.String
   *
   * @param code is >=0 for boxed types and -1 otherwise.
   * @param cls
   * @return a C++ wrapper handle for a jp_classtype
   */
  private ClassDescriptor createClass(Class cls, boolean special)
  {
    if (cls.isArray())
      return this.createArrayClass(cls);

    // Object classes are more work as we need the super information as well.
    // Make sure all base classes are loaded
    Class superClass = cls.getSuperclass();
    Class[] interfaces = cls.getInterfaces();
    ClassDescriptor[] parents = new ClassDescriptor[interfaces.length + 1];
    long[] interfacesPtr = new long[interfaces.length];
    long superClassPtr = 0;
    if (superClass != null)
    {
      parents[0] = this.getClass(superClass);
      superClassPtr = parents[0].classPtr;
    }

    // Make sure all interfaces are loaded.
    for (int i = 0; i < interfaces.length; ++i)
    {
      parents[i + 1] = this.getClass(interfaces[i]);
      interfacesPtr[i] = parents[i + 1].classPtr;
    }

    // Set up the modifiers
    int modifiers = cls.getModifiers();
    if (special)
      modifiers |= ModifierCodes.SPECIAL.value;
    if (Throwable.class.isAssignableFrom(cls))
      modifiers |= ModifierCodes.THROWABLE.value;

    // FIXME watch out for anonyous and lambda here.
    String name = cls.getSimpleName();

    // Create the JPClass
    long classPtr = typeFactory.defineObjectClass(
            cls,
            superClassPtr,
            interfacesPtr,
            modifiers,
            name);

    //
    ClassDescriptor out = new ClassDescriptor(cls, classPtr);
    this.classMap.put(cls, out);

    this.deferred.add(new CreateMembers(out));
    return out;
  }

  ClassDescriptor createArrayClass(Class cls)
  {
    // Array classes are simple, we just need the component type
    Class componentType = cls.getComponentType();
    long componentTypePtr = this.getClass(componentType).classPtr;

    long classPtr = typeFactory
            .defineArrayClass(
                    cls,
                    this.java_lang_Object.classPtr,
                    cls.getSimpleName(),
                    componentTypePtr);

    ClassDescriptor out = new ClassDescriptor(cls, classPtr);
    this.classMap.put(cls, out);
    return out;
  }

  /**
   * Tell JPype to make a primitive Class.
   *
   * @param code
   * @param cls
   * @param boxed
   */
  private void createPrimitive(int code, Class cls, Class boxed)
  {
    long classPtr = typeFactory.definePrimitive(
            code,
            cls,
            this.getClass(boxed).classPtr);
    this.classMap.put(cls, new ClassDescriptor(cls, classPtr));
  }

//</editor-fold>
//<editor-fold desc="members" defaultstate="collapsed">
  private void createMembers(ClassDescriptor desc)
  {
    this.createFields(desc);
    this.createConstructorDispatch(desc);
    this.createMethodDispatches(desc);

    // Verify integrity
    if (audit != null)
      audit.verifyMembers(desc);

    // Pass this to JPype      
    this.typeFactory.assignMembers(desc.classPtr,
            desc.constructorDispatch,
            desc.methodDispatch,
            desc.fields);
  }

//<editor-fold desc="fields" defaultstate="collapsed">
  private void createFields(ClassDescriptor desc)
  {
    // We only need declared fields as the wrappers for previous classes hold
    // members declared earlier
    LinkedList<Field> fields = filterPublic(desc.cls.getDeclaredFields());

    long[] fieldPtr = new long[fields.size()];
    int i = 0;
    for (Field field : fields)
    {
      fieldPtr[i++] = this.typeFactory.defineField(
              desc.classPtr,
              field.getName(),
              field,
              getClass(field.getType()).classPtr,
              field.getModifiers());
    }
    desc.fields = fieldPtr;
  }
//</editor-fold>
//<editor-fold desc="ctor" defaultstate="collapsed">

  /**
   * Load the constructors for a class.
   *
   * @param desc
   */
  public void createConstructorDispatch(ClassDescriptor desc)
  {
    Class cls = desc.cls;

    // Get the list of declared constructors
    LinkedList<Constructor> constructors
            = filterPublic(cls.getDeclaredConstructors());

    if (constructors.isEmpty())
      return;

    // Sort them by precedence order
    List<MethodResolution> overloads = MethodResolution.sortMethods(constructors);

    // Convert overload list to a list of overloads pointers
    desc.constructors = this.createConstructors(desc, overloads);

    // Create the dispatch for it
    desc.constructorDispatch = typeFactory
            .defineMethodDispatch(
                    desc.classPtr,
                    "<init>",
                    desc.constructors,
                    ModifierCodes.CTOR.value);
  }

  /**
   * Construct a set of constructor overloads for an OverloadResolution.
   * <p>
   * These will be added to the shutdown destruction list.
   *
   * @param desc
   * @param overloads
   * @return
   */
  private long[] createConstructors(ClassDescriptor desc,
          List<MethodResolution> overloads)
  {
    int n = overloads.size();
    long[] overloadPtrs = new long[overloads.size()];
    for (MethodResolution ov : overloads)
    {
      Constructor constructor = (Constructor) ov.executable;
      Class[] params = constructor.getParameterTypes();
      long[] paramPtrs = new long[params.length];
      int i = 0;
      for (Class p : params)
      {
        paramPtrs[i++] = this.getClass(p).classPtr;
      }

      i = 0;
      long[] precedencePtrs = new long[ov.children.size()];
      for (MethodResolution ch : ov.children)
      {
        precedencePtrs[i++] = ch.ptr;
      }

      int modifiers = constructor.getModifiers() | ModifierCodes.CTOR.value;
      ov.ptr = typeFactory.defineMethod(
              desc.classPtr,
              constructor.toString(),
              constructor,
              0,
              paramPtrs,
              precedencePtrs,
              modifiers);
      overloadPtrs[--n] = ov.ptr;
    }
    return overloadPtrs;
  }

//</editor-fold>
//<editor-fold desc="methods" defaultstate="collapsed">
  /**
   * Load the methods for a class.
   *
   * @param desc
   */
  public void createMethodDispatches(ClassDescriptor desc)
  {
    Class cls = desc.cls;

    // Get the list of all public, non-overrided methods we will process
    LinkedList<Method> methods = filterOverridden(cls, cls.getMethods());

    // Get the list of public declared methods
    LinkedList<Method> declaredMethods = filterOverridden(cls, cls.getDeclaredMethods());

    // We only need one dispatch per name
    TreeSet<String> resolve = new TreeSet<>();
    for (Method method : declaredMethods)
    {
      resolve.add(method.getName());
    }

    // Reserve memory for our lookup table
    desc.methods = new long[declaredMethods.size()];
    desc.methodIndex = new Method[declaredMethods.size()];
    desc.methodDispatch = new long[resolve.size()];

    int i = 0;
    for (String name : resolve)
    {
      desc.methodDispatch[i++] = this.createMethodDispatch(desc, name, methods);
    }
  }

  private long createMethodDispatch(
          ClassDescriptor desc,
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
      if (!next.getName().equals(key))
        continue;
      iter.remove();
      methods.add(next);
      if (Modifier.isStatic(next.getModifiers()))
        hasStatic = true;
    }

    // Convert overload list to a list of overloads pointers
    List<MethodResolution> overloads = MethodResolution.sortMethods(methods);
    long[] overloadPtrs = this.createMethods(desc, overloads);

    int modifiers = 0;
    if (hasStatic)
      modifiers |= ModifierCodes.STATIC.value;
    long methodContainer = typeFactory.defineMethodDispatch(
            desc.classPtr,
            key,
            overloadPtrs,
            modifiers);

    return methodContainer;
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
  private long[] createMethods(
          ClassDescriptor desc,
          List<MethodResolution> overloads)
  {
    int n = overloads.size();
    long[] overloadPtrs = new long[overloads.size()];
    for (MethodResolution ov : overloads)
    {
      Method method = (Method) ov.executable;

      // We may already have built a methodoverload for this
      Class<?> decl = method.getDeclaringClass();
      if (method.getDeclaringClass() != desc.cls)
      {
        ov.ptr = this.classMap.get(decl).getMethod(method);
        if (ov.ptr == 0)
        {
          if (audit != null)
            audit.failFindMethod(desc, method);
          throw new RuntimeException("Fail");
        }
        overloadPtrs[--n] = ov.ptr;
        continue;
      }

      long returnPtr = getClass(method.getReturnType()).classPtr;

      // Convert the executable parameters
      Class<?>[] params = method.getParameterTypes();
      long[] paramPtrs = new long[params.length];
      int i = 0;
      for (Class<?> p : params)
      {
        paramPtrs[i++] = this.getClass(p).classPtr;
      }

      // Determine what takes precedence
      i = 0;
      long[] precedencePtrs = new long[ov.children.size()];
      for (MethodResolution ch : ov.children)
      {
        precedencePtrs[i++] = ch.ptr;
      }

      int modifiers = method.getModifiers();
      ov.ptr = typeFactory.defineMethod(
              desc.classPtr,
              method.toString(),
              method,
              returnPtr,
              paramPtrs,
              precedencePtrs,
              modifiers);
      overloadPtrs[--n] = ov.ptr;
      desc.methods[desc.methodCounter] = ov.ptr;
      desc.methodIndex[desc.methodCounter] = method;
      desc.methodCounter++;
    }
    return overloadPtrs;
  }

//</editor-fold>
//<editor-fold desc="containers" defaultstate="collapsed">
//</editor-fold>
//</editor-fold>
//<editor-fold desc="filters" defaultstate="collapsed">
  /**
   * Remove any methods that are not public from a list.
   *
   * @param <T>
   * @param methods
   */
  public static <T extends Member> LinkedList<T> filterPublic(T[] methods)
  {
    LinkedList<T> out = new LinkedList<>();
    for (T method : methods)
    {
      if (!Modifier.isPublic(method.getModifiers()))
        continue;
      out.add(method);
    }
    return out;
  }

  /**
   * Remove any methods that are not public and have been overridden from a
   * list.
   *
   * @param cls
   * @param methods
   */
  public static LinkedList<Method> filterOverridden(Class cls, Method[] methods)
  {
    LinkedList<Method> out = new LinkedList<>();
    for (Method method : methods)
    {
      if (!Modifier.isPublic(method.getModifiers()) || isOverridden(cls, method))
        continue;
      out.add(method);
    }
    return out;
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
  private interface Action
  {
    void execute();
  }

  private class CreateMembers implements Action
  {
    ClassDescriptor cd;

    public CreateMembers(ClassDescriptor out)
    {
      this.cd = out;
    }

    @Override
    public void execute()
    {
      createMembers(cd);
    }
  }

  private class Destroyer
  {
    final int BLOCK_SIZE = 64;
    long[] queue = new long[BLOCK_SIZE];
    int index = 0;

    void add(long v)
    {
      queue[index++] = v;
      if (index == BLOCK_SIZE)
        flush();
    }

    void add(long[] v)
    {
      if (v == null)
        return;
      if (v.length > BLOCK_SIZE / 2)
      {
        typeFactory.destroy(v, v.length);
        return;
      }
      if (index + v.length > BLOCK_SIZE)
      {
        flush();
      }
      for (int j = 0; j < v.length; ++j)
      {
        queue[index++] = v[j];
      }
      if (index == BLOCK_SIZE)
        flush();
    }

    void flush()
    {
      typeFactory.destroy(queue, index);
      index = 0;
    }
  }
}
