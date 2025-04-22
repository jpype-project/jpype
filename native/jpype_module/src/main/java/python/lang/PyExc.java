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

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.logging.Level;
import java.util.logging.Logger;
import python.exception.PyException;
import static python.lang.PyExceptionFactory.LOOKUP;

/**
 * Native version of a Python exception.
 *
 * This will be the type that is unwrapped to in Python.
 *
 * @author nelson85
 */
public interface PyExc extends PyObject
{

  /** 
   * Wraps a Python exception with the appropriate Java wrapper type.
   * 
   * @param base
   * @return 
   */
  static Exception of(PyExc base)
  {
System.out.println("PyExc.of");
    PyType type = base.getType();
    String name = type.getName();
System.out.println("PyExc.of "+name);
    Class cls = LOOKUP.get(name);
    if (cls == null)
    {
      PyTuple mro = type.mro();
      int sz = mro.size();
      for (int i = 0; i < sz; ++i)
      {
        mro.get(i); // FIXME we have the wrong wrapper type here until we fix the probe method
      }
      cls = PyException.class;
    }
    try
    {
      Constructor<? extends PyException> ctor = cls.getDeclaredConstructor(PyExc.class);
      return ctor.newInstance(base);
    } catch (NoSuchMethodException | SecurityException | InstantiationException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex)
    {
      Logger.getLogger(PyExc.class.getName()).log(Level.SEVERE, null, ex);
      return new RuntimeException("Unable to find Python error type " + name);
    }
  }
  
  /** 
   * Used to pass an exception through the Python stack.
   * 
   * @param th
   * @return 
   */
  static public PyExc unwrap(Throwable th)
  {    
    if (th instanceof PyException)
      return ((PyException) th).get();
    return null;
  }

  String getMessage();

}
