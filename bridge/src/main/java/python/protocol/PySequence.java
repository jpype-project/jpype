/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package python.protocol;

import python.lang.PyObject;

/**
 * Interface for objects that act as sequences.
 */
public interface PySequence extends PyProtocol
{

    /**
     * Get the item by index.
     *
     * Equivalent to getitem(obj, index).
     *
     * @param index
     * @return
     */
    PyObject get(int index);

    /**
     * Set an item by index.
     *
     * Equivalent to setitem(obj, index).
     *
     * @param index
     * @param value
     */
    void set(int index, Object value);

    /**
     * Delete an item by index.
     *
     * Equivalent to delitem(obj, index).
     *
     * @param index
     */
    void remove(int index);

    /**
     * Delete an item by index.
     *
     * Equivalent to len(obj).
     *
     * @return
     */
    int size();

}
