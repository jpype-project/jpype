/* ****************************************************************************
  Copyright 2019, Karl Einar Nelson

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 *****************************************************************************/
package org.jpype.compiler;

import java.io.ByteArrayOutputStream;
import java.io.OutputStream;
import java.net.URI;
import java.net.URISyntaxException;
import javax.tools.SimpleJavaFileObject;

/**
 * Simple representation of code in memory.
 */
class MemoryCodeObject extends SimpleJavaFileObject
{
  ByteArrayOutputStream baos = new ByteArrayOutputStream();

  MemoryCodeObject(String name) throws URISyntaxException
  {
    super(new URI(name + Kind.CLASS.extension), Kind.CLASS);
  }

  @Override
  public OutputStream openOutputStream()
  {
    return this.baos;
  }

  byte[] toByteArray()
  {
    return this.baos.toByteArray();
  }
}
