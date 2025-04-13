/* ****************************************************************************
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
 * ***************************************************************************/
package python.lang;

import java.util.Collection;
import java.util.Map;
import java.util.Set;

/**
 *
 * @author nelson85
 */
public interface PyDict extends PyObject, Map<PyObject, Object>
{

    @Override
    public int size();

    @Override
    public boolean isEmpty();

    @Override
    public boolean containsKey(Object key);

    @Override
    public boolean containsValue(Object value);

    @Override
    public PyObject get(Object key);

    @Override
    public PyObject put(PyObject key, Object value);

    @Override
    public PyObject remove(Object key);

    @Override
    public void putAll(Map<? extends PyObject, ? extends Object> m);

    @Override
    public void clear();

    @Override
    public Set<PyObject> keySet();

    @Override
    public Collection<Object> values();

    @Override
    public Set<Entry<PyObject, Object>> entrySet();
  
}
