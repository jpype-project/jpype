/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package python.lang;

/**
 * Interface for Python objects that act as a function.
 *
 * @author nelson85
 */
public interface PyCallable
{
    PyObject call(PyObject args, PyDict kwargs);
    
}
