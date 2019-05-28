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

import java.lang.reflect.Executable;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;

/**
 * A special version of the TypeFactory for debugging and testing.
 * 
 * This harness operates like JPype C++ layer with checks for problems and 
 * inconsistencies that may indicate a problem with the TypeManager.
 *
 * @author nelson85
 */
public class TypeFactoryHarness implements TypeFactory, TypeAudit
{
  long value = 0;
  HashMap<Long, Resource> resourceMap = new HashMap<>();
  private final TypeManager typeManager;

  TypeFactoryHarness(TypeManager tm)
  {
    this.typeManager = tm;

    // Register as the audit for the TypeManager
    tm.audit = this;
  }

  <T extends Resource> T assertResource(long id, Class<T> cls)
  {
    Resource resource = resourceMap.get(id);
    if (resource == null)
      throw new RuntimeException("Resource not found " + id + " " + cls.getName());
    if (!cls.isInstance(resource))
      throw new RuntimeException("Incorrect resource type " + id
              + "  expected: " + cls.getName()
              + "  found: " + resource.getClass().getName());
    return (T) resource;
  }

//<editor-fold desc="class" defaultstate="collapsed">
  @Override
  public long defineArrayClass(
          long context,
          Class cls,
          String name,
          long superClass,
          long componentPtr,
          int modifiers)
  {
    value++;
    System.out.println("defineArrayClass " + value + ": " + name);
    System.out.println("  modifiers: " + ModifierCode.decode(modifiers));
    Resource resource = new ArrayClassResource(value, "array class " + name, cls);
    resourceMap.put(value, resource);
    return value;
  }

  @Override
  public long defineObjectClass(long context, Class cls, String name, long superClass, long[] interfaces, int modifiers)
  {
    value++;
    System.out.println("defineObjectClass " + value + ": " + name);
    System.out.println("  modifiers: " + ModifierCode.decode(modifiers));
    if (superClass != 0)
    {
      ClassResource superClassResource = assertResource(superClass, ClassResource.class);
      System.out.println("  super: " + superClass + " " + superClassResource.getName());
    }
    this.dumpResourceList("interfaces", interfaces);

    Resource resource = new ObjectClassResource(value, "object class " + name, cls);
    resourceMap.put(value, resource);
    return value;
  }

  @Override
  public long definePrimitive(long context, String name, Class cls, long boxedPtr, int modifiers)
  {
    value++;
    System.out.println("defineObjectClass " + value + ": " + cls.toString());
    System.out.println("  modifiers: " + ModifierCode.decode(modifiers));
    Resource resource = new PrimitiveClassResource(value, "primitive class " + cls.getName(), cls);
    resourceMap.put(value, resource);
    return value;
  }
//</editor-fold>
//<editor-fold desc="members" defaultstate="collapsed">

  @Override
  public void assignMembers(
          long context,
          long classId,
          long ctorMethod,
          long[] methodList,
          long[] fieldList)
  {
    // Verify resources
    ClassResource classResource = assertResource(classId, ClassResource.class);
    try
    {
      if (ctorMethod != 0)
        assertResource(ctorMethod, MethodDispatchResource.class);
      for (int i = 0; i < methodList.length; ++i)
        assertResource(methodList[i], MethodDispatchResource.class);
      for (int i = 0; i < fieldList.length; ++i)
        assertResource(fieldList[i], FieldResource.class);

      System.out.println("assignMembers " + classId + " " + classResource.getName());
    } catch (Exception ex)
    {
      System.out.println("Fail in assignClass");
      ClassResource r = (ClassResource) this.resourceMap.get(classId);
      dump(typeManager.classMap.get(r.getResource()));
      throw ex;
    }
  }

  @Override
  public long defineField(
          long context,
          long classId,
          String name,
          Field field,
          long fieldTypeId,
          int modifiers)
  {
    ClassResource classResource = assertResource(classId, ClassResource.class);
    ClassResource fieldResource = assertResource(fieldTypeId, ClassResource.class);
    value++;
    System.out.println("defineField " + value + ":" + name);
    System.out.println("  modifiers: " + ModifierCode.decode(modifiers));
    System.out.println("  class: " + classResource.getName());
    System.out.println("  fieldType: " + fieldResource.getName());

    resourceMap.put(value, new FieldResource(value, "field " + field.getName(), field));
    return value;
  }
  
