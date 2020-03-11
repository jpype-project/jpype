/* ****************************************************************************
  Copyright 2019, Karl Einar Nelson

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 *****************************************************************************/
package org.jpype.manager;

import java.io.Serializable;
import java.lang.annotation.Annotation;
import java.lang.reflect.Constructor;
import java.lang.reflect.Executable;
import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.Proxy;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.TreeSet;
import org.jpype.proxy.JPypeProxy;

/**
 *
 */
public class TypeManager
{

  public long context = 0;
  public boolean isStarted = false;
  public boolean isShutdown = false;
  public HashMap<Class, ClassDescriptor> classMap = new HashMap<>();
  public TypeFactory typeFactory = null;
  public TypeAudit audit = null;
  private ClassDescriptor java_lang_Object;
  public Class<? extends Annotation> functionalAnnotation = null;

  public TypeManager()
  {
  }

  public TypeManager(long context, TypeFactory typeFactory)
  {
    this.context = context;
    this.typeFactory = typeFactory;
  }

//<editor-fold desc="interface">
  public synchronized void init()
  {
    if (isStarted)
      throw new RuntimeException("Cannot be restarted");
    isStarted = true;
    isShutdown = false;

    try
    {
      this.functionalAnnotation = Class.forName("java.lang.FunctionalInterface")
              .asSubclass(Annotation.class);
    } catch (ClassNotFoundException ex)
    {
      // It is okay if we don't find this
    }

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
    createClass(CharSequence.class, true);
    createClass(JPypeProxy.class, true);
    createClass(Method.class, true);
    createClass(Field.class, true);
    createClass(Throwable.class, true);

    // Create the primitive types
    // Link boxed and primitive types so that the wrappers can find them.
    createPrimitive("void", Void.TYPE, Void.class);
    createPrimitive("boolean", Boolean.TYPE, Boolean.class);
    createPrimitive("byte", Byte.TYPE, Byte.class);
    createPrimitive("char", Character.TYPE, Character.class);
    createPrimitive("short", Short.TYPE, Short.class);
    createPrimitive("int", Integer.TYPE, Integer.class);
    createPrimitive("long", Long.TYPE, Long.class);
    createPrimitive("float", Float.TYPE, Float.class);
    createPrimitive("double", Double.TYPE, Double.class);
  }

  /**
   * Find a wrapper for a class.
   * <p>
   * Creates one if needed. This a front end used by JPype.
   *
   * @param cls
   * @return the JPClass, or 0 it one cannot be created.
   */
  public synchronized long findClass(Class<?> cls)
  {
    if (cls == null)
      return 0;
    if (this.isShutdown)
      return 0;

    long out;
    if (cls.isSynthetic() && cls.getSimpleName().contains("$Lambda$"))
    {
      // If is it lambda, we need a special wrapper
      // we don't want to create a class each time in that case.
      // Thus use the parent interface for this class
      out = getClass(cls.getInterfaces()[0]).classPtr;
    } else if (cls.isAnonymousClass())
    {
      // This one is more of a burden.  It depends what whether is was
      // anonymous extends or implements.
      if (cls.getInterfaces().length == 1)
        out = getClass(cls.getInterfaces()[0]).classPtr;
      else
      {
        ClassDescriptor parent = getClass(cls.getSuperclass());
        out = createAnonymous(parent);
      }
    } else
    {
      // Just a regular class
      out = getClass(cls).classPtr;
    }

    return out;
  }

