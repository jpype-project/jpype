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

import java.net.URI;
import java.net.URISyntaxException;
import javax.tools.SimpleJavaFileObject;

/**
 * Simple representation for a source file in memory.
 */
class MemorySourceObject extends SimpleJavaFileObject
{
  private final String contents;
  private final String name;

  MemorySourceObject(String name, String contents) throws URISyntaxException
  {
    super(new URI("memory://" + name.replace(".", "/") + Kind.SOURCE.extension), Kind.SOURCE);
    this.contents = contents;
    this.name = name.replace(".", "/") + Kind.SOURCE.extension;
  }

  @Override
  public String getName()
  {
    return name;
  }

  @Override
  public CharSequence getCharContent(boolean ignoreEncodingErrors)
  {
    return contents;
  }
}
