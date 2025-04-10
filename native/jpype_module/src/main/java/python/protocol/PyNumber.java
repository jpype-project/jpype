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

import python.lang.PyObject;

/**
 *
 * @author nelson85
 */
public interface PyNumber extends PyProtocol
{

  PyObject add(PyObject o);

  PyObject add(long o);

  PyObject add(double o);

  PyObject addAssign(PyObject o);

  PyObject addAssign(long o);

  PyObject addAssign(double o);

  PyObject div(PyObject o);

  PyObject div(long o);

  PyObject div(double o);

  PyObject divAssign(PyObject o);

  PyObject divAssign(long o);

  PyObject divAssign(double o);

  PyObject divMod(PyObject o);

  PyObject matMult(PyObject o);

  PyObject mult(PyObject o);

  PyObject mult(long o);

  PyObject mult(double o);

  PyObject multAssign(PyObject o);

  PyObject multAssign(long o);

  PyObject multAssign(double o);

  boolean not();

  PyObject pow(PyObject o);

  PyObject remainder(PyObject o);

  PyObject sub(PyObject o);

  PyObject sub(long o);

  PyObject sub(double o);

  PyObject subAssign(PyObject o);

  PyObject subAssign(long o);

  PyObject subAssign(double o);

  boolean toBool();

  double toFloat();

  int toInt();

}
