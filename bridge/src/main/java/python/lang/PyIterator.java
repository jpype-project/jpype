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
package python.lang;

import java.util.Iterator;
import java.util.NoSuchElementException;
import org.jpype.bridge.Bridge;
import org.jpype.bridge.GenericIterator;
import python.protocol.PyCallable;

/**
 * Python concept of an iterator.
 */
public interface PyIterator extends PyObject
{

    PyIterator filter(PyCallable callable);

    /**
     * Converts the Python iterator into a Java iterator.
     *
     * @return
     */
    default Iterator<PyObject> iterator()
    {
        return new GenericIterator(Bridge.getBackend().tee(this));
    }

    /**
     * Get the next item.
     *
     * FIXME This throws StopIteration, we need to figure out how to convert and
     * catch it.
     *
     * @return the next element in the series.
     */
    default PyObject next()
    {
        PyObject out = Bridge.getBackend().next(this, Bridge.stop);
        if (out.equals(Bridge.stop))
            throw new NoSuchElementException();
        return out;
    }

    /**
     * Get the next item.
     *
     * @param defaults is the element to return if there is no additional
     * elements.
     * @return the next element in the series.
     */
    default PyObject next(PyObject defaults)
    {
        return Bridge.getBackend().next(this, defaults);
    }

}
