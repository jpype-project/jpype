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

  static native void initialize(ClassLoader loader);
  
  /**
   * Entry point for jpython.
   *
   * This code is only accessed from the JNI side.
   *
   * @param args
   */
  static void mainX(String[] args, String nativeLib)
  {
    // Load the native library
    System.load(nativeLib);
    
    // Create a new main thread for Python
    ClassLoader classLoader = Main.class.getClassLoader();
    initialize(classLoader);
    
    // Launch Python in a new thread
    Thread thread = new Thread(() -> launch(args), "Python");
    thread.start();
    // Return control to C so we can call Destroy and wait for shutdown.
  }
}
