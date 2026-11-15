// --- file: python/pathlib/PyPathlibWrapperService.java ---
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
package python.pathlib;

import org.jpype.SpiLoader;
import org.jpype.WrapperService;

/**
 * SPI provider mapping Python's {@code pathlib} module classes onto the
 * {@code python.pathlib} interfaces, following the same recipe as
 * {@code python.io.PyIOWrapperService}, {@code
 * python.collections.PyCollectionsWrapperService}, and {@code
 * python.datetime.PyDatetimeWrapperService}.
 *
 * {@code pathlib.Path()} always dispatches to one of two concrete platform
 * subclasses, {@code PosixPath} or {@code WindowsPath} — a bare
 * {@code Path} instance is never actually constructed — but both report
 * {@code "pathlib"} as {@code __module__}, so a single module name still
 * covers both; see {@code pathlib.PosixPath.pyspi} /
 * {@code pathlib.WindowsPath.pyspi}.
 *
 * {@link #getResources()} scans this provider's own resource directory
 * ({@link SpiLoader#listPyspiResources}) — adding a class means dropping a
 * new {@code .pyspi} file under {@code python/pathlib/spi/}, nothing else
 * here needs to change.
 */
public final class PyPathlibWrapperService implements WrapperService
{

  @Override
  public String[] getModuleNames()
  {
    return new String[]
    {
      "pathlib"
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
    return SpiLoader.listPyspiResources(PyPathlibWrapperService.class, "/python/pathlib/spi");
  }

}
