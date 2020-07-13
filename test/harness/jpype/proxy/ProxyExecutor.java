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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

public class ProxyExecutor
{

  private ExecutorService executor;

  private List<Future<Integer>> executedTasks = new ArrayList<Future<Integer>>();
  private List<ProxyCaller> proxies = new ArrayList<ProxyCaller>();

  public ProxyExecutor(int noOfThreads)
  {
    executor = Executors.newFixedThreadPool(noOfThreads);
  }

  public void shutdown()
  {
    executor.shutdown(); // Disable new tasks from being submitted
    try
    {
      // Wait a while for existing tasks to terminate
      if (!executor.awaitTermination(5, TimeUnit.SECONDS))
      {
        executor.shutdownNow(); // Cancel currently executing tasks
        // Wait a while for tasks to respond to being cancelled
        if (!executor.awaitTermination(5, TimeUnit.SECONDS))
          System.err.println("Pool did not terminate");
      }
    } catch (InterruptedException ie)
    {
      // (Re-)Cancel if current thread also interrupted
      executor.shutdownNow();
      // Preserve interrupt status
      Thread.currentThread().interrupt();
    }
  }

  public void registerProxy(TestInterface4 proxy, int noOfExecutions)
  {

    for (int i = 0; i < noOfExecutions; i++)
    {
      proxies.add(new ProxyCaller(proxy));
    }
  }

  public int getExpectedTasks()
  {
    return proxies.size();
  }

  public void runExecutor()
  {

    // Collections.shuffle(proxies);
    for (ProxyCaller proxy : proxies)
    {
      Future<Integer> future = executor.submit(proxy);
      executedTasks.add(future);
    }

  }

  public int waitForExecutedTasks()
  {
    int returns = 0;

    for (Future<Integer> task : executedTasks)
    {
      try
      {
        returns += task.get();
      } catch (Exception ex)
      {
        System.out.println("An exception has thrown during execution");
        ex.printStackTrace();
      }
    }
    return returns;
  }

  public class ProxyCaller implements Callable<Integer>
  {

    private TestInterface4 proxy;

    public ProxyCaller(TestInterface4 proxy)
    {
      this.proxy = proxy;
    }

    @Override
    public Integer call() throws Exception
    {

      int result = 0;

      try
      {
        int intValue = proxy.testMethodInt();
        ReturnObject objectValue = proxy.testMethodObject();
        String stringValue = proxy.testMethodString();
        List<ReturnObject> listValue = proxy.testMethodList(5);
      } catch (Exception ex)
      {
        ex.printStackTrace();
      }

      return 1; //result;
    }

  }
}
