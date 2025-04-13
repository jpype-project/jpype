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

import org.jpype.bridge.Context;
import python.lang.PyDict;
import python.lang.PyObject;
import python.lang.PyTuple;

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
