/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package python.protocol;

import org.jpype.bridge.Context;
import python.lang.PyDict;
import python.lang.PyObject;
import python.lang.PyTuple;
import python.protocol.PyProtocol;

/**
 * Interface for Python objects that act as a callable.
 * 
 * To allow for overloading, the entry point must be private.
 *
 */
public interface PyCallable extends PyProtocol
{
    
    default PyObject call(PyTuple args, PyDict kwargs)
    {
        return _call(args, kwargs);
    }
            
    default PyObject call(PyTuple args)
    {
        return _call(args, null);
    }
    
    default PyObject call(Object... args)
    {
        return _call(Context.tuple(args), null);
    }
    
    /** 
     * Actual interface used for dispatch.
     * 
     * @param args
     * @param kwargs
     * @return 
     */
    PyObject _call(PyTuple args, PyDict kwargs);
    
}
