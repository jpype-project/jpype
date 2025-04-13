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
package org.jpype.bridge;

import java.util.Iterator;
import java.util.NoSuchElementException;
import python.lang.PyIterator;
import python.lang.PyObject;

/**
 * Conversion of a Python iterator to Java.
 */
public class GenericIterator implements Iterator<PyObject>
{

    private final PyIterator iter;
    private PyObject yield;
    private boolean done = false;
    private boolean check = false;
    

    public GenericIterator(PyIterator iter)
    {
        this.iter = iter;
    }

    @Override
    public boolean hasNext()
    {
        if (done) 
            return false;
        if (check)
            return !done;
        check = true;
        if (yield== null)
            yield = Bridge.backend.next(iter, Bridge.stop);
        done = (yield == Bridge.stop);
        return !done;
    }

    @Override
    public PyObject next() throws NoSuchElementException
    {
        if (!check)
            hasNext();
        if (done) throw new  NoSuchElementException();
        check = false;
        return yield;
    }
    
}
