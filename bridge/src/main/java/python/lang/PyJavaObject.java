package python.lang;

/** ***************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * See NOTICE file for details.
 **************************************************************************** */


/**
 *  This is a wrapper for a Java object which was returned from Python.
 * 
 * @author nelson85
 */
public class PyJavaObject implements PyObject
{
  Object obj_;

  public PyJavaObject(Object obj)
  {
    this.obj_ = obj;
  }
  
  public Object get()
  {
    return obj_;
  }

  @Override
  public boolean hasAttr(String s)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public PyObject getAttr(String s)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public void setAttr(String s, Object obj)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public void delAttr(String s)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public PyObject call(PyTuple args, PyDict kwargs)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public PyObject setItem(PyObject key, Object value)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public PyObject getItem(PyObject key)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public void delItem(PyObject obj)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public int len()
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public PyObject iter()
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean isInstance(PyObject cls)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public PyObject type()
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public PyObject dir()
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean not()
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean isTrue()
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public int hash()
  {
    return obj_.hashCode();
  }

  @Override
  public double asFloat()
  {
    if (obj_ instanceof Number)
      return ((Number)obj_).doubleValue();
    throw new UnsupportedOperationException();
  }

  @Override
  public int asInt()
  {
    if (obj_ instanceof Number)
      return ((Number)obj_).intValue();
    throw new UnsupportedOperationException();
  }

  @Override
  public PyObject str()
  {
    if (obj_ == null)
      return null;
    return new PyJavaObject(obj_.toString());
  }

  @Override
  public PyObject repr()
  {
    return str();
  }

  @Override
  public PyObject bytes()
  {
    throw new UnsupportedOperationException(); 
  }
  
}
