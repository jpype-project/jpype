// --- file: python/lang/PyMutableSet.java ---
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
 * A Python mutable {@code set}: supports adding and removing elements in
 * addition to the read-only operations of {@link PyAbstractSet}.
 *
 * @param <T> the element type.
 */
public interface PyMutableSet<T extends PyObject> extends PyAbstractSet<T>
{

  @Bypass
  @Override
  default boolean contains(Object obj)
  {
    return builtin().backend.contains(this, obj);
  }

  @Bypass
  @Override
  default int size()
  {
    return builtin().len(this);
  }
}
