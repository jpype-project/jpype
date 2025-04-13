/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package python.lang;

/**
 * FIXME try to make this more like a Java map if possible, but currently it can't 
 * implement the full contract.
 * 
 * @author nelson85
 */
public interface PyAttributes
{
    /**
     * Check if an attribute exists.
     * 
     * Equivalent of hasattr(obj, key).
     * 
     * @param key
     * @return true if the attribute exists.
     */
    boolean has(String key);
    
    /**
     * Get the value of an attribute.
     * 
     * Equivalent of getattr(obj, key).
     * 
     * @param key
     * @return 
     */
    PyObject get(String key);
    
    /**
     * Set the value of an attribute.
     * 
     * Equivalent of setattr(obj, key, value).
     * 
     * @param key
     * @return 
     */
    void set(String key, Object obj);
}