  /**
   * Get a class by name.
   *
   * @param name is the class name.
   * @return the C++ portion.
   */
  public long findClassByName(String name)
  {
    // Try it directly
    Class<?> cls = null;
    try
    {
      cls = Class.forName(name);
    } catch (ClassNotFoundException ex)
    {
    }

    // Special case for primitives
    if (cls == null && !name.contains("."))
    {
      if ("boolean".equals(name))
        cls = Boolean.TYPE;
      if ("byte".equals(name))
        cls = Byte.TYPE;
      if ("char".equals(name))
        cls = Character.TYPE;
      if ("short".equals(name))
        cls = Short.TYPE;
      if ("long".equals(name))
        cls = Long.TYPE;
      if ("int".equals(name))
        cls = Integer.TYPE;
      if ("float".equals(name))
        cls = Float.TYPE;
      if ("double".equals(name))
        cls = Double.TYPE;
    }

    // Attempt to find an inner class
    if (cls == null)
    {
      String[] parts = name.split("\\.");
      StringBuilder sb = new StringBuilder();
      sb.append(parts[0]);
      for (int i = 1; i < parts.length; ++i)
      {
        try
        {
          sb.append(".");
          sb.append(parts[i]);
          cls = Class.forName(sb.toString());
          for (int j = i + 1; j < parts.length; ++j)
          {
            sb.append("$");
            sb.append(parts[j]);
          }
          cls = Class.forName(sb.toString());
        } catch (ClassNotFoundException ex)
        {
        }
      }
    }
    return this.findClass(cls);
  }

  public synchronized void populateMethod(long wrapper, Executable method)
  {
    if (method == null)
      return;

    long returnType = 0;
    if (method instanceof Method)
    {
      returnType = getClass(((Method) method).getReturnType()).classPtr;
    }

    Class<?>[] params = method.getParameterTypes();
    int i = 0;
    long[] paramPtrs;
    if (!Modifier.isStatic(method.getModifiers()) && !(method instanceof Constructor))
    {
      paramPtrs = new long[params.length + 1];
      paramPtrs[0] = getClass(method.getDeclaringClass()).classPtr;
      i++;
    } else
    {
      paramPtrs = new long[params.length];
    }

    // Copy in the parameters
    for (Class<?> p : params)
    {
      paramPtrs[i] = getClass(p).classPtr;
      i++;
    }

    try
    {
      typeFactory.populateMethod(context, wrapper, returnType, paramPtrs);
    } catch (Exception ex)
    {
      ex.printStackTrace();
    }
  }

  /**
   * Get a class for an object.
   *
   * @param object is the object to interrogate.
   * @return the C++ portion or null if the object is null.
   */
  public long findClassForObject(Object object)
  {
    if (object == null)
      return 0;

    Class cls = object.getClass();
    if (Proxy.isProxyClass(cls)
            && (Proxy.getInvocationHandler(object) instanceof JPypeProxy))
    {
      return this.findClass(JPypeProxy.class);
    }

    return this.findClass(cls);
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
      if (entry.anonymous != 0)
        dest.add(entry.anonymous);
      dest.add(entry.classPtr);
    }
    dest.flush();

    // FIXME. If someone attempts to shutdown the JVM within a Python
    // proxy, everything will crash here.  We would lose the class
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

  /**
   * Allocate a new wrapper for a java class.
   * <p>
   * Boxed types require special handlers, as does java.lang.String
   *
   * @param cls is the Java class to wrap.
   * @param special marks class as requiring a specialized C++ wrapper.
   * @return a C++ wrapper handle for a jp_classtype
   */
  private ClassDescriptor createClass(Class<?> cls, boolean special)
  {
    if (cls.isArray())
      return this.createArrayClass(cls);

    // Object classes are more work as we need the super information as well.
    // Make sure all base classes are loaded
    Class<?> superClass = cls.getSuperclass();
    Class<?>[] interfaces = cls.getInterfaces();
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
    int modifiers = cls.getModifiers() & 0xffff;
    if (special)
      modifiers |= ModifierCode.SPECIAL.value;
    if (Throwable.class.isAssignableFrom(cls))
      modifiers |= ModifierCode.THROWABLE.value;
    if (Serializable.class.isAssignableFrom(cls))
      modifiers |= ModifierCode.SERIALIZABLE.value;
    if (this.functionalAnnotation != null
            && cls.getAnnotation(this.functionalAnnotation) != null)
      modifiers |= ModifierCode.FUNCTIONAL.value;

    // FIXME watch out for anonyous and lambda here.
    String name = cls.getCanonicalName();

    // Create the JPClass
    long classPtr = typeFactory.defineObjectClass(context, cls, name,
            superClassPtr,
            interfacesPtr,
            modifiers);

    //
    ClassDescriptor out = new ClassDescriptor(cls, classPtr);
    this.classMap.put(cls, out);

    return out;
  }

