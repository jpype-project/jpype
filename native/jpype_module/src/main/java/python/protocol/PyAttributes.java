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
package python.protocol;

import java.util.Collection;
import java.util.Map;
import java.util.Set;
import org.jpype.bridge.Backend;
import python.lang.PyBuiltIn;
import org.jpype.bridge.Interpreter;
import python.lang.PyDict;
import python.lang.PyDictItems;
import python.lang.PyDictKeySet;
import python.lang.PyList;
import python.lang.PyObject;

/**
 * A {@link Map}-like implementation for accessing and manipulating Python
 * object attributes.
 *
 * The {@code PyAttributes} class provides a Java interface for interacting with
 * the attributes of a Python object. It acts as a bridge between Python's
 * attribute handling and Java's {@link Map} interface, allowing attributes to
 * be accessed, modified, and queried in a Java-friendly manner.
 *
 * <p>
 * Key Features:</p>
 * <ul>
 * <li>Supports retrieving attributes using {@code get()} and
 * {@code getOrDefault()}.</li>
 * <li>Allows setting attributes using {@code put()}.</li>
 * <li>Provides methods for checking attribute existence with
 * {@code contains()}.</li>
 * <li>Integrates with Python's {@code dir()} and {@code vars()} functions.</li>
 * </ul>
 *
 * <p>
 * Usage Example:</p>
 * <pre>
 * PyObject obj = BuiltIn.dict();  // Create a Python object
 * PyAttributes attributes = new PyAttributes(obj);
 *
 * // Access attributes
 * PyObject value = attributes.get("key");
 *
 * // Set attributes
 * attributes.put("key", BuiltIn.str("value"));
 *
 * // Check existence
 * boolean exists = attributes.contains("key");
 *
 * // Clear all attributes
 * attributes.clear();
 * </pre>
 *
 */
public class PyAttributes implements Map<CharSequence, PyObject>
{

  /**
   * Backend implementation for interacting with Python objects.
   */
  private final Backend backend;

  /**
   * The Python object whose attributes are being managed.
   */
  private final PyObject obj;

  /**
   * Cached dictionary representation of the object's attributes.
   */
  private PyDict dict;

  /**
   * Constructs a new {@code PyAttributes} instance for the given Python object.
   *
   * @param obj The Python object whose attributes are to be accessed and
   * manipulated.
   */
  public PyAttributes(PyObject obj)
  {
    this.obj = obj;
    this.backend = Interpreter.getBackend();
  }

  /**
   * Returns the dictionary representation of the object's attributes.
   *
   * <p>
   * This method uses Python's {@code vars()} function to retrieve the
   * attributes as a {@link PyDict}. The dictionary is cached for
   * performance.</p>
   *
   * @return A {@link PyDict} containing the object's attributes.
   */
  public PyDict asDict()
  {
    if (this.dict == null)
    {
      this.dict = PyBuiltIn.vars(this);
    }
    return this.dict;
  }

  /**
   * Clears all attributes of the Python object.
   *
   * <p>
   * This method removes all attributes from the object by clearing the
   * dictionary representation.</p>
   */
  @Override
  public void clear()
  {
    asDict().clear();
  }

  /**
   * Checks whether the Python object has an attribute with the specified key.
   *
   * @param key The name of the attribute to check.
   * @return {@code true} if the attribute exists, {@code false} otherwise.
   */
  @Override
  public boolean containsKey(Object key)
  {
    return asDict().containsKey(key);
  }

  /**
   * Checks whether the Python object has an attribute with the specified value.
   *
   * @param value The value to check for.
   * @return {@code true} if the value exists, {@code false} otherwise.
   */
  @Override
  public boolean containsValue(Object value)
  {
    return PyBuiltIn.vars(this).containsValue(value);
  }

  /**
   * Returns a list of all attribute names of the Python object.
   *
   * <p>
   * This method uses Python's {@code dir()} function to retrieve the list of
   * attribute names.</p>
   *
   * @return A {@link PyList} containing the names of all attributes.
   */
  public PyList dir()
  {
    return PyBuiltIn.dir(obj);
  }

  @Override
  public Set<Entry<CharSequence, PyObject>> entrySet()
  {
    return new PyDictItems(this.asDict());
  }

  /**
   * Retrieves the value of the specified attribute.
   *
   * <p>
   * This method is equivalent to Python's {@code getattr(obj, key)}.</p>
   *
   * @param key The name of the attribute to retrieve.
   * @return The value of the attribute.
   */
  @Override
  public PyObject get(Object key)
  {
    return PyBuiltIn.getattr(obj, key);
  }

  /**
   * Retrieves the value of the specified attribute, or a default value if the
   * attribute does not exist.
   *
   * <p>
   * This method is equivalent to Python's
   * {@code getattr(obj, key, defaultValue)}.</p>
   *
   * @param key The name of the attribute to retrieve.
   * @param defaultValue The default value to return if the attribute does not
   * exist.
   * @return The value of the attribute, or {@code defaultValue} if the
   * attribute does not exist.
   */
  @Override
  public PyObject getOrDefault(Object key, PyObject defaultValue)
  {
    return PyBuiltIn.getattrDefault(obj, key, defaultValue);
  }

  /**
   * Checks whether the Python object has an attribute with the specified name.
   *
   * <p>
   * This method is equivalent to Python's {@code hasattr(obj, key)}.</p>
   *
   * @param key The name of the attribute to check.
   * @return {@code true} if the attribute exists, {@code false} otherwise.
   */
  public boolean contains(CharSequence key)
  {
    return PyBuiltIn.hasattr(obj, key);
  }

  /**
   * Checks whether the Python object has no attributes.
   *
   * @return {@code true} if the object has no attributes, {@code false}
   * otherwise.
   */
  @Override
  public boolean isEmpty()
  {
    return asDict().isEmpty();
  }

  @Override
  public Set<CharSequence> keySet()
  {
    return new PyDictKeySet(asDict());
  }

  /**
   * Sets the value of the specified attribute.
   *
   * <p>
   * This method is equivalent to Python's {@code setattr(obj, key, value)}.</p>
   *
   * @param key The name of the attribute to set.
   * @param value The value to associate with the attribute.
   * @return The previous value of the attribute, or {@code null} if no previous
   * value existed.
   */
  @Override
  public PyObject put(CharSequence key, PyObject value)
  {
    return backend.setattrReturn(obj, key, value);
  }

  /**
   * Unsupported operation for adding multiple attributes.
   *
   * @param map is the map of attributes to add.
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public void putAll(Map<? extends CharSequence, ? extends PyObject> map)
  {
    for (var v : map.entrySet())
    {
      backend.setattrString(this.obj, v.getKey(), v.getValue());
    }
  }

  /**
   * Unsupported operation for removing an attribute.
   *
   * @param key The name of the attribute to remove.
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public PyObject remove(Object key)
  {
    return backend.delattrReturn(this.obj, key);
  }

  /**
   * Returns the number of attributes of the Python object.
   *
   * @return The number of attributes.
   */
  @Override
  public int size()
  {
    return asDict().size();
  }

  /**
   * Returns a collection of all attribute values of the Python object.
   *
   * @return A {@link Collection} containing all attribute values.
   */
  @Override
  public Collection<PyObject> values()
  {
    return asDict().values();
  }
}
