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
 * Interface for creating and managing resources used by JPype.
 *
 * The {@code TypeFactory} interface defines methods for constructing various
 * JPype-related types and resources, such as wrapper types, array classes,
 * object classes, primitive types, methods, fields, and dispatch mechanisms. It
 * serves as a bridge between the Java layer and the C++ layer, enabling
 * efficient resource creation and management without requiring direct probing
 * of Java from C++.
 *
 * This interface is also designed to facilitate testing by providing a clear
 * contract for resource creation and destruction.
 *
 * <p>
 * Key responsibilities:
 * <ul>
 * <li>Define and create JPype-specific types and classes</li>
 * <li>Assign members (fields and methods) to classes</li>
 * <li>Manage method dispatch and field definitions</li>
 * <li>Destroy resources when no longer needed</li>
 * </ul>
 *
 */
public interface TypeFactory
{

  //<editor-fold desc="class" defaultstate="collapsed">
  /**
   * Creates a new wrapper type for Python.
   *
   * This method constructs a wrapper type that can be used to interface with
   * Python.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param cls A pointer to the Java class (JClass) to wrap.
   */
  void newWrapper(long context, long cls);

  /**
   * Creates a JPArray class.
   *
   * This method defines a new array class in JPype, including its type
   * information, superclass, component type, and modifiers.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param cls The Java class type for the array.
   * @param name The name of the array class.
   * @param superClass A pointer to the superclass of the array class.
   * @param componentPtr A pointer to the component type of the array.
   * @param modifiers The modifiers for the array class (e.g., public, private,
   * etc.).
   * @return A pointer to the newly created JPArrayClass.
   */
  long defineArrayClass(
          long context,
          Class cls,
          String name,
          long superClass,
          long componentPtr,
          int modifiers);

  /**
   * Creates a JPObject class.
   *
   * This method defines a new object class in JPype, including its type
   * information, superclass, interfaces, and modifiers.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param cls The Java class type for the object.
   * @param name The name of the object class.
   * @param superClass A pointer to the superclass of the object class.
   * @param interfaces An array of pointers to the interfaces implemented by the
   * object class.
   * @param modifiers The modifiers for the object class (e.g., public, private,
   * etc.).
   * @return A pointer to the newly created JPObjectClass.
   */
  long defineObjectClass(
          long context,
          Class cls,
          String name,
          long superClass,
          long[] interfaces,
          int modifiers);

  /**
   * Defines a primitive type.
   *
   * This method creates a JPype representation of a Java primitive type,
   * including its boxed type and modifiers.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param name The name of the primitive type.
   * @param cls The Java class representing the primitive type.
   * @param boxedPtr A pointer to the JPClass representing the boxed type.
   * @param modifiers The modifiers for the primitive type.
   * @return A pointer to the newly defined primitive type.
   */
  long definePrimitive(
          long context,
          String name,
          Class cls,
          long boxedPtr,
          int modifiers);

  //</editor-fold>
  //<editor-fold desc="members" defaultstate="collapsed">
  /**
   * Assigns members (fields and methods) to a class.
   *
   * This method populates the specified JPClass with its constructor, methods,
   * and fields.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param cls A pointer to the JPClass to populate.
   * @param ctorMethod A pointer to the JPMethod representing the constructor.
   * @param methodList An array of pointers to JPMethod objects representing the
   * methods.
   * @param fieldList An array of pointers to JPField objects representing the
   * fields.
   */
  void assignMembers(
          long context,
          long cls,
          long ctorMethod,
          long[] methodList,
          long[] fieldList);

  /**
   * Defines a field.
   *
   * This method creates a JPField for the specified class.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param cls A pointer to the class owning the field.
   * @param name The name of the field.
   * @param field The Java {@link Field} object representing the field.
   * @param fieldType A pointer to the JPClass representing the field's type.
   * @param modifiers The modifiers for the field (e.g., public, private, etc.).
   * @return A pointer to the newly defined JPField.
   */
  long defineField(
          long context,
          long cls,
          String name,
          Field field,
          long fieldType,
          int modifiers);

  /**
   * Defines a method.
   *
   * This method creates a JPMethod for the specified class.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param cls A pointer to the class owning the method.
   * @param name The name of the method.
   * @param method The Java {@link Executable} object representing the method.
   * @param overloadList An array of pointers to overloads for the method.
   * @param modifiers The modifiers for the method (e.g., public, private,
   * static, etc.).
   * @return A pointer to the newly defined JPMethod.
   */
  long defineMethod(
          long context,
          long cls,
          String name,
          Executable method,
          long[] overloadList,
          int modifiers);

  /**
   * Populates a method with its return type and argument types.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param method A pointer to the JPMethod to populate.
   * @param returnType A pointer to the JPClass representing the return type of
   * the method.
   * @param argumentTypes An array of pointers to JPClass objects representing
   * the argument types.
   */
  void populateMethod(
          long context,
          long method,
          long returnType,
          long[] argumentTypes);

  /**
   * Defines a method dispatch for Python by name.
   *
   * This method creates a JPMethodDispatch for the specified class and method
   * name.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param cls A pointer to the class owning the dispatch.
   * @param name The name of the dispatch.
   * @param overloadList An array of pointers to overloads for the dispatch.
   * @param modifiers The modifiers for the dispatch (e.g., constructor, static,
   * etc.).
   * @return A pointer to the newly defined JPMethodDispatch.
   */
  long defineMethodDispatch(
          long context,
          long cls,
          String name,
          long[] overloadList,
          int modifiers);

  //</editor-fold>
  //<editor-fold desc="destroy" defaultstate="collapsed">
  /**
   * Destroys resources.
   *
   * This method releases the specified resources in the JPype context.
   *
   * @param context The JPContext object representing the current JPype context.
   * @param resources An array of pointers to the resources to destroy.
   * @param sz The size of the resources array.
   */
  void destroy(
          long context,
          long[] resources, int sz);

  //</editor-fold>
}
