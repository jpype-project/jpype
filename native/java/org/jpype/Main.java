/** ***************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * See NOTICE file for details.
 **************************************************************************** */
package org.jpype;

/**
 *
 * @author nelson85
 */
public class Main
{
  
  /**
   * Return control to Python to complete startup.
   * 
   * @param args 
   */
  static native void launch(String[] args);
  
  /** Entry point for jpython.
   * 
   * This code is only accessed from the JNI side.
   * @param args 
   */
  static void mainX(String[] args, String nativeLib)
  { 
    System.out.println("Load Java stubs from "+nativeLib);
      System.load(nativeLib);
    System.out.println("starting main thread");
    // Create a new main thread for Python
    Thread thread = new Thread(()->launch(args));
    thread.start();
    System.out.println("return control");
    
    // Return control to C so we can call Destroy and wait for shutdown.
  }
}