  @Override
  public long defineMethod(
          long context,
          long classId, String name, Executable method,
          long returnType,
          long[] argumentTypes,
          long[] overloadList,
          int modifiers)
  {
    // Verify resources
    assertResource(classId, ClassResource.class);
    if (returnType != 0)
      assertResource(returnType, ClassResource.class);
    for (int i = 0; i < argumentTypes.length; ++i)
      assertResource(argumentTypes[i], ClassResource.class);
    for (int i = 0; i < overloadList.length; ++i)
      assertResource(overloadList[i], MethodResource.class);
    value++;
    System.out.println("defineMethod " + value + ": " + name);
    System.out.println("  modifiers: " + ModifierCode.decode(modifiers));
    System.out.flush();

    resourceMap.put(value, new MethodResource(value, "method " + method.toString(), method));
    return value;
  }

  @Override
  public long defineMethodDispatch(
          long context,
          long classId,
          String name,
          long[] overloadList,
          int modifiers)
  {
    ClassResource classResource = assertResource(classId, ClassResource.class);
    for (int i = 0; i < overloadList.length; ++i)
      assertResource(overloadList[i], MethodResource.class);
    value++;
    System.out.println("defineMethodDispatch " + value + ": '" + name + "' for " + classResource.getName());
    System.out.println("  modifiers: " + ModifierCode.decode(modifiers));
    this.dumpResourceList("members", overloadList);

    resourceMap.put(value, new MethodDispatchResource(value, "dispatch " + name));
    return value;
  }
//</editor-fold>
//<editor-fold desc="destroy" defaultstate="collapsed">
  Resource lastDestroyed = null;

  @Override
  public void destroy(long context, long[] resources, int sz)
  {
    System.out.println("destroy resources " + sz);
    for (int i = 0; i < sz; ++i)
    {
      long r = resources[i];
      if (this.resourceMap.containsKey(r))
      {
        //  if (r==0)
        //    continue;
        System.out.println("  destroy " + r + ": " + this.resourceMap.get(r).getName());
        lastDestroyed = this.resourceMap.remove(r);
        if (!(lastDestroyed instanceof DeletedResource))
        {
          this.resourceMap.put(r, new DeletedResource(lastDestroyed));
          continue;
        }
      }

      if (lastDestroyed != null)
      {
        if (lastDestroyed instanceof MethodResource)
        {
          Class<?> cls = ((MethodResource) lastDestroyed).getResource().getDeclaringClass();
          dump(this.typeManager.classMap.get(cls));
        }
      }

      throw new RuntimeException("repeat delete " + r + " at index " + i + " last " + lastDestroyed);
    }
  }
//</editor-fold>
//<editor-fold desc="audit" defaultstate="collapsed">

  @Override
  public void dump(ClassDescriptor desc)
  {
    System.out.println("ClassDescriptor: " + desc.classPtr);
    System.out.println("  class: " + desc.cls.toString());
    System.out.println("  methodCounter:" + desc.methodCounter);
    if (desc.constructorDispatch != 0)
    {
      System.out.println("  ctor dispatch:");
      Resource resource = this.resourceMap.get(desc.constructorDispatch);
      if (resource == null)
        System.out.println("    null");
      else
        System.out.println("    " + resource.getEntityId() + " " + resource.getName());
    }
    dumpResourceList("ctors", desc.constructors);
    dumpResourceList("method dispatch", desc.methodDispatch);
    dumpResourceList("methods", desc.methods);
    dumpResourceList("fields", desc.fields);

  }

  private void dumpResourceList(String name, long[] resourceIds)
  {
    if (resourceIds == null)
      return;
    {
      System.out.println("  " + name + ": " + resourceIds.length);
      for (long l : resourceIds)
      {
        Resource resource = this.resourceMap.get(l);
        if (resource == null)
          System.out.println("    null");
        else
          System.out.println("    " + resource.getEntityId() + " " + resource.getName());
      }
    }
  }

