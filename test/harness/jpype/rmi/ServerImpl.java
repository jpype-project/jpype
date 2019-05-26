//*****************************************************************************
//Copyright 2004-2008 Steve Menard
//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//http://www.apache.org/licenses/LICENSE-2.0
//
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.
//
//*****************************************************************************

package jpype.rmi;

import java.rmi.*;
import java.rmi.server.*;
import java.rmi.registry.*;

class ServerImpl extends UnicastRemoteObject implements IServer
{
    public ServerImpl() throws RemoteException 
    {
		super();
	}
    
    public void callRemote() throws RemoteException
    {
        System.out.println("Method has been called!!!!");
    }
    
    public static void main(String args[]) 
    {
        
		// Create and install a security manager
//		if (System.getSecurityManager() == null) {
//		    System.setSecurityManager(new RMISecurityManager());
//		}
		
		try {
	        Registry reg = LocateRegistry.createRegistry(2004);
		    
		    
		    ServerImpl obj = new ServerImpl();
		
		    // Bind this object instance to the name "HelloServer"
		    Naming.rebind("//192.168.100.60:2004/server", obj);
		
		    System.out.println("HelloServer bound in registry");
		} catch (Exception e) {
		    System.out.println("HelloImpl err: " + e.getMessage());
		    e.printStackTrace();
		}
	}    
}
