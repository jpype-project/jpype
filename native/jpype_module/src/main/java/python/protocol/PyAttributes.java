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
import org.jpype.bridge.BuiltIn;
import org.jpype.bridge.Interpreter;
import python.lang.PyDict;
import python.lang.PyList;
import python.lang.PyObject;

/**
 * FIXME try to make this more like a Java map if possible, but currently it
 * can't implement the full contract.
 *
 * @author nelson85
 */
public class PyAttributes implements Map<CharSequence, PyObject>
{

  private final Backend backend;
  PyDict dict;
  private final PyObject obj;

  PyAttributes(PyObject obj)
  {
    this.obj = obj;
    this.backend = Interpreter.getBackend();
  }
  public PyDict asDict()
  {
    if (this.dict == null)
      this.dict = BuiltIn.vars(this);
    return this.dict;
  }

  @Override
  public void clear()
  {
    asDict().clear();
  }

  @Override
  public boolean containsKey(Object key)
  {
    return asDict().containsKey(key);
  }

  @Override
  public boolean containsValue(Object value)
  {
    return BuiltIn.vars(this).containsValue(value);
  }


  public PyList dir()
  {
    return BuiltIn.dir(obj);
  }

  @Override
  public Set<Entry<CharSequence, PyObject>> entrySet()
  {
    throw new UnsupportedOperationException("Not supported yet."); // Generated from nbfs://nbhost/SystemFileSystem/Templates/Classes/Code/GeneratedMethodBody
  }

  @Override
  public PyObject get(Object key)
  {
    return BuiltIn.getattr(obj, key);
  }

  /**
   * Get the value of an attribute.
   *
   * Equivalent of getattr(obj, key).
   *
   * @param key
   * @return
   */
  public PyObject get(CharSequence key)
  {
    return BuiltIn.getattr(obj, key);
  }

  @Override
  public PyObject getOrDefault(Object key, PyObject defaultValue)
  {
    return BuiltIn.getattrDefault(obj, key, defaultValue);
  }

  /**
   * Check if an attribute exists.
   *
   * Equivalent of hasattr(obj, key).
   *
   * @param key
   * @return true if the attribute exists.
   */
  public boolean contains(CharSequence key)
  {
    return BuiltIn.hasattr(obj, key);
  }

  @Override
  public boolean isEmpty()
  {
    return asDict().isEmpty();
  }

  @Override
  public Set<CharSequence> keySet()
  {
    throw new UnsupportedOperationException("Not supported yet."); // Generated from nbfs://nbhost/SystemFileSystem/Templates/Classes/Code/GeneratedMethodBody
  }

  @Override
  public PyObject put(CharSequence key, PyObject value)
  {
    return backend.setattrReturn(obj, key, value);
  }

  @Override
  public void putAll(Map<? extends CharSequence, ? extends PyObject> m)
  {
    throw new UnsupportedOperationException("Not supported yet."); // Generated from nbfs://nbhost/SystemFileSystem/Templates/Classes/Code/GeneratedMethodBody
  }
 
  @Override
  public PyObject remove(Object key)
  {
    throw new UnsupportedOperationException("Not supported yet."); // Generated from nbfs://nbhost/SystemFileSystem/Templates/Classes/Code/GeneratedMethodBody
  }

  @Override
  public int size()
  {
    return asDict().size();
  }

  @Override
  public Collection<PyObject> values()
  {
    return asDict().values();
  }

}