  private long createAnonymous(ClassDescriptor parent)
  {
    if (parent.anonymous != 0)
      return parent.anonymous;

    parent.anonymous = typeFactory.defineObjectClass(context,
            parent.cls, parent.cls.getCanonicalName() + "$Anonymous",
            parent.classPtr,
            null,
            ModifierCode.ANONYMOUS.value);
    return parent.anonymous;
  }

  ClassDescriptor createArrayClass(Class cls)
  {
    // Array classes are simple, we just need the component type
    Class componentType = cls.getComponentType();
    long componentTypePtr = this.getClass(componentType).classPtr;

    long classPtr = typeFactory
            .defineArrayClass(context, cls,
                    cls.getCanonicalName(), this.java_lang_Object.classPtr,
                    componentTypePtr,
                    cls.getModifiers() & 0xffff);

    ClassDescriptor out = new ClassDescriptor(cls, classPtr);
    this.classMap.put(cls, out);
    return out;
  }

  /**
   * Tell JPype to make a primitive Class.
   *
   * @param name
   * @param cls
   * @param boxed
   */
  private void createPrimitive(String name, Class cls, Class boxed)
  {
    long classPtr = typeFactory.definePrimitive(context,
            name,
            cls,
            this.getClass(boxed).classPtr,
            cls.getModifiers() & 0xffff);
    this.classMap.put(cls, new ClassDescriptor(cls, classPtr));
  }

//</editor-fold>
//<editor-fold desc="members" defaultstate="collapsed">
  public synchronized void populateMembers(Class cls)
  {
    ClassDescriptor desc = this.classMap.get(cls);
    if (desc == null)
      throw new RuntimeException("Class not loaded");
    if (desc.fields != null)
      return;
    try
    {
      createMembers(desc);
    } catch (Exception ex)
    {
      ex.printStackTrace(System.out);
      throw ex;
    }
  }

