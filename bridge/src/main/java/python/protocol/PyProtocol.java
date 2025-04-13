/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package python.protocol;

import python.lang.PyObject;

/**
 *
 * @author nelson85
 */
public interface PyProtocol
{
    /** 
     * Return the base object protocol.
     * 
     * @return the object this protocol represents. 
     */
    PyObject asObject();
}
