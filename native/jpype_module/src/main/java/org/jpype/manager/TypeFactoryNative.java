/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package org.jpype.manager;

import java.lang.reflect.Executable;
import java.lang.reflect.Field;

/**
 * This is the interface for creating C++ object in JPype.
 * <p>
 * These methods are all native.
 * <p>
 */
public class TypeFactoryNative implements TypeFactory
{

  public long context;

  public native void newWrapper(long context, long cls);

  @Override
  public native long defineArrayClass(
          long context,
          Class cls,
          String name,
          long superClass,
          long componentPtr,
          int modifiers);

  @Override
  public native long defineObjectClass(
          long context,
          Class cls,
          String name,
          long superClass,
          long[] interfaces,
          int modifiers);

  @Override
  public native long definePrimitive(
          long context,
          String name,
          Class cls,
          long boxedPtr,
          int modifiers);

  @Override
  public native void assignMembers(
          long context,
          long cls,
          long ctorMethod,
          long[] methodList,
          long[] fieldList);

  @Override
  public native long defineField(
          long context,
          long cls,
          String name,
          Field field,
          long fieldType,
          int modifiers);

  @Override
  public native long defineMethod(
          long context,
          long cls,
          String name,
          Executable method,
          long[] overloadList,
          int modifiers);

  @Override
  public native void populateMethod(
          long context,
          long method,
          long returnType,
          long[] argumentTypes);

  @Override
  public native long defineMethodDispatch(
          long context,
          long cls,
          String name,
          long[] overloadList,
          int modifiers);

  @Override
  public native void destroy(
          long context,
          long[] resources, int sz);
}