  private void createMembers(ClassDescriptor desc)
  {
    this.createFields(desc);
    this.createConstructorDispatch(desc);
    this.createMethodDispatches(desc);

    // Verify integrity
    if (audit != null)
      audit.verifyMembers(desc);

    // Pass this to JPype
    this.typeFactory.assignMembers(context,
            desc.classPtr,
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
      fieldPtr[i++] = this.typeFactory.defineField(context,
              desc.classPtr,
              field.getName(),
              field,
              getClass(field.getType()).classPtr,
              field.getModifiers() & 0xffff);
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
            .defineMethodDispatch(context,
                    desc.classPtr,
                    "<init>",
                    desc.constructors,
                    ModifierCode.PUBLIC.value | ModifierCode.CTOR.value);
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

      int i = 0;
      long[] precedencePtrs = new long[ov.children.size()];
      for (MethodResolution ch : ov.children)
      {
        precedencePtrs[i++] = ch.ptr;
      }

      int modifiers = constructor.getModifiers() & 0xffff;
      modifiers |= ModifierCode.CTOR.value;
      ov.ptr = typeFactory.defineMethod(context,
              desc.classPtr,
              constructor.toString(),
              constructor,
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
    Class<?> cls = desc.cls;

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

    int modifiers = 0;
    while (iter.hasNext())
    {
      Method next = iter.next();
      if (!next.getName().equals(key))
        continue;
      iter.remove();
      methods.add(next);
      if (Modifier.isStatic(next.getModifiers()))
        modifiers |= ModifierCode.STATIC.value;
      if (isBeanAccessor(next))
        modifiers |= ModifierCode.BEAN_ACCESSOR.value;
      if (isBeanMutator(next))
        modifiers |= ModifierCode.BEAN_MUTATOR.value;
    }

    // Convert overload list to a list of overloads pointers
    List<MethodResolution> overloads = MethodResolution.sortMethods(methods);
    long[] overloadPtrs = this.createMethods(desc, overloads);

    long methodContainer = typeFactory.defineMethodDispatch(context,
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
        this.populateMembers(decl);
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

      // Determine what takes precedence
      int i = 0;
      long[] precedencePtrs = new long[ov.children.size()];
      for (MethodResolution ch : ov.children)
      {
        precedencePtrs[i++] = ch.ptr;
      }

      int modifiers = method.getModifiers() & 0xffff;
      if (isBeanMutator(method))
        modifiers |= ModifierCode.BEAN_MUTATOR.value;
      if (isBeanAccessor(method))
        modifiers |= ModifierCode.BEAN_ACCESSOR.value;
      if (isCallerSensitive(method))
        modifiers |= ModifierCode.CALLER_SENSITIVE.value;

      ov.ptr = typeFactory.defineMethod(context,
              desc.classPtr,
              method.toString(),
              method,
              precedencePtrs,
              modifiers);
      overloadPtrs[--n] = ov.ptr;
      desc.methods[desc.methodCounter] = ov.ptr;
      desc.methodIndex[desc.methodCounter] = method;
      desc.methodCounter++;
    }
    return overloadPtrs;
  }

  static boolean hasCallerSensitive = false;

  static
  {
    try
    {
      java.lang.reflect.Method method = java.lang.Class.class.getDeclaredMethod("forName", String.class);
      for (Annotation annotation : method.getAnnotations())
      {
        if ("@jdk.internal.reflect.CallerSensitive()".equals(annotation.toString()))
        {
          hasCallerSensitive = true;
        }
      }
    } catch (NoSuchMethodException | SecurityException ex)
    {
    }
  }

  /**
   * Checks to see if the method is caller sensitive.
   *
   * As the annotation is a private internal, we must check by name.
   *
   * @param method is the method to be probed.
   * @return true if caller sensitive.
   */
  public static boolean isCallerSensitive(Method method)
  {
    if (hasCallerSensitive)
    {
      for (Annotation annotation : method.getAnnotations())
      {
        if ("@jdk.internal.reflect.CallerSensitive()".equals(annotation.toString()))
        {
          return true;
        }
      }
    } else
    {
      // JDK prior versions prior to 9 do not annotate methods that
      // require special handling, thus we will just blanket those
      // classes known to have issues.
      Class<?> cls = method.getDeclaringClass();
      if (cls.equals(java.lang.Class.class)
              || cls.equals(java.lang.ClassLoader.class)
              || cls.equals(java.sql.DriverManager.class))
      {
        return true;
      }
    }
    return false;
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
   * @return a new list containing only public members.
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
   * @return a new list containing only public members that are not overridden.
   */
  public static LinkedList<Method> filterOverridden(Class<?> cls, Method[] methods)
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
   * @return true if the method is hidden by another method.
   */
  public static boolean isOverridden(Class<?> cls, Method method)
  {
    try
    {
      return !method.equals(cls.getMethod(method.getName(), method.getParameterTypes()));
    } catch (NoSuchMethodException | SecurityException ex)
    {
      return false;
    }
  }

  /**
   * Bean accessor is flag is used for property module.
   * <p>
   * Accessors need
   *
   * @param method
   * @return
   */
  private boolean isBeanAccessor(Method method)
  {
    if (Modifier.isStatic(method.getModifiers()))
      return false;
    if (method.getReturnType().equals(void.class))
      return false;
    if (method.getParameterCount() > 0)
      return false;
    if (method.getName().length() < 4)
      return false;
    return (method.getName().startsWith("get"));
  }

  /**
   * Bean mutator is flag is used for property module.
   *
   * @param method
   * @return
   */
  private boolean isBeanMutator(Method method)
  {
    if (Modifier.isStatic(method.getModifiers()))
      return false;
    if (!method.getReturnType().equals(void.class))
      return false;
    if (method.getParameterCount() != 1)
      return false;
    if (method.getName().length() < 4)
      return false;
    return (method.getName().startsWith("set"));
  }

//</editor-fold>
//<editor-fold desc="inner" defaultstate="collapsed">
  private class Destroyer
  {

    final int BLOCK_SIZE = 1024;
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
        typeFactory.destroy(context, v, v.length);
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
      typeFactory.destroy(context, queue, index);
      index = 0;
    }
  }
//</editor-fold>
}
