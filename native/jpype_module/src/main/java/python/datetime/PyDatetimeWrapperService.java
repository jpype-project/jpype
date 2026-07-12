// --- file: python/datetime/PyDatetimeWrapperService.java ---
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
package python.datetime;

import org.jpype.SpiLoader;
import org.jpype.WrapperService;

/**
 * SPI provider mapping Python's {@code datetime} module classes onto the
 * {@code python.datetime} interfaces, following the same recipe as
 * {@code python.io.PyIOWrapperService} and
 * {@code python.collections.PyCollectionsWrapperService}.
 *
 * Every class in {@code datetime} ({@code date}, {@code datetime},
 * {@code timedelta}, {@code time}) reports {@code "datetime"} as its
 * {@code __module__} — there is no {@code io}-style C-accelerator split to
 * account for — so a single module name covers all of them.
 *
 * {@link #getResources()} scans this provider's own resource directory
 * ({@link SpiLoader#listPyspiResources}) — adding a class means dropping a
 * new {@code .pyspi} file under {@code python/datetime/spi/}, nothing else
 * here needs to change.
 */
public final class PyDatetimeWrapperService implements WrapperService
{

  @Override
  public String[] getModuleNames()
  {
    return new String[]
    {
      "datetime"
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
    return SpiLoader.listPyspiResources(PyDatetimeWrapperService.class, "/python/datetime/spi");
  }

}
