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
public interface PyNumber extends PyProtocol
{
    int toInt();

    double toFloat();

    boolean toBool();
    
    boolean not();
    
    PyObject add(PyObject o);
    PyObject add(long o);
    PyObject add(double o);
    
    PyObject sub(PyObject o);
    PyObject sub(long o);
    PyObject sub(double o);
    
    PyObject mult(PyObject o);
    PyObject mult(long o);
    PyObject mult(double o);

    PyObject div(PyObject o);
    PyObject div(long o);
    PyObject div(double o);

    PyObject addAssign(PyObject o);
    PyObject addAssign(long o);
    PyObject addAssign(double o);
    
    PyObject subAssign(PyObject o);
    PyObject subAssign(long o);
    PyObject subAssign(double o);
    
    PyObject multAssign(PyObject o);
    PyObject multAssign(long o);
    PyObject multAssign(double o);

    PyObject divAssign(PyObject o);
    PyObject divAssign(long o);
    PyObject divAssign(double o);
    
    PyObject matMult(PyObject o);

}
