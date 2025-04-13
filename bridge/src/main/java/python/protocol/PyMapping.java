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
import python.lang.PyObject;

/**
 *
 * @author nelson85
 */
public interface PyMapping extends PyProtocol, Map<Object, PyObject>
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
    public PyObject put(Object key, PyObject value);

    @Override
    public PyObject remove(Object key);
    
    @Override
    public void putAll(Map<? extends Object, ? extends PyObject> m);

    @Override
    public void clear();
    
    @Override
    public Set<Object> keySet();

    @Override
    public Collection<PyObject> values();
    
    @Override
    public Set<Entry<Object, PyObject>> entrySet();
}
