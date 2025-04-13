/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package org.jpype.bridge;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.Set;
import python.protocol.PyMapping;

/**
 *  Representation of a KeySet of Python Mappings.
 */
public class KeySet implements Set<Object>
{

    final PyMapping obj;
    
    public KeySet(PyMapping mapping)
    {
        this.obj = mapping;
    }

    @Override
    public int size()
    {
        return obj.size();
    }

    @Override
    public boolean isEmpty()
    {
        return obj.isEmpty();
    }

    @Override
    public boolean contains(Object o)
    {
        return obj.containsKey(o);
    }

    @Override
    public Iterator<Object> iterator()
    {
        // FIXME this one is required
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public Object[] toArray()
    {
        return new ArrayList(this).toArray();
    }

    @Override
    public <T> T[] toArray(T[] a)
    {
        return (T[]) new ArrayList(this).toArray(a);
    }

    @Override
    public boolean add(Object e)
    {
        throw new UnsupportedOperationException(); 
    }

    @Override
    public boolean remove(Object o)
    {
        return obj.remove(o) != null;
    }

    @Override
    public boolean containsAll(Collection<?> c)
    {
        for (var x : c)
        {
            if (!obj.containsKey(c))
                return false;
        }
        return true;
    }

    @Override
    public boolean addAll(Collection<? extends Object> c)
    {
        throw new UnsupportedOperationException(); 
    }

    @Override
    public boolean retainAll(Collection<?> c)
    {
        throw new UnsupportedOperationException(); 
    }

    @Override
    public boolean removeAll(Collection<?> c)
    {
        boolean y = true;
        for (var x : c)
        {
            y &= this.remove(x);
        }
        return y;
    }

    @Override
    public void clear()
    {
        obj.clear();
    }

}
