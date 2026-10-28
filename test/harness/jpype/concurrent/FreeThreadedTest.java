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
package jpype.concurrent;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/** A bounded Java frame in which Python callers can prove overlapping entry. */
public class FreeThreadedTest
{
  public static void hold(CountDownLatch entered, CountDownLatch release)
          throws InterruptedException
  {
    entered.countDown();
    if (!release.await(15, TimeUnit.SECONDS))
      throw new IllegalStateException("Java call was not released");
  }

  /** Exercise the existing Java-to-Python exception restoration path. */
  public static void rethrow(Throwable error) throws Throwable
  {
    throw error;
  }
}
