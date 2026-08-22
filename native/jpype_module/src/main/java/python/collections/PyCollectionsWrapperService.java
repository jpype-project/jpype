// --- file: python/collections/PyCollectionsWrapperService.java ---
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
package python.collections;

import org.jpype.SpiLoader;
import org.jpype.WrapperService;

/**
 * SPI provider mapping Python's {@code collections} module classes onto
 * the {@code python.collections} interfaces, following the same recipe as
 * {@code python.io.PyIOWrapperService}.
 *
 * Every class in {@code collections} (unlike {@code io}'s {@code io}/
 * {@code _io} split) reports {@code "collections"} as its {@code __module__}
 * — {@code deque}, {@code Counter}, {@code OrderedDict}, and
 * {@code defaultdict} all live directly in that one module, so a single
 * module name covers all four.
 *
 * {@link #getResources()} scans this provider's own resource directory
 * ({@link SpiLoader#listPyspiResources}) — adding a class means dropping a
 * new {@code .pyspi} file under {@code python/collections/spi/}, nothing
 * else here needs to change.
 */
public final class PyCollectionsWrapperService implements WrapperService
{

  @Override
  public String[] getModuleNames()
  {
    return new String[]
    {
      "collections"
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
    return SpiLoader.listPyspiResources(PyCollectionsWrapperService.class, "/python/collections/spi");
  }

}
