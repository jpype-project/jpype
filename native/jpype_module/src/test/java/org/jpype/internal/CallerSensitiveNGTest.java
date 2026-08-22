// --- file: org/jpype/internal/CallerSensitiveNGTest.java ---
/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package org.jpype.internal;

import python.lang.PyObject;
import python.lang.PyTestHarness;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Exercises JPMethod::invokeCallerSensitive (native/common/jp_method.cpp),
 * the only call path that reaches {@code Reflector0.callMethod} (see its
 * class Javadoc for why it's loaded specially from META-INF/versions/0).
 * That path is taken only for Java methods annotated
 * {@code @jdk.internal.reflect.CallerSensitive} - {@code Class.forName}
 * is one - as opposed to the ordinary JNI call path used for everything
 * else, which is why it needed a dedicated test rather than being covered
 * incidentally by the rest of the suite.
 */
public class CallerSensitiveNGTest extends PyTestHarness
{

  @Test
  public void testCallerSensitiveMethodInvocation()
  {
    PyObject result = context.eval(
            "__import__('jpype').JClass('java.lang.Class').forName('java.lang.String')");
    assertNotNull(result);
  }

}
