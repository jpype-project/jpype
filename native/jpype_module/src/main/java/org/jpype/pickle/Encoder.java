/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package org.jpype.pickle;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;

/**
 *
 * @author Karl Einar Nelson
 */
public class Encoder
{

  ByteArrayOutputStream baos;
  ObjectOutputStream oos;

  public Encoder() throws IOException
  {
    baos = new ByteArrayOutputStream();
    oos = new ObjectOutputStream(baos);
  }

  public byte[] pack(Object obj) throws IOException
  {
    oos.writeObject(obj);
    oos.flush();
    byte[] out = baos.toByteArray().clone();
    baos.reset();
    return out;
  }
}
