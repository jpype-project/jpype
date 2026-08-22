// --- file: python/io/PyIOWrapperService.java ---
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
package python.io;

import org.jpype.SpiLoader;
import org.jpype.WrapperService;

/**
 * SPI provider mapping Python's {@code io}/{@code _io} classes onto the
 * {@code python.io} interfaces — a worked example of {@link WrapperService},
 * whose Javadoc documents the general mechanism.
 *
 * Covers two module names — {@code "io"} (the public facade, where the
 * abstract base classes report their {@code __module__}) and {@code "_io"}
 * (the C accelerator module, where every concrete class actually lives).
 *
 * Currently exposes {@code BytesIO}, {@code StringIO}, and their abstract
 * base classes ({@code IOBase}, {@code BufferedIOBase}, {@code TextIOBase}).
 * {@code BytesIO} and the abstract bases resolve eagerly (at interpreter
 * startup); {@code StringIO} resolves lazily, the first time an instance
 * of it actually crosses into Java.
 *
 * {@link #getResources()} scans this provider's own resource directory
 * ({@link SpiLoader#listPyspiResources}) — adding a class means dropping a
 * new {@code .pyspi} file under {@code python/io/spi/}, nothing else here
 * needs to change.
 */
public final class PyIOWrapperService implements WrapperService
{

  @Override
  public String[] getModuleNames()
  {
    return new String[]
    {
      "io", "_io"
    };
  }

  @Override
  public String getVersion()
  {
    return "1";
  }

  @Override
  public Iterable<String> getResources()
  {
    return SpiLoader.listPyspiResources(PyIOWrapperService.class, "/python/io/spi");
  }

}
