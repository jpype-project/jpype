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

import java.io.IOException;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.List;
import javax.tools.FileObject;
import javax.tools.ForwardingJavaFileManager;
import javax.tools.JavaFileManager;
import javax.tools.JavaFileObject;
import javax.tools.JavaFileObject.Kind;

/**
 *
 * Minimal file manager for collecting code objects produced by the compiler.
 */
class MemoryFileManager extends ForwardingJavaFileManager<JavaFileManager>
{
  List<MemoryCodeObject> code = new ArrayList<>();

  MemoryFileManager(JavaFileManager jfm)
  {
    super(jfm);
  }

  @Override
  public JavaFileObject getJavaFileForOutput(JavaFileManager.Location location, String className, Kind kind, FileObject sibling) throws IOException
  {
    try
    {
      if (kind != Kind.CLASS)
        throw new RuntimeException("Only type code supported");
      MemoryCodeObject code = new MemoryCodeObject(className);
      this.code.add(code);
      return code;
    } catch (URISyntaxException ex)
    {
      throw new RuntimeException(ex);
    }
  }

  public List<MemoryCodeObject> getCode()
  {
    return this.code;
  }
}
