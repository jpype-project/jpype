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

import java.util.ArrayList;

/**
 *
 * @author nelson85
 */
public class Probe
{

  private Probe()
  {
  }

  static public String probe(PyObject obj)
  {
    if (obj == null)
      return "null";
    ArrayList<String> contracts = new ArrayList<>();
    if (obj instanceof PyObject)
      contracts.add("object");
    if (obj instanceof PyString)
      contracts.add("str");
    if (obj instanceof PyTuple)
      contracts.add("tuple");
    if (obj instanceof PyDict)
      contracts.add("dict");

    if (obj instanceof PyNumber)
      contracts.add("number");
    if (obj instanceof PyInt)
      contracts.add("int");
    if (obj instanceof PyFloat)
      contracts.add("float");
    if (obj instanceof PyComplex)
      contracts.add("complex");

    if (obj instanceof PyBytes)
      contracts.add("bytes");
    if (obj instanceof PyMemoryView)
      contracts.add("memoryview");
    if (obj instanceof PyJavaObject)
      contracts.add("java");
    if (obj instanceof PyAttributes)
      contracts.add("attributes");
    if (obj instanceof PyBuffer)
      contracts.add("buffer");
    if (obj instanceof PyByteArray)
      contracts.add("bytearray");
    if (obj instanceof PyCallable)
      contracts.add("callable");
    if (obj instanceof PySubscript)
      contracts.add("subscript");
    if (obj instanceof PyIndex)
      contracts.add("index");

    if (obj instanceof PyIter)
      contracts.add("iter");
    if (obj instanceof PyZip)
      contracts.add("zip");
    if (obj instanceof PyEnumerate)
      contracts.add("enumerate");
    if (obj instanceof PySlice)
      contracts.add("slice");
    if (obj instanceof PyCombinable)
      contracts.add("combinable");

    if (obj instanceof PyExc)
      contracts.add("exec");

    return String.join(", ", contracts);
  }
}
