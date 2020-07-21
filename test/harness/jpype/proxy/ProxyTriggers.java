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
package jpype.proxy;

import java.util.LinkedList;
import java.util.List;

public class ProxyTriggers
{

  public static String[] testProxy(Object itf)
  {
    List<String> methods = new LinkedList<>();
    if (itf instanceof TestInterface1)
    {
      methods.add("Test Method1 = " + ((TestInterface1) itf).testMethod1());
    }
    if (itf instanceof TestInterface2)
    {
      methods.add("Test Method2 = " + ((TestInterface2) itf).testMethod2());
    }
    if (itf instanceof TestInterface3)
    {
      methods.add("Test Method3 = " + ((TestInterface3) itf).testMethod3());
    }
    return methods.toArray(new String[0]);
  }

  public void testProxyWithThread(final TestThreadCallback itf)
  {
    itf.notifyValue("Waiting for thread start");
    Thread t = new Thread(new Runnable()
    {
      public void run()
      {
        for (int i = 1; i <= 3; i++)
        {
          itf.notifyValue(String.valueOf(i));
        }

      }
    });
    t.start();
    try
    {
      t.join();
      itf.notifyValue("Thread finished");
    } catch (InterruptedException ex)
    {
      Thread.currentThread().interrupt();
      itf.notifyValue("Thread has been interrupted");
    }
  }

  public Object[] testCallbackWithParameters(TestInterface2 itf)
  {
    byte[] vals =
    {
      1, 2, 3, 4
    };
    return itf.write(vals, 12, 13);
  }

  public boolean testEquals(Object o)
  {
    return o.equals(o);
  }
}
