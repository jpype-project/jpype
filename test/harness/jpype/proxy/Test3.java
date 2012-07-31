//*****************************************************************************
//Copyright 2004-2008 Steve Menard
//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.
//
//*****************************************************************************
package jpype.proxy;

public class Test3
{
    public static void testProxy(ITestInterface2 itf)
    {
        System.out.println("Test Method = "+itf.testMethod());
        if (itf instanceof ITestInterface3)
        {
            System.out.println("Test Method2 = "+((ITestInterface3)itf).testMethod2());
        }
    }
    
    public void testProxyWithThread(final ITestInterface2 itf)
    {
        Thread t = new Thread(new Runnable() {
            public void run()
            {
                for (int i = 0; i < 10; i++)
                {
                    itf.testMethod();
                }

            }
        });
        t.start();
        
        try {
            System.out.println("Waiting for thread to finish");
            t.join();
            System.out.println("Thread has finished");
        }
        catch(InterruptedException ex)
        {
            
        }        
    }
    
    public void testCallbackWithParameters(ITestInterface2 itf)
    {
    	byte[] vals = { 1, 2, 3, 4};
    	itf.write(vals , 12, 13);
    }
}