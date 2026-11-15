// --- file: python/decimal/PyDecimalWrapperService.java ---
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
package python.decimal;

import org.jpype.SpiLoader;
import org.jpype.WrapperService;

/**
 * SPI provider mapping Python's {@code decimal} module onto the
 * {@code python.decimal} interfaces, following the same recipe as
 * {@code python.io.PyIOWrapperService}, {@code
 * python.collections.PyCollectionsWrapperService}, {@code
 * python.datetime.PyDatetimeWrapperService}, and {@code
 * python.pathlib.PyPathlibWrapperService}.
 *
 * {@code decimal.Decimal} reports {@code "decimal"} as {@code __module__}
 * regardless of whether the C accelerator ({@code _decimal}) or the pure
 * Python fallback is in use — unlike {@code io}, there is no separate
 * accelerator module name to also register, confirmed via
 * {@code python3 -c "import decimal; print(decimal.Decimal('0').__module__)"}.
 *
 * {@link #getResources()} scans this provider's own resource directory
 * ({@link SpiLoader#listPyspiResources}) — adding a class would mean
 * dropping a new {@code .pyspi} file under {@code python/decimal/spi/},
 * though this package's scope is deliberately just the one class.
 */
public final class PyDecimalWrapperService implements WrapperService
{

  @Override
  public String[] getModuleNames()
  {
    return new String[]
    {
      "decimal"
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
    return SpiLoader.listPyspiResources(PyDecimalWrapperService.class, "/python/decimal/spi");
  }

}
