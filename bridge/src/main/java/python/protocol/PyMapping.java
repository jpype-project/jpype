/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
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
