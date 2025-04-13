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

/**
 * Java front end for concrete Python bytearray.
 * 
 * FIXME can we get Java set behavior for this object?
 * FIXME need additional dunder methods.
 */
public interface PySet extends PyObject
{

    int len();

    PySet isDisjoint(PySet set);

    PySet isSubset(PySet set);

    PySet isSuperset(PySet set);

    PySet union(PySet... set);

    PySet intersect(PySet... set);

    PySet difference(PySet... set);

    PySet symmetricDifference(PySet... set);

    /**
     * Shallow copy.
     *
     * @return a new set
     */
    PySet copy();
}
