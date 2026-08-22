// --- file: python/lang/PySized.java ---
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

import org.jpype.annotation.Bypass;

/**
 * Protocol for Python objects are sized.
 */
public interface PySized extends PyObject
{

  @Bypass
  default int size()
  {
    return builtin().len(this);
  }

  @Bypass
  default boolean isEmpty()
  {
    return size() == 0;
  }

}
