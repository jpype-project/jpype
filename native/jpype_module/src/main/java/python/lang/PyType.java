/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 * 
 *  See NOTICE file for details.
 */
package python.lang;

import python.protocol.PyCallable;

/**
 * Represents a Java front-end for a concrete Python type.
 *
 * This interface provides methods to interact with Python types in Java,
 * mimicking Python's type object functionality. It includes methods for
 * retrieving type information, checking type relationships, and accessing
 * attributes and methods defined on the type.
 *
 */
public interface PyType extends PyObject, PyCallable
{

  /**
   * Returns the Python type object for the "zip" built-in function.
   *
   * This is a static utility method to demonstrate how Python types can be
   * evaluated and accessed from Java.
   *
   * @return the PyType object corresponding to the "zip" function.
   */
  static PyType getType()
  {
    return (PyType) PyBuiltIn.eval("zip", null, null);
  }

  /**
   * Retrieves the name of the type.
   *
   * This corresponds to the Python `__name__` attribute, which represents the
   * name of the type as a string.
   *
   * @return the name of the type as a String.
   */
  String getName();

  /**
   * Retrieves the method resolution order (MRO) of the type.The MRO defines the
   * order in which base classes are searched when resolving methods and
   * attributes.
   *
   * This corresponds to the Python `__mro__` attribute.
   *
   * @return a PyTuple representing the MRO of the getType.
   */
  PyTuple mro();

  /**
   * Retrieves the base class of the type.
   *
   * This corresponds to the Python `__base__` attribute, which represents the
   * immediate base class of the type.
   *
   * @return the base class as a PyType.
   */
  PyType getBase();

  /**
   * Retrieves a tuple of all base classes for the type.This corresponds to the
   * Python `__bases__` attribute, which contains all base classes of the type.
   *
   *
   * @return a PyTuple containing the base classes of the getType.
   */
  PyTuple getBases();

  /**
   * Checks if the current type is a subclass of the specified type.This
   * corresponds to Python's `issubclass()` function and allows checking type
   * relationships.
   *
   *
   * @param type The getType to check against.
   * @return true if this getType is a subclass of the specified getType, false
   * otherwise.
   */
  boolean isSubclassOf(PyType type);

  /**
   * Checks if the given object is an instance of this type.This corresponds to
   * Python's `isinstance()` function and allows checking if an object belongs
   * to the current type.
   *
   *
   * @param obj The object to check.
   * @return true if the object is an instance of this getType, false otherwise.
   */
  boolean isInstance(PyObject obj);

  /**
   * Retrieves a callable method by name from the type.
   *
   * This allows accessing methods defined on the type by their name.
   *
   * @param name The name of the method to retrieve.
   * @return the callable method as a PyCallable, or null if the method does not
   * exist.
   */
  PyCallable getMethod(String name);

  /**
   * Checks if the type is abstract.An abstract type is one that contains
   * abstract methods and cannot be instantiated directly.
   *
   * This corresponds to Python's `abc` module behavior.
   *
   * @return true if the getType is abstract, false otherwise.
   */
  boolean isAbstract();

  /**
   * Retrieves a list of subclasses of the type.This corresponds to Python's
   * `__subclasses__()` method, which returns all known subclasses of the type.
   *
   *
   * @return a PyList containing the subclasses of the getType.
   */
  PyList getSubclasses();
}