  @Override
  public void verifyMembers(ClassDescriptor desc)
  {
    StringBuffer failures = new StringBuffer();
    String nl = System.lineSeparator();
    System.out.println("Verify " + desc.cls.toString());
    if (desc.methodCounter != desc.methods.length)
    {
      failures.append("  Method counter inconsistency").append(nl);
    }
    if (desc.constructorDispatch == 0 && desc.constructors != null)
    {
      failures.append("  Constructor dispatch missing").append(nl);
    }
    if (desc.constructors != null)
    {
      for (long l : desc.constructors)
      {
        if (l != 0)
          continue;

        failures.append("  Constructor is null").append(nl);
        break;
      }
    }

    for (long l : desc.methodDispatch)
    {
      if (l != 0)
        continue;

      failures.append("  Method dispatch is null").append(nl);
      break;
    }

    HashSet<Executable> methodSet = new HashSet<>(
            Collections.<Executable>unmodifiableList(
                    TypeManager.filterOverridden(desc.cls,
                            desc.cls.getDeclaredMethods())));
    for (long l : desc.methods)
    {
      if (l == 0)
      {
        failures.append("  Method is null").append(nl);
        break;
      }
      methodSet.remove(assertResource(l, MethodResource.class).getResource());
    }
    if (!methodSet.isEmpty())
    {
      for (Executable e : methodSet)
      {
        failures.append("  Unaccounted method ").append(e.toString()).append(nl);
      }
    }

    String s = failures.toString();
    if (s.isEmpty())
      return;
    System.out.println(s);
    dump(desc);
    try
    {
      Thread.sleep(200);
    } catch (InterruptedException ex)
    {
      throw new RuntimeException(ex);
    }
    throw new RuntimeException("Verify failed");
  }

  @Override
  public void failFindMethod(ClassDescriptor desc, Method requestedMethod)
  {

    System.out.println("Failed to find method:");
    System.out.println(" requested: " + requestedMethod.toString());
    System.out.println(" class: " + desc.cls.getName());
    System.out.println(" declaring class: " + requestedMethod.getDeclaringClass());
    System.out.println(" methods: " + desc.methodIndex.length);
    for (int i = 0; i < desc.methodIndex.length; ++i)
    {
      if (desc.methodIndex[i] == null)
        System.out.println("    null");
      else
        System.out.println("    " + desc.methodIndex[i].toString()
                + " " + (desc.methodIndex[i].equals(requestedMethod)));
    }
    System.out.println("  declared methods:");
    for (Method dmethod : desc.cls.getDeclaredMethods())
    {
      System.out.println("    " + dmethod.toString());
    }
    try
    {
      Thread.sleep(200);
    } catch (InterruptedException ex)
    {
      throw new RuntimeException(ex);
    }
    throw new RuntimeException("method not found " + requestedMethod);
  }
//</editor-fold>
//<editor-fold desc="inner" defaultstate="collapsed">  

  public interface Resource
  {
    String getName();

    Object getResource();

    long getEntityId();
  }

  public static class BaseResource<T> implements Resource
  {
    private String name;
    private T object;
    private long id;

    BaseResource(long id, String name, T object)
    {
      this.id = id;
      this.name = name;
      this.object = object;
    }

    @Override
    public String getName()
    {
      return this.name;
    }

    @Override
    public T getResource()
    {
      return this.object;
    }

    @Override
    public long getEntityId()
    {
      return this.id;
    }
  }

  public static class ClassResource extends BaseResource<Class>
  {
    private ClassResource(long id, String name, Class cls)
    {
      super(id, name, cls);
    }
  }

  public static class ArrayClassResource extends ClassResource
  {
    private ArrayClassResource(long id, String name, Class cls)
    {
      super(id, name, cls);
    }
  }

  public static class ObjectClassResource extends ClassResource
  {
    private ObjectClassResource(long id, String name, Class cls)
    {
      super(id, name, cls);
    }
  }

  public static class PrimitiveClassResource extends ClassResource
  {
    private PrimitiveClassResource(long id, String name, Class cls)
    {
      super(id, name, cls);
    }
  }

  public static class FieldResource extends BaseResource<Field>
  {
    private FieldResource(long id, String name, Field cls)
    {
      super(id, name, cls);
    }
  }

  public static class MethodResource extends BaseResource<Executable>
  {
    private MethodResource(long id, String name, Executable cls)
    {
      super(id, name, cls);
    }
  }

  public static class MethodDispatchResource extends BaseResource<Object>
  {
    private MethodDispatchResource(long id, String name)
    {
      super(id, name, null);
    }
  }

  public static class DeletedResource extends BaseResource<Resource>
  {
    private DeletedResource(Resource resource)
    {
      super(resource.getEntityId(), "deleted " + resource.getName(), resource);
    }
  }
//</editor-fold>
}
